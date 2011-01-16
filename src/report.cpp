/*
     Copyright (C) 2008-2011  Herve Fache

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License version 2 as
     published by the Free Software Foundation.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330,
     Boston, MA 02111-1307, USA.
*/

#include <string>
#include <list>

using namespace std;

#include "stdio.h"
#include "stdlib.h"
#include "limits.h"
#include "stdarg.h"
#include "string.h"
#include "errno.h"
#include "pthread.h"

#include "sys/stat.h"

#include "report.h"

#include <files.h>
#include <filereaderwriter.h>
#include <zipwriter.h>
#include <asyncwriter.h>

using namespace htoolbox;

Report htoolbox::report;
__thread Report* htoolbox::tl_report;

enum {
  FILE_NAME_MAX = 128
};

struct Report::Private {
  pthread_mutex_t mutex;
  Private() {
    pthread_mutex_init(&mutex, NULL);
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

Report::Report() : _d(new Private), _reg_filter(&_console, false) {
  _reg_filter.registerObserver(this);
  _console.open();
}

Report::~Report() {
  delete _d;
}

const char* Report::levelString(Level level) {
  switch (level) {
    case alert:
      return "alert";
    case error:
      return "error";
    case warning:
      return "warning";
    case info:
      return "info";
    case verbose:
      return "verbose";
    case debug:
      return "debug";
    case regression:
      return "regression";
  }
  return "unknown";
}

int Report::stringToLevel(const char* str, Level* level) {
  switch (str[0]) {
    case 'a':
    case 'A':
      *level = alert;
      return 0;
    case 'e':
    case 'E':
      *level = error;
      return 0;
    case 'w':
    case 'W':
      *level = warning;
      return 0;
    case 'i':
    case 'I':
      *level = info;
      return 0;
    case 'v':
    case 'V':
      *level = verbose;
      return 0;
    case 'd':
    case 'D':
      *level = debug;
      return 0;
    default:
      return -1;
  }
}

void Report::startConsoleLog() {
  _console.open();
}

void Report::stopConsoleLog() {
  _console.close();
}

void Report::notify() {
  Level level = alert;
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

void Report::addConsoleCondition(
    const char*     file_name,
    Level           min_level,
    Level           max_level,
    size_t          min_line,
    size_t          max_line) {
  _reg_filter.addCondition(file_name, min_level, max_level,
    min_line, max_line);
}

int Report::log(
    const char*     file,
    size_t          line,
    Level           level,
    bool            temp,
    size_t          ident,
    const char*     format,
    ...) {
  // print only if required
  if (level > this->level()) {
    return 0;
  }
  va_list ap;
  va_start(ap, format);
  // lock
  _d->lock();
  int rc = 0;
  for (list<Observee*>::const_iterator it = _observees.begin();
      it != _observees.end(); ++it) {
    IOutput* output = dynamic_cast<IOutput*>(*it);
    if (output->isOpen() && (level <= output->level())) {
      if (output->log(file, line, level, temp, ident, format, &ap) < 0) {
        rc = -1;
      }
    }
  }
  // unlock
  _d->unlock();
  va_end(ap);
  return rc;
}

int Report::ConsoleOutput::log(
    const char*     file,
    size_t          line,
    Level           level,
    bool            temporary,
    size_t          ident,
    const char*     format,
    va_list*        args) {
  (void) file;
  (void) line;
  // print only if required
  if (level > this->level()) {
    return 0;
  }
  FILE* fd = (level <= warning) ? stderr : stdout;
  char buffer[1024];
  size_t offset = 0;
  // prefix
  switch (level) {
    case alert:
      offset += sprintf(&buffer[offset], "ALERT! ");
      break;
    case error:
      offset += sprintf(&buffer[offset], "Error: ");
      break;
    case warning:
      offset += sprintf(&buffer[offset], "Warning: ");
      break;
    case verbose:
    case debug:
      // add arrow
      if (ident > 0) {
        /* " --n--> " */
        buffer[offset++] = ' ';
        for (size_t i = 0; i < ident; ++i) {
          buffer[offset++] = '-';
        }
        buffer[offset++] = '>';
        buffer[offset++] = ' ';
      }
      break;
    case regression:
      break;
    default:
      ;
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
  if (temporary || (_size_to_overwrite != 0)) {
    size = utf8_len(buffer);
  }
  // if previous length stored, overwrite end of previous line
  if ((_size_to_overwrite != 0) && (_size_to_overwrite > size)) {
    size_t diff = _size_to_overwrite - size;
    if (diff > (buffer_size - offset)) {
      diff = buffer_size - offset;
    }
    memset(&buffer[offset], ' ', diff);
    offset += diff;
  }
  // print
  fwrite(buffer, offset, 1, fd);
  // end
  if (temporary) {
    fprintf(fd, "\r");
  } else {
    fprintf(fd, "\n");
  }
  fflush(fd);
  // if temp, store length (should be UTF-8 length...)
  if (temporary) {
    _size_to_overwrite = size;
  } else {
    _size_to_overwrite = 0;
  }
  return static_cast<int>(offset);
}

struct Report::FileOutput::Private {
  FileOutput&     parent;
  FILE*           fd;
  char*           name;
  size_t          max_lines;
  size_t          max_files;
  size_t          lines;
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
    ZipWriter zw(&fw, false, 5);
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
          size = fr.read(buffer, BUFFER_SIZE);
          if (size <= 0) {
            if (size < 0) {
              failed = true;
            }
            break;
          }
          if (aw.write(buffer, size) < 0) {
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
#if 0
          if (no == 0) {
            /* zip the new file */
            zip(new_name, len + num_chars);
          }
#endif
        }
      }
    } while (i-- != 0);
    return rc;
  }
};

Report::FileOutput::FileOutput(
    const char*     name,
    size_t          max_lines,
    size_t          max_files) : _d(new Private(*this)) {
  _d->name = strdup(name);
  _d->max_lines = max_lines;
  _d->max_files = max_files;
}

Report::FileOutput::~FileOutput() {
  close();
  free(_d->name);
  delete _d;
}

int Report::FileOutput::open() {
  _d->lines = 0;
  if (_d->checkRotate(true) < 0) {
    hlog_error("%s creating log file: '%s'", strerror(errno), _d->name);
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
    Level           level,
    bool            temporary,
    size_t          ident,
    const char*     format,
    va_list*        args) {
  (void) ident;
  // print only if required
  if ((level > this->level()) || temporary) {
    return 0;
  }
  if (_d->checkRotate() < 0) {
    return -1;
  }
  int rc = 0;
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
  // message
  rc += vfprintf(_d->fd, format, *args);
  // end
  rc += fprintf(_d->fd, "\n");
  fflush(_d->fd);
  ++_d->lines;
  return rc;
}

int Report::TlvOutput::log(
    const char*     file,
    size_t          line,
    Level           level,
    bool            temporary,
    size_t          indent,
    const char*     format,
    va_list*        args) {
  uint8_t tag = _start_tag;
  _sender.write(tag++, file);
  _sender.write(tag++, line);
  _sender.write(tag++, level);
  _sender.write(tag++, temporary);
  _sender.write(tag++, indent);
  char buffer[65536];
  vsnprintf(buffer, sizeof(buffer), format, *args);
  buffer[sizeof(buffer) - 1] = '\0';
  if (_sender.write(tag++, buffer) < 0) {
    hlog_error("%s: %s logging", __FUNCTION__, strerror(errno));
    return -1;
  }
  return 0;
}

class Report::Filter::Condition {
  char        _file_name[FILE_NAME_MAX + 1];
  size_t      _file_name_length;
  Level       _min_level;
  Level       _max_level;
  size_t      _min_line;
  size_t      _max_line;
public:
  Condition(const char* f, Level b, Level t, size_t l, size_t h)
  : _min_level(b), _max_level(t), _min_line(l), _max_line(h) {
    strncpy(_file_name, f, FILE_NAME_MAX);
    _file_name[FILE_NAME_MAX] = '\0';
    _file_name_length = strlen(_file_name) + 1;
  }
  bool matches(const char* file, size_t line, Level level) const {
    if ((level < _min_level) || (level > _max_level)) {
      return true;
    }
    return (memcmp(_file_name, file, _file_name_length) == 0) &&
      (line >= _min_line) && ((_max_line == 0) || (line <= _max_line));
  }
};

Report::Filter::~Filter() {
  if ((_output != NULL) && _auto_delete) {
    delete _output;
  }
  for (std::list<Condition*>::iterator it = _conditions.begin();
      it != _conditions.end(); ++it) {
    delete *it;
  }
}

void Report::Filter::addCondition(
    const char*     file_name,
    Level           min_level,
    Level           max_level,
    size_t          min_line,
    size_t          max_line) {
  _conditions.push_back(new Condition(file_name, min_level, max_level,
    min_line, max_line));
  if (min_level < _min_level) _min_level = min_level;
  if (max_level > _max_level) _max_level = max_level;
}

bool Report::Filter::conditionsMatch(const char* file, size_t line, Level level) {
  bool matches = false;
  for (list<Condition*>::const_iterator it = _conditions.begin();
      it != _conditions.end(); ++it) {
    if ((*it)->matches(file, line, level)) {
      matches = true;
      break;
    }
  }
  return matches;
}
