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
#include "hbackup.h"

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
    add(new Condition(condition_type, value[0], negated));
  } else
  if (type == "name") {
    add(new Condition(condition_name, value, negated));
  } else
  if (type == "name_start") {
    add(new Condition(condition_name_start, value, negated));
  } else
  if (type == "name_end") {
    add(new Condition(condition_name_end, value, negated));
  } else
  if (type == "name_regex") {
    add(new Condition(condition_name_regex, value, negated));
  } else
  if (type == "path") {
    add(new Condition(condition_path, value, negated));
  } else
  if (type == "path_start") {
    add(new Condition(condition_path_start, value, negated));
  } else
  if (type == "path_end") {
    add(new Condition(condition_path_end, value, negated));
  } else
  if (type == "path_regex") {
    add(new Condition(condition_path_regex, value, negated));
  } else
  if (type == "size<") {
    off64_t size = strtoul(value.c_str(), NULL, 10);
    add(new Condition(condition_size_lt, size, negated));
  } else
  if (type == "size<=") {
    off64_t size = strtoul(value.c_str(), NULL, 10);
    add(new Condition(condition_size_le, size, negated));
  } else
  if (type == "size>=") {
    off64_t size = strtoul(value.c_str(), NULL, 10);
    add(new Condition(condition_size_ge, size, negated));
  } else
  if (type == "size>") {
    off64_t size = strtoul(value.c_str(), NULL, 10);
    add(new Condition(condition_size_gt, size, negated));
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
  filter_type_t ftype;
  if ((type == "and") || (type == "all")) {
    ftype = filter_and;
  } else
  if ((type == "or") || (type == "any")) {
    ftype = filter_or;
  } else
  {
    return NULL;
  }
  Filter* filter = new Filter(ftype, name.c_str());
  push_back(filter);
  return filter;
}
