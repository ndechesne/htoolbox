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

Report*        Report::_self  = NULL;

Report* Report::self() {
  if (_self == NULL) {
    _self = new Report;
  }
  return _self;
}

void Report::setVerbosityLevel(VerbosityLevel level) {
  Report::self()->_level = level;
}

ostream& Report::out(VerbosityLevel level, int arrow_length) {
  if (level > _level) {
    return null;
  } else {
    switch (level) {
      case alert:
        if (arrow_length < 0) {
          return cerr << "ALERT! ";
        } else {
          return cerr;
        }
      case error:
        if (arrow_length < 0) {
          return cerr << "Error: ";
        } else {
          return cerr;
        }
      case warning:
        if (arrow_length < 0) {
          return cout << "Warning: ";
        }
        // No break: allow warnings to use the arrows stuff
      case info:
      case verbose:
      case debug:
      {
        string arrow;
        if (arrow_length > 0) {
          arrow = " ";
          arrow.append(arrow_length, '-');
          arrow.append("> ");
        }
        return cout << arrow;
      }
    }
  }
  // g++ knows I dealt with all cases (no warning for switch), but still warns
  return null;
}

void Report::out(
    VerbosityLevel  level,
    MessageType     type,
    const char*     message,
    int             number,
    const char*     prepend) {
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
    s << prepend << ": ";
  }
  switch (type) {
    case msg_errno:
      s << strerror(number) << ": ";
      break;
    case msg_line_no:
      s << number << ": ";
      break;
    default:;
  }
  s << message;
  switch (level) {
    case alert:
    case error:
      cerr << s.str() << endl;
      break;
    case warning:
    case info:
    case verbose:
    case debug:
      cout << s.str() << endl;
  }
}
