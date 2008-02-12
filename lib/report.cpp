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

Report*      Report::_self  = NULL;
// Display all non-debug messages
Report::Level Report::_level = info;

Report* Report::self() {
  if (_self == NULL) {
    _self = new Report;
  }
  return _self;
}

Report::Level Report::operator=(Level level) {
  Report* report = Report::self();
  report->_level = level;
  return level;
}

ostream& Report::out(Level level) {
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
      case debug:
        return cout << "debug: ";
      default:
        return cout << "???: ";
    }
  }
}
