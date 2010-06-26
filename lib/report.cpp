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
  size_t          max_size;
  size_t          max_files;
  FILE*           fd;
  Private() : size_to_overwrite(0), console_log(true), max_size(0),
      max_files(0), fd(NULL) {
    pthread_mutex_init(&mutex, NULL);
  }
  int lock() {
    return pthread_mutex_lock(&mutex);
  }
  int unlock() {
    return pthread_mutex_unlock(&mutex);
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

int Report::startFileLog(const char* name, size_t max_size, size_t max_files) {
  _d->max_size = max_size;
  _d->max_files = max_files;
  _d->fd = fopen(name, "w");
  if (_d->fd == NULL) {
    hlog_error("%s creating log file", strerror(errno));
    return -1;
  }
  _d->file_name = name;
  _d->file_log = true;
  return 0;
}

void Report::stopFileLog() {
  _d->file_log = false;
  if (_d->fd != NULL) {
    fclose(_d->fd);
    _d->fd = NULL;
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
