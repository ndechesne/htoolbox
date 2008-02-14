/*
     Copyright (C) 2006-2008  Herve Fache

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

#include <string>

using namespace std;

#include "files.h"
#include "conditions.h"
#include "filters.h"

using namespace hbackup;

Filter::~Filter() {
  list<Condition*>::const_iterator condition;
  for (condition = begin(); condition != end(); condition++) {
    delete *condition;
  }
}

int Filter::add(
    const string&   type,
    const string&   value,
    bool            negated) {
  /* Add specified filter */
  if (type == "type") {
    add(new Condition(Condition::type, value[0], negated));
  } else
  if (type == "name") {
    add(new Condition(Condition::name, value, negated));
  } else
  if (type == "name_start") {
    add(new Condition(Condition::name_start, value, negated));
  } else
  if (type == "name_end") {
    add(new Condition(Condition::name_end, value, negated));
  } else
  if (type == "name_regex") {
    add(new Condition(Condition::name_regex, value, negated));
  } else
  if (type == "path") {
    add(new Condition(Condition::path, value, negated));
  } else
  if (type == "path_start") {
    add(new Condition(Condition::path_start, value, negated));
  } else
  if (type == "path_end") {
    add(new Condition(Condition::path_end, value, negated));
  } else
  if (type == "path_regex") {
    add(new Condition(Condition::path_regex, value, negated));
  } else
  if (type == "size<") {
    off64_t size = strtoul(value.c_str(), NULL, 10);
    add(new Condition(Condition::size_lt, size, negated));
  } else
  if (type == "size<=") {
    off64_t size = strtoul(value.c_str(), NULL, 10);
    add(new Condition(Condition::size_le, size, negated));
  } else
  if (type == "size>=") {
    off64_t size = strtoul(value.c_str(), NULL, 10);
    add(new Condition(Condition::size_ge, size, negated));
  } else
  if (type == "size>") {
    off64_t size = strtoul(value.c_str(), NULL, 10);
    add(new Condition(Condition::size_gt, size, negated));
  } else
  {
    // Wrong type
    return 1;
  }
  return 0;
}

bool Filter::match(const char* path, const Node& node) const {
  // Test all conditions
  list<Condition*>::const_iterator condition;
  for (condition = begin(); condition != end(); condition++) {
    bool matches = (*condition)->match(path, node);
    if (_type == any) {
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
  if (_type == any) {
    return false;
  } else {  // filter_and
    return true;
  }
}

Filters::~Filters() {
  // Delete all filters
  list<Filter*>::const_iterator filter;
  for (filter = begin(); filter != end(); filter++) {
    delete *filter;
  }
}

Filter* Filters::find(
    const string& name) const {
  list<Filter*>::const_iterator filter;
  for (filter = begin(); filter != end(); filter++) {
    if ((*filter)->name() == name) {
      return *filter;
    }
  }
  return NULL;
}

Filter* Filters::add(
    const string&   type,
    const string&   name) {
  Filter::Mode ftype;
  if ((type == "and") || (type == "all")) {
    ftype = Filter::all;
  } else
  if ((type == "or") || (type == "any")) {
    ftype = Filter::any;
  } else
  {
    return NULL;
  }
  Filter* filter = new Filter(ftype, name.c_str());
  push_back(filter);
  return filter;
}
