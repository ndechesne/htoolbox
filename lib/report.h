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

class Report {
  static Report*    _self;
  VerbosityLevel    _level;
  Report() {
    // Display all non-debug messages
    _level = info;
  }
public:
  // Create/get current instance of this singleton
  static Report* self();
  // Set output verbosity level
  void setVerbosityLevel(
    VerbosityLevel  level);
  // Report text, at given level
  ostream& out(
    VerbosityLevel  level,
    int             arrow_length = -1);
  // Report message, at given level
  void out(
    VerbosityLevel  level,
    MessageType     type,
    const char*     message,
    int             number  = -1, // Either line no or error no or arrow length
    const char*     prepend = NULL);
};

}

#endif
