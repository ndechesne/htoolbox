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
#include "stdarg.h"
#include "string.h"

#include "hbackup.h"
#include "hreport.h"

using namespace hbackup;

struct nullstream : std::ostream {
  struct nullbuf : std::streambuf {
    int overflow(int c) {
      return traits_type::not_eof(c);
    }
  } m_sbuf;
  nullstream() : std::ios(&m_sbuf), std::ostream(&m_sbuf) {}
};

static nullstream null;

void hbackup::out(
    VerbosityLevel  level,
    MessageType     type,
    const char*     message,
    int             number,
    const char*     prepend) {
  Report::self()->out(level, type, message, number, prepend);
}

Report* Report::_self = NULL;

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

Report* Report::self() {
  if (_self == NULL) {
    _self = new Report;
  }
  return _self;
}

void Report::setVerbosityLevel(VerbosityLevel level) {
  _level = level;
}

void Report::setMessageCallback(message_f message) {
  _message = message;
}

void Report::out(
    VerbosityLevel  level,
    MessageType     type,
    const char*     message,
    int             number,
    const char*     prepend) {
  if (_message != NULL) {
    (*_message)(level, type, message, number, prepend);
    return;
  }
  if (level > _level) {
    return;
  }
  stringstream s;
  if ((type != msg_standard) || (number < 0)) {
    switch (level) {
      case alert:
        s << "ALERT! ";
        break;
      case error:
        s << "Error: ";
        break;
      case warning:
        s << "Warning: ";
        break;
      default:;
    }
  } else if (number > 0) {
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
  Report::out(level, (number == -3), "%s", s.str().c_str());
}

int Report::out(
    VerbosityLevel  level,
    bool            temporary,
    const char*     format,
    ...) {
  // get instance
  Report* report = Report::self();
  if (report == NULL) {
    return -1;
  }
  // print only if required
  if (level > report->_level) {
    return 0;
  }
  // output fd depends on level
  FILE* fd;
  switch (level) {
    case alert:
    case error:
      fd = stderr;
      break;
    default:
      fd = stdout;
  }
  // lock
  // fill in buffer
  va_list ap;
  va_start(ap, format);
  int rc = vsnprintf(report->_buffer, sizeof(_buffer), format, ap);
  va_end(ap);
  size_t buffer_size = sizeof(report->_buffer) - 1;
  report->_buffer[buffer_size] = '\0';
  // compute UTF-8 string length
  size_t size = 0;
  if (temporary || (report->_size_to_overwrite != 0)) {
    size = utf8_len(report->_buffer);
  }
  // if previous length stored, overwrite end of previous line
  if ((report->_size_to_overwrite != 0) && (report->_size_to_overwrite > size)){
    size_t diff = report->_size_to_overwrite - size;
    if (diff > (buffer_size - rc)) {
      diff = buffer_size - rc;
    }
    memset(&report->_buffer[rc], ' ', diff);
    rc += diff;
  }
  // print
  fwrite(report->_buffer, rc, 1, fd);
  if (temporary) {
    fprintf(fd, "\r");
  } else {
    fprintf(fd, "\n");
  }
  fflush(fd);
  // if temp, store length (should be UTF-8 length...)
  if (temporary) {
    report->_size_to_overwrite = size;
  } else {
    report->_size_to_overwrite = 0;
  }
  // unlock
  return rc;
}
