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
  static Report*    _instance;
  static Type       _verb;
protected:
  Report() {}
public:
  // Create/get current instance of this singleton
  static Report* instance();
  // Set output verbosity level
  Type verbosity(Type level = none);
  // Report text
  int report(
    const char*     text,
    Type            type = info) const;
};

}

#endif
