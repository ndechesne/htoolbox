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
public:
  enum Type {
    // This is the impossible value
    none,
    // These go to error output
    alert,
    error,
    // These go to standard output
    warning,
    info,
    debug
  };
private:
  static Report*    _self;
  static Type       _level;
protected:
  Report() {}
public:
  // Create/get current instance of this singleton
  static Report* self();
  // Set output verbosity level
  Type operator=(Type level);
  // Report text, giving instance
  static int report(
    const char*     text,
    Type            type = info);
  // Report text
  static int out(
    const char*     text,
    Type            type = info);
  static int out(
    string&         text,
    Type            type = info);
  static int out(
    stringstream&   text,
    Type            type = info);
};

}

#endif
