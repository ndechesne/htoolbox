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

Report*      Report::_self  = NULL;
// Display all non-debug messages
Report::Type Report::_level = info;

Report* Report::self() {
  if (_self == NULL) {
    _self = new Report;
  }
  return _self;
}

Report::Type Report::operator=(Type level) {
  Report* report = Report::self();
  report->_level = level;
  return level;
}

int Report::report(
    const char*     text,
    Type            type) {
  if (type > _level) {
    // Message not displayed
    return 1;
  }
  switch (type) {
    case alert:
      cerr << "ALERT! " << text << endl;
      break;
    case error:
      cerr << "Error: " << text << endl;
      break;
    case warning:
      cout << "Warning: " << text << endl;
      break;
    case info:
      cout << text << endl;
      break;
    case debug:
      cout << "debug: " << text << endl;
      break;
    default:
      cout << "???: " << text << endl;
  }
  return 0;
}

int Report::out(
    const char*     text,
    Type            type) {
  return self()->report(text, type);
}

int Report::out(
    string&         text,
    Type            type) {
  return out(text.c_str(), type);
}

int Report::out(
    stringstream&   text,
    Type            type) {
  return out(text.str().c_str(), type);
}
