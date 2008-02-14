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
#include <list>
// #include <string>
// #include <errno.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "report.h"
#include "parsers.h"

using namespace hbackup;

Parsers::~Parsers() {
  for (Parsers::iterator i = begin(); i != end(); i++) {
    delete *i;
  }
}

Parser* Parsers::isControlled(const string& dir_path) const {
  Parser *parser;
  for (Parsers::const_iterator i = begin(); i != end(); i++) {
    parser = (*i)->isControlled(dir_path);
    if (parser != NULL) {
      return parser;
    }
  }
  return NULL;
}

void Parsers::list() const {
  out(debug) << "List: " << size() << " parser(s)" << endl;
  for (Parsers::const_iterator i = begin(); i != end(); i++) {
    out(debug, 1) << (*i)->name() << endl;
  }
}
