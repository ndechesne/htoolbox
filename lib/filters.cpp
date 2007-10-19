/*
     Copyright (C) 2006-2007  Herve Fache

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

#include "strings.h"
#include "files.h"
#include "conditions.h"
#include "filters.h"

using namespace hbackup;

Filter::~Filter() {
  list<Condition*>::const_iterator condition;
  for (condition = _conditions.begin(); condition != _conditions.end();
      condition++) {
    delete *condition;
  }
}

bool Filter::match(const char* path, const Node& node) const {
  // Test all conditions
  list<Condition*>::const_iterator condition;
  for (condition = _conditions.begin(); condition != _conditions.end();
      condition++) {
    bool matches = (*condition)->match(path, node);
    if (_type == filter_or) {
      if (matches) {
        return true;
      }
    } else {  // filter_and
      if (! matches) {
        return false;
      }
    }
  }
  // All conditions evaluated
  if (_type == filter_or) {
    return false;
  } else {  // filter_and
    return true;
  }
}
