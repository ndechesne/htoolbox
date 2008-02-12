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

/* The aim of this part is to concentrate the reporting stuff */

#ifndef REPORT_H
#define REPORT_H

#include <string>
#include <sstream>

namespace hbackup {

namespace report {

enum Level {
  // These go to error output
  alert,        // Your're dead
  error,        // Big issue, but might recover
  // These go to standard output
  warning,      // Non-blocking issue
  info,         // Normal information, typically if 'quiet' not selected
  verbose,      // Extra information, typically if 'verbose' selected
  debug         // Developper information, typically if 'debug' selected
};

class Report {
  static Report*    _self;
  static Level      _level;
protected:
  Report() {}
public:
  // Create/get current instance of this singleton
  static Report* self();
  // Set output verbosity level
  Level operator=(Level level);
  // Report text, at given level
  static ostream& out(Level level);
};

inline void setOutLevel(Level level) {
  *Report::self() = level;
}

inline ostream& out(Level level = info) {
  return Report::self()->out(level);
}

}

}

#endif
