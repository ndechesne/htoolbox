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
  bool    failed = false;
  off64_t size;

  if (type.substr(0, 4) == "size") {
    char    unit[4] = { '\0', '\0', '\0', '\0' };
    int     rc = sscanf(value.c_str(), "%lld%3s", &size, unit);
    unit[3] = '\0';
    if (rc < 1) {
      failed = true;
      out(error) << "Cannot decode decimal value" << endl;
    } else {
      int kilo;
      if (rc > 1) {
        // *iB or *i
        if ((unit[1] == 'i') && ((unit[2] == 'B') || (unit[2] == '\0'))) {
          kilo = 1024;
        } else if ((unit[1] == 'B') || (unit[1] == '\0')) {
          kilo = 1000;
        } else {
          out(error) << "Cannot decode unit, must be 'iB' or 'B'" << endl;
          failed = true;
        }
      }
      if (! failed) {
        switch (unit[0]) {
          case 'Y':
            size *= kilo;
          case 'Z':
            size *= kilo;
          case 'E':
            size *= kilo;
          case 'P':
            size *= kilo;
          case 'T':
            size *= kilo;
          case 'G':
            size *= kilo;
          case 'M':
            size *= kilo;
          case 'k':
          // Accept capital K too, even though it's incorrect ;)
          case 'K':
            size *= kilo;
          case '\0':
            break;
          default:
            failed = true;
            out(error) << "Cannot decode multiplier" << endl;
        }
      }
    }
  }

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
    Condition* cond = new Condition(Condition::name_regex, value, negated);
    Node test("");
    if (cond->match("", test) >= 0) {
      add(cond);
    } else {
      out(error) << "Cannot create regex" << endl;
      failed = true;
    }
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
    Condition* cond = new Condition(Condition::name_regex, value, negated);
    Node test("");
    if (cond->match("", test) >= 0) {
      add(cond);
    } else {
      out(error) << "Cannot create regex" << endl;
      failed = true;
    }
  } else
  if (type == "size<") {
    if (! failed) {
      add(new Condition(Condition::size_lt, size, negated));
    }
  } else
  if (type == "size<=") {
    if (! failed) {
      add(new Condition(Condition::size_le, size, negated));
    }
  } else
  if (type == "size>=") {
    if (! failed) {
      add(new Condition(Condition::size_ge, size, negated));
    }
  } else
  if (type == "size>") {
    if (! failed) {
      add(new Condition(Condition::size_gt, size, negated));
    }
  } else
  if (type == "mode&") {
    mode_t mode;
    failed = (sscanf(value.c_str(), "%o", &mode) != 1);
    if (failed) {
      out(error) << "Cannot decode octal value" << endl;
    } else {
      add(new Condition(Condition::mode_and, mode, negated));
    }
  } else
  if (type == "mode=") {
    mode_t mode;
    failed = (sscanf(value.c_str(), "%o", &mode) != 1);
    if (failed) {
      out(error) << "Cannot decode octal value" << endl;
    } else {
      add(new Condition(Condition::mode_eq, mode, negated));
    }
  } else
  {
    // Wrong type
    return -2;
  }
  return failed ? -1 : 0;
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

void Filter::show(int level) const {
  out(verbose, level) << "Filter " << ((_type == any) ? "or" : "and") << " "
    << _name <<  endl;
  // Show all conditions
  list<Condition*>::const_iterator condition;
  for (condition = begin(); condition != end(); condition++) {
    (*condition)->show(level + 1);
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

void Filters::show(int level) const {
  list<Filter*>::const_iterator filter;
  for (filter = begin(); filter != end(); filter++) {
    (*filter)->show(level);
  }
}
