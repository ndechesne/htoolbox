/*
     Copyright (C) 2008  Herve Fache

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

using namespace std;

#include "hbackup.h"
#include "report.h"

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
      add_colon = true;
      break;
    case msg_line_no:
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
  if (number == -3) {
    if (s.str().size() > _size_to_overwrite) {
      _size_to_overwrite = s.str().size();
    } else
    if (s.str().size() < _size_to_overwrite) {
      string blank;
      blank.append(_size_to_overwrite - s.str().size(), ' ');
      _size_to_overwrite = s.str().size();
      s << blank;
    }
    s << '\r';
  } else {
    if (_size_to_overwrite > 0) {
      string blank;
      blank.append(_size_to_overwrite, ' ');
      cout << blank << '\r' << flush;
      _size_to_overwrite = 0;
    }
    s << endl;
  }
  switch (level) {
    case alert:
    case error:
      cerr << s.str() << flush;
      break;
    case warning:
    case info:
    case verbose:
    case debug:
      cout << s.str() << flush;
  }
}
