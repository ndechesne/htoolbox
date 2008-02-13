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
using namespace report;

struct nullstream : std::ostream {
  struct nullbuf : std::streambuf {
    int overflow(int c) {
      return traits_type::not_eof(c);
    }
  } m_sbuf;
  nullstream() : std::ios(&m_sbuf), std::ostream(&m_sbuf) {}
};

static nullstream null;

Report* Report::_self  = NULL;
// Display all non-debug messages
VerbosityLevel   Report::_level = info;

Report* Report::self() {
  if (_self == NULL) {
    _self = new Report;
  }
  return _self;
}

VerbosityLevel Report::operator=(VerbosityLevel level) {
  Report* report = Report::self();
  report->_level = level;
  return level;
}

ostream& Report::out(VerbosityLevel level, int arrow_length) {
  if (level > _level) {
    return null;
  } else {
    switch (level) {
      case alert:
        return cerr << "ALERT! ";
      case error:
        return cerr << "Error: ";
      case warning:
        return cout << "Warning: ";
      case info:
        return cout;
      case verbose: {
        string arrow;
        if (arrow_length > 0) {
          arrow = " ";
          arrow.append(arrow_length, '-');
          arrow.append("> ");
        }
        return cout << arrow;
      }
      case debug:
        return cout;
    }
  }
  // g++ knows I dealt with all cases (no warning for switch), but still warns
  return null;
}

void report::setOutLevel(VerbosityLevel level) {
  *Report::self() = level;
}

ostream& report::out(VerbosityLevel level, int arrow_length) {
  return Report::self()->out(level, arrow_length);
}
