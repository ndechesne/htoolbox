/*
    Copyright (C) 2008 - 2011  Hervé Fache

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string>
#include <list>

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <utime.h>

#include <sys/stat.h>

#include "report.h"

#include <files.h>
#include <filereaderwriter.h>
#include <zipper.h>
#include <asyncwriter.h>

using namespace htoolbox;

Report htoolbox::report("default report");
__thread Report* htoolbox::tl_report;

enum {
  FILE_NAME_MAX = 128,
  FUNCTION_NAME_MAX = 128,
};

struct Report::Private {
  const char*     name;
  pthread_mutex_t mutex;
  Private() {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex, &attr);
    pthread_mutexattr_destroy(&attr);
  }
  ~Private() {
    pthread_mutex_destroy(&mutex);
  }
  int lock() {
    return pthread_mutex_lock(&mutex);
  }
  int unlock() {
    return pthread_mutex_unlock(&mutex);
  }
};

size_t Report::utf8_len(const char* s) {
  size_t size = 0;
  while (*s) {
    if ((*s & 0xc0) != 0x80) {
      ++size;
    }
    ++s;
  }
  return size;
}

Report::Report(const char* name) : _d(new Private), _console("default console"),
    _con_filter("default console filter", &_console, false) {
  _d->name = name;
  _con_filter.registerObserver(this);
  _console.open();
}

Report::~Report() {
  delete _d;
}

const char* Report::name() const {
  return _d->name;
}

void Report::startConsoleLog() {
  _console.open();
}

void Report::stopConsoleLog() {
  _console.close();
}

void Report::notify() {
  Criticality level(alert);
  for (list<Observee*>::const_iterator it = _observees.begin();
      it != _observees.end(); ++it) {
    IOutput* output = dynamic_cast<IOutput*>(*it);
    if (output->isOpen() && (output->level() > level)) {
      level = output->level();
    }
  }
  _level = level;
}

void Report::setLevel(Level level) {
  for (list<Observee*>::const_iterator it = _observees.begin();
      it != _observees.end(); ++it) {
    IOutput* output = dynamic_cast<IOutput*>(*it);
    output->setLevel(level);
  }
}

int Report::log(
    const char*     file,
    size_t          line,
    const char*     function,
    Level           level,
    int             flags,
    size_t          indentation,
    const char*     format,
    ...) {
  va_list ap;
  va_start(ap, format);
  // lock
  _d->lock();
  int rc = 0;
  for (list<Observee*>::const_iterator it = _observees.begin();
      it != _observees.end(); ++it) {
    IOutput* output = dynamic_cast<IOutput*>(*it);
    if (output->isOpen() && (level <= output->level())) {
      va_list aq;
      va_copy(aq, ap);
      if (output->log(file, line, function, level, flags, indentation, format,
          &aq) < 0) {
        rc = -1;
      }
      va_end(aq);
    }
  }
  // unlock
  _d->unlock();
  va_end(ap);
  return rc;
}

void Report::show(Level level, size_t indentation, bool show_closed) const {
  hlog_generic(level, false, indentation, "report '%s' [%s]:", name(),
    this->level().toString());
  for (list<Observee*>::const_iterator it = _observees.begin();
      it != _observees.end(); ++it) {
    IOutput* output = dynamic_cast<IOutput*>(*it);
    if (show_closed || output->isOpen()) {
      output->show(level, indentation + 1);
    }
  }
}

int Report::ConsoleOutput::log(
    const char*     file,
    size_t          line,
    const char*     function,
    Level           level,
    int             flags,
    size_t          indentation,
    const char*     format,
    va_list*        args) {
  (void) file;
  (void) line;
  (void) function;
  FILE* fd = (level <= warning) ? stderr : stdout;
  char buffer[1024];
  size_t offset = 0;
  // recover previous
  if (_last_flags & HLOG_NOLINEFEED) {
    // Level or temporary status change trigger a conclusion
    if ((level != _last_level) || ((flags ^ _last_flags) & HLOG_TEMPORARY)) {
      if (_last_flags & HLOG_TEMPORARY) {
        fprintf(fd, "\r");
      } else {
        fprintf(fd, "\n");
      }
      _last_flags &= ~HLOG_NOLINEFEED;
    } else {
      // Don't repeat the prefix
      flags |= HLOG_NOPREFIX;
      // backspaces
      char bsbuffer[64];
      memset(bsbuffer, '\b', sizeof(bsbuffer));
      while (_size_to_recover > 0) {
        size_t size = sizeof(bsbuffer);
        if (_size_to_recover < size) {
          size = _size_to_recover;
        }
        _size_to_recover -= size;
        fwrite(bsbuffer, size, 1, fd);
      }
    }
  }
  // prefix
  switch (level) {
    case alert:
      if (! (flags & HLOG_NOPREFIX)) {
        offset += sprintf(&buffer[offset], "ALERT! ");
      }
      break;
    case error:
      if (! (flags & HLOG_NOPREFIX)) {
        offset += sprintf(&buffer[offset], "Error: ");
      }
      break;
    case warning:
      if (! (flags & HLOG_NOPREFIX)) {
        offset += sprintf(&buffer[offset], "Warning: ");
      }
      break;
    case info:
    case verbose:
    case debug:
    case regression:
      // add arrow
      if (indentation > 0) {
        /* " --n--> " */
        buffer[offset++] = ' ';
        for (size_t i = 0; i < indentation; ++i) {
          buffer[offset++] = '-';
        }
        buffer[offset++] = '>';
        buffer[offset++] = ' ';
      }
      break;
  }
  // message
  offset += vsnprintf(&buffer[offset], sizeof(buffer) - offset, format, *args);
  size_t buffer_size = sizeof(buffer) - 1;
  /* offset is what _would_ have been written, had there been enough space */
  if (offset >= buffer_size) {
    const char ending[] = "... [truncated]";
    const size_t ending_length = sizeof(ending) - 1;
    strcpy(&buffer[buffer_size - ending_length], ending);
    offset = buffer_size;
  }
  buffer[buffer_size] = '\0';
  // compute UTF-8 string length
  size_t size = 0;
  if ((flags & HLOG_TEMPORARY) || (_size_to_overwrite != 0)) {
    size = utf8_len(buffer);
  }
  // print
  fwrite(buffer, offset, 1, fd);
  // if previous line was temporary, overwrite the end of it
  _size_to_recover = 0;
  if ((_size_to_overwrite > size) && ! (_last_flags & HLOG_NOLINEFEED)) {
    char format[16];
    sprintf(format, "%%%zus", _size_to_overwrite - size);
    fprintf(fd, format, "");
    if (flags & HLOG_NOLINEFEED) {
      _size_to_recover = _size_to_overwrite - size;
    }
  }
  if (! (_last_flags & HLOG_NOLINEFEED)) {
    _size_to_overwrite = 0;
  }
  if (! (flags & HLOG_NOLINEFEED)) {
    // end
    if (flags & HLOG_TEMPORARY) {
      fprintf(fd, "\r");
    } else {
      fprintf(fd, "\n");
    }
  }
  fflush(fd);
  // if temporary, store/update length
  if (flags & HLOG_TEMPORARY) {
    _size_to_overwrite += size;
  } else {
    _size_to_overwrite = 0;
  }
  _last_flags = flags;
  _last_level = level;
  return static_cast<int>(offset);
}

void Report::ConsoleOutput::show(Level level, size_t indentation) const {
  hlog_generic(level, false, indentation, "console '%s' (%s) [%s]", name(),
    isOpen() ? "open" : "closed", this->level().toString());
}

struct Report::FileOutput::Private {
  FileOutput&     parent;
  FILE*           fd;
  char*           name;
  size_t          max_lines;
  size_t          max_files;
  bool            zip_backups;
  size_t          lines;
  int             last_flags;
  Level           last_level;
  Private(FileOutput& p) : parent(p), fd(NULL) {}

  int checkRotate(bool init = false) {
    // check file still exists and size
    struct stat stat_buf;
    if (stat(name, &stat_buf) < 0) {
      if (parent.isOpen()) {
        parent.close();
      }
    } else
    if (init) {
      if (stat_buf.st_size != 0) {
        rotate();
      }
    } else
    if ((max_lines != 0) && (lines >= max_lines)) {
      parent.close();
      rotate();
    }
    // re-open file
    if (! parent.isOpen()) {
      fd = fopen(name, "w");
      lines = 0;
    }
    return (fd == NULL) ? -1 : 0;
  }

  int zip(char* name, size_t length) {
    FileReaderWriter fr(name, false);
    sprintf(&name[length], ".gz");
    FileReaderWriter fw(name, true);
    Zipper zw(&fw, false, 5);
    AsyncWriter aw(&zw, false);
    bool failed = false;
    if (fr.open() < 0) {
      failed = true;
    } else {
      if (aw.open() < 0) {
        failed = true;
      } else {
        // Copy
        enum { BUFFER_SIZE = 102400 };  // Too big and we end up wasting time
        char buffer1[BUFFER_SIZE];      // odd buffer
        char buffer2[BUFFER_SIZE];      // even buffer
        char* buffer = buffer1;         // currently unused buffer
        ssize_t size;                   // Size actually read at loop begining
        do {
          // size will be BUFFER_SIZE unless the end of file has been reached
          size = fr.get(buffer, BUFFER_SIZE);
          if (size <= 0) {
            if (size < 0) {
              failed = true;
            }
            break;
          }
          if (aw.put(buffer, size) < 0) {
            failed = true;
            break;
          }
          // Swap unused buffers
          if (buffer == buffer1) {
            buffer = buffer2;
          } else {
            buffer = buffer1;
          }
        } while (size == BUFFER_SIZE);
        if (aw.close() < 0) {
          failed = true;
        }
      }
      if (fr.close() < 0) {
        failed = true;
      }
    }
    if (failed) {
      ::remove(fw.path());
      return -1;
    } else {
      struct stat64 metadata;
      if (lstat64(fr.path(), &metadata) == 0) {
        struct utimbuf times = { -1, metadata.st_mtime };
        utime(fw.path(), &times);
      }
      ::remove(fr.path());
      return 0;
    }
  }

  int rotate() {
    int rc = 0;
    /* Max size for: file_name-N.gz, where N can be up to 10 digits (32 bits) */
    size_t len = strlen(name);
    char old_name[PATH_MAX];
    char new_name[PATH_MAX];
    strcpy(old_name, name);
    strcpy(new_name, name);
    /* Loop over potential files */
    size_t i = max_files;
    do {
      // Complete name
      int num_chars;
      if (i != 0) {
        num_chars = sprintf(&old_name[len], "-%zu", i);
      } else {
        old_name[len] = '\0';
        num_chars = 0;
      }
      const char* extensions[] = { "", ".gz", NULL };
      int no = Node::findExtension(old_name, extensions, len + num_chars);
      /* File found */
      if (no >= 0) {
        if (i == max_files) {
          ::remove(old_name);
        } else {
          num_chars = sprintf(&new_name[len], "-%zu", i + 1);
          if (no > 0) {
            sprintf(&new_name[len + num_chars], ".gz");
          }
          if (rename(old_name, new_name)) {
            rc = -1;
            break;
          }
          if (zip_backups && (no == 0)) {
            /* zip the new file */
            zip(new_name, len + num_chars);
          }
        }
      }
    } while (i-- != 0);
    return rc;
  }
};

Report::FileOutput::FileOutput(
    const char*     name,
    size_t          max_lines,
    size_t          max_files,
    bool            zip) : IOutput(""), _d(new Private(*this)) {
  _d->name = strdup(name);
  _d->max_lines = max_lines;
  _d->max_files = max_files;
  _d->zip_backups = zip;
}

Report::FileOutput::~FileOutput() {
  close();
  free(_d->name);
  delete _d;
}

const char* Report::FileOutput::name() const {
  return _d->name;
}

int Report::FileOutput::open() {
  _d->lines = 0;
  _d->last_flags = 0;
  if (_d->checkRotate(true) < 0) {
    return -1;
  }
  return _d->fd != NULL ? 0 : -1;
}

int Report::FileOutput::close() {
  int rc = -1;
  if (_d->fd != NULL) {
    rc = fclose(_d->fd);
    _d->fd = NULL;
  } else {
    errno = EBADF;
  }
  return rc;
}

bool Report::FileOutput::isOpen() const {
  return _d->fd != NULL;
}

int Report::FileOutput::log(
    const char*     file,
    size_t          line,
    const char*     function,
    Level           level,
    int             flags,
    size_t          indentation,
    const char*     format,
    va_list*        args) {
  (void) function;
  // print only if required
  if (flags & HLOG_TEMPORARY) {
    return 0;
  }
  if (_d->checkRotate() < 0) {
    return -1;
  }
  int rc = 0;
  if ((_d->last_flags & HLOG_NOLINEFEED) && (_d->last_level != level)) {
    rc += fprintf(_d->fd, "\n");
    _d->last_flags &= ~HLOG_NOLINEFEED;
  }
  if (! (_d->last_flags & HLOG_NOLINEFEED)) {
    // time
    time_t epoch = time(NULL);
    struct tm date;
    localtime_r(&epoch, &date); // voluntarily ignore error case
    rc += fprintf(_d->fd, "%04d-%02d-%02d %02d:%02d:%02d ", date.tm_year + 1900,
      date.tm_mon + 1, date.tm_mday, date.tm_hour, date.tm_min, date.tm_sec);
    // level
    switch (level) {
      case alert:
        rc += fprintf(_d->fd, "ALERT ");
        break;
      case error:
        rc += fprintf(_d->fd, "ERROR ");
        break;
      case warning:
        rc += fprintf(_d->fd, "WARN  ");
        break;
      case info:
        rc += fprintf(_d->fd, "INFO  ");
        break;
      case verbose:
        rc += fprintf(_d->fd, "VERB  ");
        break;
      case debug:
        rc += fprintf(_d->fd, "DEBUG ");
        break;
      case regression:
        rc += fprintf(_d->fd, "REGR  ");
        break;
    }
    // location
    rc += fprintf(_d->fd, "%s:%zd ", file, line);
    // indentation
    if (indentation > 0) {
      char indent[] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
      if (indentation < sizeof(indent)) {
        indent[indentation] = '\0';
      }
      rc += fprintf(_d->fd, "%s", indent);
    }
  }
  // message
  rc += vfprintf(_d->fd, format, *args);
  // end
  if (! (flags & HLOG_NOLINEFEED)) {
    rc += fprintf(_d->fd, "\n");
  }
  fflush(_d->fd);
  ++_d->lines;
  _d->last_flags = flags;
  _d->last_level = level;
  return rc;
}

void Report::FileOutput::show(Level level, size_t indentation) const {
  hlog_generic(level, false, indentation, "file '%s' (%s) [%s]:", name(),
    isOpen() ? "open" : "closed", this->level().toString());
  hlog_generic(level, false, indentation + 1, "name: '%s'", _d->name);
  hlog_generic(level, false, indentation + 1, "max_lines: %zu", _d->max_lines);
  hlog_generic(level, false, indentation + 1, "max_files: %zu", _d->max_files);
  hlog_generic(level, false, indentation + 1, "zip_backups: %s",
    _d->zip_backups ? "yes" : "no");
}

int Report::TlvOutput::log(
    const char*     file,
    size_t          line,
    const char*     function,
    Level           level,
    int             flags,
    size_t          indent,
    const char*     format,
    va_list*        args) {
  uint16_t tag = tlv::log_start_tag;
  tlv::TransmissionManager m;
  m.add(tag++, file, strlen(file));
  m.add(tag++, static_cast<int32_t>(line));
  m.add(tag++, function, strlen(function));
  m.add(tag++, level);
  m.add(tag++, flags);
  m.add(tag++, static_cast<int32_t>(indent));
  char buffer[65536];
  int len = vsnprintf(buffer, sizeof(buffer), format, *args);
  buffer[sizeof(buffer) - 1] = '\0';
  m.add(tag++, buffer, len);
  if (m.send(_sender, false) < 0) {
    return -1;
  }
  return 0;
}

void Report::TlvOutput::show(Level level, size_t indentation) const {
  hlog_generic(level, false, indentation, "tlv '%s' (%s) [%s]", name(),
    isOpen() ? "open" : "closed", this->level().toString());
}

int Report::TlvManager::submit(uint16_t tag, size_t size, const char* val) {
  int rc = _manager.submit(tag, size, val);
  if (rc < 0) {
    return rc;
  }
  if (tag == tlv::log_start_tag + 6) {
    Level level = static_cast<Level>(_level);
    if (tl_report != NULL) {
      tl_report->log(_file.c_str(), _line, _function.c_str(), level, _flags,
        _indent, "%s", val);
    }
    report.log(_file.c_str(), _line, _file.c_str(), level, _flags,
      _indent, "%s", val);
  }
  return 0;
}

class Report::Filter::Condition {
  Mode        _mode;
  char        _file_name[FILE_NAME_MAX + 1];
  size_t      _file_name_length;
  size_t      _min_line;
  size_t      _max_line;
  char        _function_name[FUNCTION_NAME_MAX + 1];
  size_t      _function_name_length;
  Criticality _min_level;
  Criticality _max_level;
  size_t      _index;
public:
  friend class Report::Filter;
  Condition(Mode m, const char* f, size_t l, size_t h, Level b, Level t, size_t i)
  : _mode(m), _min_line(l), _max_line(h), _min_level(b), _max_level(t), _index(i) {
    strncpy(_file_name, f, FILE_NAME_MAX);
    _file_name[FILE_NAME_MAX] = '\0';
    _file_name_length = strlen(_file_name) + 1;
    _function_name[0] = '\0';
    _function_name_length = 1;
  }
  Condition(Mode m, const char* f, const char* l, Level b, Level t, size_t i)
  : _mode(m), _min_line(0), _max_line(0), _min_level(b), _max_level(t), _index(i) {
    strncpy(_file_name, f, FILE_NAME_MAX);
    _file_name[FILE_NAME_MAX] = '\0';
    _file_name_length = strlen(_file_name) + 1;
    strncpy(_function_name, l, FUNCTION_NAME_MAX);
    _function_name[FUNCTION_NAME_MAX] = '\0';
    _function_name_length = strlen(_function_name) + 1;
  }
  bool matches(const char* file, size_t line, const char* function, Level level) const {
    if ((level < _min_level) || (level > _max_level)) {
      return false;
    }
    if ((_file_name_length != 1) &&
        (memcmp(_file_name, file, _file_name_length) != 0)) {
      return false;
    }
    if (_function_name_length == 1) {
      return (line >= _min_line) && ((_max_line == 0) || (line <= _max_line));
    } else {
      return memcmp(_function_name, function, _function_name_length) == 0;
    }
  }
  void show(Level level, size_t indentation = 0) const {
    char level_str[64] = "";
    if (_mode < accept) {
      sprintf(level_str, ", %s <= level <= %s",
        _min_level.toString(), _max_level.toString());
    }
    if (_function_name_length == 1) {
      hlog_generic(level, false, indentation,
        "%s '%s' %zu <= line <= %zu%s",
        _mode > reject ? "ACCEPT" : "REJECT", _file_name, _min_line, _max_line,
        level_str);
    } else {
      hlog_generic(level, false, indentation,
        "%s '%s' function = %s%s",
        _mode > reject ? "ACCEPT" : "REJECT", _file_name, _function_name,
        level_str);
    }
  }
};

Report::Filter::Filter(const char* name, IOutput* output, bool auto_delete)
  : IOutput(""), _output(output), _auto_delete(auto_delete), _index(0) {
  strncpy(_name, name, sizeof(_name));
  _name[sizeof(_name) - 1] = '\0';
  _output->registerObserver(this);
}

Report::Filter::~Filter() {
  if ((_output != NULL) && _auto_delete) {
    // So _output does not try to notify us
    _output->remove(this);
    delete _output;
  }
  for (std::list<Condition*>::iterator it = _conditions.begin();
      it != _conditions.end(); ++it) {
    delete *it;
  }
}

void Report::Filter::notify() {
  _level = _output->level();
  for (list<Condition*>::iterator it = _conditions.begin();
      it != _conditions.end(); ++it) {
    if (((*it)->_mode == force) && ((*it)->_max_level > _level)) {
      _level = (*it)->_max_level;
    }
  }
  notifyObservers();
}

const char* Report::Filter::ALL_FILES = "";

size_t Report::Filter::addCondition(
    Mode            mode,
    const char*     file_name,
    size_t          min_line,
    size_t          max_line,
    Level           min_level,
    Level           max_level) {
  _conditions.push_back(new Condition(mode, file_name,
    min_line, max_line, min_level, max_level, ++_index));
  notify();
  return _index;
}

size_t Report::Filter::addCondition(
    Mode            mode,
    const char*     file_name,
    const char*     function_name,
    Level           min_level,
    Level           max_level) {
  _conditions.push_back(new Condition(mode, file_name,
    function_name, min_level, max_level, ++_index));
  notify();
  return _index;
}

void Report::Filter::removeCondition(size_t index) {
  list<Condition*>::iterator it = _conditions.begin();
  while (it != _conditions.end()) {
    if ((*it)->_index == index) {
      Condition* condition = *it;
      it = _conditions.erase(it);
      delete condition;
    } else {
      ++it;
    }
  }
  notify();
}

int Report::Filter::log(
    const char*     file,
    size_t          line,
    const char*     function,
    Level           level,
    int             flags,
    size_t          indentation,
    const char*     format,
    va_list*        args) {
  // If the level is loggable, accept
  bool log_me = level <= _output->level();
  // Check all conditions
  for (list<Condition*>::const_iterator it = _conditions.begin();
      it != _conditions.end(); ++it) {
    Condition* c = *it;
    // If one matches, check whether it's an accept or a reject
    if (c->matches(file, line, function, level) &&
        ((c->_mode < accept) || (level <= _output->level()))) {
      log_me = (*it)->_mode > reject;
    }
  }
  if (log_me) {
    return _output->log(file, line, function, level, flags, indentation, format, args);
  }
  return 0;
}

void Report::Filter::show(Level level, size_t indentation) const {
  hlog_generic(level, false, indentation, "filter '%s' (%s) [%s]:", name(),
    isOpen() ? "open" : "closed", this->level().toString());
  if (_conditions.empty()) {
    hlog_generic(level, false, indentation + 1, "no conditions");
  } else {
    hlog_generic(level, false, indentation + 1, "conditions:");
    for (list<Condition*>::const_iterator it = _conditions.begin();
        it != _conditions.end(); ++it) {
      (*it)->show(level, indentation + 2);
    }
  }
  hlog_generic(level, false, indentation + 1, "output:");
  _output->show(level, indentation + 2);
}
