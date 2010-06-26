/*
     Copyright (C) 2008-2010  Herve Fache

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

#include <iostream>
#include <sstream>

using namespace std;

#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"
#include "errno.h"
#include "pthread.h"

#include "sys/stat.h"

#include "hreport.h"

using namespace hreport;

Report hreport::report;

void hreport::out_hidden(
    const char*     file,
    int             line,
    Level           level,
    MessageType     type,
    const char*     message,
    int             number,
    const char*     prepend) {
  if (level > report.level()) {
    return;
  }
  stringstream s;
  if ((type == msg_standard) && (number > 0)) {
    string arrow;
    arrow = " ";
    arrow.append(number, '-');
    arrow.append("> ");
    s << arrow;
  }
  if (prepend != NULL) {
    s << prepend;
    if (number >= -1) {
      s << ":";
    }
    s << " ";
  }
  bool add_colon = false;
  switch (type) {
    case msg_errno:
      s << strerror(number);
      s << " ";
      break;
    case msg_number:
      s << number;
      add_colon = true;
      break;
    default:;
  }
  if (message != NULL) {
    if (add_colon) {
      s << ": ";
    }
    s << message;
  }
  report.log(file, line, level, (number == -3), "%s", s.str().c_str());
}

struct Report::Private {
  pthread_mutex_t mutex;
  unsigned int    size_to_overwrite;
  bool            console_log;
  bool            file_log;
  string          file_name;
  size_t          max_lines;
  size_t          max_files;
  FILE*           fd;
  size_t          lines;
  Private() : size_to_overwrite(0), console_log(true), max_lines(0),
      max_files(0), fd(NULL) {
    pthread_mutex_init(&mutex, NULL);
  }
  int lock() {
    return pthread_mutex_lock(&mutex);
  }
  int unlock() {
    return pthread_mutex_unlock(&mutex);
  }
  int checkRotate() {
    bool need_open = file_log ? false : true;
    // check file still exists and size
    struct stat stat_buf;
    if (stat(file_name.c_str(), &stat_buf) < 0) {
      if (file_log) {
        fclose(fd);
      }
      need_open = true;
    } else
    if ((max_lines != 0) && (lines >= max_lines)) {
      fclose(fd);
      need_open = true;
      rotate();
    }
    // re-open file
    if (need_open) {
      fd = fopen(file_name.c_str(), "w");
      lines = 0;
    }
    return (fd == NULL) ? -1 : 0;
  }
  int rotate() {
    int rc = 0;
    /* Max size for: file_name-N.gz, where N can be up to 10 digits (32 bits) */
    size_t len = file_name.length();
    char* name = static_cast<char*>(malloc(len + 11 + 3 + 1));
    char* new_name = static_cast<char*>(malloc(len + 5 + 3 + 1));
    strcpy(name, file_name.c_str());
    strcpy(new_name, file_name.c_str());
    /* Loop over potential files */
    size_t i = max_files;
    do {
      int zipped = 0;
      int num_chars;
      struct stat buf;
      if (i != 0) {
        num_chars = sprintf(&name[len], "-%u", i);
      } else {
        name[len] = '\0';
        num_chars = 0;
      }
      int st = stat(name, &buf);
      if (st < 0) {
        sprintf(&name[len + num_chars], ".gz");
        zipped = 1;
        st = stat(name, &buf);
      }
      /* File found */
      if (st >= 0) {
        if (i == max_files) {
          remove(name);
        } else {
          num_chars = sprintf(&new_name[len], "-%u", i + 1);
          if (zipped) {
            sprintf(&new_name[len + num_chars], ".gz");
          }
          if (rename(name, new_name)) {
            rc = -1;
            break;
          }
          /* zip the new file */
        }
      }
    } while (i-- != 0);
    free(name);
    free(new_name);
    return rc;
  }
  int consoleLog(
      FILE*           fd,
      const char*     file,
      int             line,
      Level           level,
      bool            temporary,
      const char*     format,
      va_list*        args) {
    (void) file;
    (void) line;
    char buffer[1024];
    int offset = 0;
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
      default:
        ;
    }
    // fill in buffer
    offset += vsnprintf(&buffer[offset], sizeof(buffer), format, *args);
    size_t buffer_size = sizeof(buffer) - 1;
    buffer[buffer_size] = '\0';
    // compute UTF-8 string length
    size_t size = 0;
    if (temporary || (size_to_overwrite != 0)) {
      size = utf8_len(buffer);
    }
    // if previous length stored, overwrite end of previous line
    if ((size_to_overwrite != 0) &&
        (size_to_overwrite > size)){
      size_t diff = size_to_overwrite - size;
      if (diff > (buffer_size - offset)) {
        diff = buffer_size - offset;
      }
      memset(&buffer[offset], ' ', diff);
      offset += diff;
    }
    // print
    fwrite(buffer, offset, 1, fd);
    if (temporary) {
      fprintf(fd, "\r");
    } else {
      fprintf(fd, "\n");
    }
    fflush(fd);
    // if temp, store length (should be UTF-8 length...)
    if (temporary) {
      size_to_overwrite = size;
    } else {
      size_to_overwrite = 0;
    }
    return offset;
  }
  int fileLog(
      FILE*           fd,
      const char*     file,
      int             line,
      Level           level,
      bool            temporary,
      const char*     format,
      va_list*        args) {
    if (temporary) {
      return 0;
    }
    if (checkRotate() < 0) {
      return -1;
    }
    int rc = 0;
    // info
    time_t epoch = time(NULL);
    struct tm date;
    localtime_r(&epoch, &date); // voluntarily ignore error case
    rc += fprintf(fd, "%04d-%02d-%02d %02d:%02d:%02d ", date.tm_year + 1900,
      date.tm_mon + 1, date.tm_mday, date.tm_hour, date.tm_min, date.tm_sec);
    rc += fprintf(fd, "%s:%d ", file, line);
    // prefix
    switch (level) {
      case alert:
        rc += fprintf(fd, "ALERT ");
        break;
      case error:
        rc += fprintf(fd, "ERROR ");
        break;
      case warning:
        rc += fprintf(fd, "WARN  ");
        break;
      case info:
        rc += fprintf(fd, "INFO  ");
        break;
      case verbose:
        rc += fprintf(fd, "VERB  ");
        break;
      case debug:
        rc += fprintf(fd, "DEBUG ");
        break;
    }
    // print message
    rc += vfprintf(fd, format, *args);
    rc += fprintf(fd, "\n");
    fflush(fd);
    ++lines;
    return rc;
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

Report::Report() : _out_level(info), _d(new Private) {}

Report::~Report() {
  stopFileLog();
  delete _d;
}

void Report::startConsoleLog() {
  _d->console_log = true;
}

void Report::stopConsoleLog() {
  _d->console_log = false;
}

int Report::startFileLog(const char* name, size_t max_lines, size_t max_files) {
  stopFileLog();
  _d->file_name = name;
  _d->max_lines = max_lines;
  _d->max_files = max_files;
  _d->lines = 0;
  if (_d->checkRotate() < 0) {
    hlog_error("%s creating log file: '%s'", strerror(errno),
      _d->file_name.c_str());
    return -1;
  }
  _d->file_log = true;
  return 0;
}

void Report::stopFileLog() {
  if (_d->file_log) {
    _d->file_log = false;
    fclose(_d->fd);
  }
}

int Report::log(
    const char*     file,
    int             line,
    Level           level,
    bool            temporary,
    const char*     format,
    ...) {
  // print only if required
  if (level > _out_level) {
    return 0;
  }
  va_list ap;
  va_start(ap, format);
  // lock
  _d->lock();
  int rc = 0;
  if (_d->console_log) {
    FILE* fd = (level <= warning) ? stderr : stdout;
    rc = _d->consoleLog(fd, file, line, level, temporary, format, &ap);
  }
  if (_d->file_log) {
    rc = _d->fileLog(_d->fd, file, line, level, temporary, format, &ap);
  }
  // unlock
  _d->unlock();
  va_end(ap);
  return rc;
}
