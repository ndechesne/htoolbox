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
  char            buffer[1024];
  unsigned int    size_to_overwrite;
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

Report::Report() : _out_level(info), _d(new Private) {
  pthread_mutex_init(&_d->mutex, NULL);
  _d->size_to_overwrite = 0;
}

int Report::lock() {
  return pthread_mutex_lock(&_d->mutex);
}

int Report::unlock() {
  return pthread_mutex_unlock(&_d->mutex);
}

Report::~Report() {
  delete _d;
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
  // output fd depends on level
  int rc = 0;
  FILE* fd;
  switch (level) {
    case alert:
      rc += sprintf(_d->buffer, "ALERT! ");
      fd = stderr;
      break;
    case error:
      rc += sprintf(_d->buffer, "Error: ");
      fd = stderr;
      break;
    case warning:
      rc += sprintf(_d->buffer, "Warning: ");
      fd = stderr;
      break;
    default:
      fd = stdout;
  }
  // lock
  lock();
  // fill in buffer
  va_list ap;
  va_start(ap, format);
  rc += vsnprintf(&_d->buffer[rc], sizeof(_d->buffer), format, ap);
  va_end(ap);
  size_t buffer_size = sizeof(_d->buffer) - 1;
  _d->buffer[buffer_size] = '\0';
  // compute UTF-8 string length
  size_t size = 0;
  if (temporary || (_d->size_to_overwrite != 0)) {
    size = utf8_len(_d->buffer);
  }
  // if previous length stored, overwrite end of previous line
  if ((_d->size_to_overwrite != 0) &&
      (_d->size_to_overwrite > size)){
    size_t diff = _d->size_to_overwrite - size;
    if (diff > (buffer_size - rc)) {
      diff = buffer_size - rc;
    }
    memset(&_d->buffer[rc], ' ', diff);
    rc += diff;
  }
  // print
  fwrite(_d->buffer, rc, 1, fd);
  if (temporary) {
    fprintf(fd, "\r");
  } else {
    fprintf(fd, "\n");
  }
  fflush(fd);
  // if temp, store length (should be UTF-8 length...)
  if (temporary) {
    _d->size_to_overwrite = size;
  } else {
    _d->size_to_overwrite = 0;
  }
  // unlock
  unlock();
  return rc;
}
