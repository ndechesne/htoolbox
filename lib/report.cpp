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

Report*      Report::_instance = NULL;
// Display all non-debug messages
Report::Type Report::_verb     = info;

Report* Report::instance() {
  if (_instance == NULL) {
    _instance = new Report;
  }
  return _instance;
}

Report::Type Report::verbosity(Type verb) {
  Type last = _verb;
  if (verb != none) {
    _verb     = verb;
  }
  return last;
}

int Report::report(
    const char*     text,
    Type            type) const {
  if (type > _verb) {
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
