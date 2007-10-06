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

bool Filters::match(const char* path, const Node& node) const {
  /* Read through list of rules */
  const_iterator rule;
  for (rule = this->begin(); rule != this->end(); rule++) {
    bool match = true;

    /* Read through list of conditions in rule */
    Filter::const_iterator condition;
    for (condition = rule->begin(); condition != rule->end(); condition++) {
      /* All filters must match for rule to match */
      if (! condition->match(path, node)) {
        match = false;
        break;
      }
    }
    /* If all conditions matched, or the rule is empty, we have a rule match */
    if (match) {
      return true;
    }
  }
  /* No match */
  return false;
}
