/*
     Copyright (C) 2006-2010  Herve Fache

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

#include <stdio.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "hreport.h"
#include "conditions.h"
#include "filters.h"

using namespace hbackup;
using namespace htools;

Filter::~Filter() {
  list<Condition*>::const_iterator condition;
  for (condition = _children.begin(); condition != _children.end();
      condition++) {
    delete *condition;
  }
}

Condition* Filter::configChildFactory(
    const vector<string>& params,
    const char*           file_path,
    size_t                line_no) {
  const string& filter_type = params[1];
  const string& value = params[2];
  size_t pos = 0;
  bool negated;
  if (filter_type[pos] == '!') {
    ++pos;
    negated = true;
  } else {
    negated = false;
  }
  bool case_sensitive = true;
  if (filter_type[pos] == 'i') {
    ++pos;
    case_sensitive = false;
  } else {
    case_sensitive = true;
  }
  string type = filter_type.substr(pos);

  /* Add specified filter */
  Condition* cond = NULL;
  bool failed = false;
  long long int size;

  if (type.substr(0, 4) == "size") {
    char    unit[4] = { '\0', '\0', '\0', '\0' };
    int     rc = sscanf(value.c_str(), "%lld%3s", &size, unit);
    unit[3] = '\0';
    if (rc < 1) {
      hlog_error("%s:%zd cannot decode decimal value '%s'",
        file_path, line_no, value.c_str());
      failed = true;
    } else {
      int kilo = 0;
      if (rc > 1) {
        // *iB or *i
        if ((unit[1] == 'i') && ((unit[2] == 'B') || (unit[2] == '\0'))) {
          kilo = 1024;
        } else if ((unit[1] == 'B') || (unit[1] == 'b') || (unit[1] == '\0')) {
          kilo = 1000;
        } else {
          hlog_error("%s:%zd cannot decode unit of '%s', must end in 'iB' or 'B'",
            file_path, line_no, value.c_str());
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
            hlog_error("%s:%zd cannot decode multiplier from string '%s', "
              "must be k, K, M, G, T, P, E, Z or Y",
              file_path, line_no, value.c_str());
            failed = true;
        }
      }
    }
  }

  if (type == "filter") {
    if (_parent != NULL) {
      Filter* subfilter = _parent->find(value);
      if (subfilter != NULL) {
        cond = new Condition(Condition::filter, subfilter, negated);
      }
    }
  } else
  if (type == "type") {
    cond = new Condition(Condition::type, value[0], negated);
  } else
  if (type == "name") {
    cond = new Condition(Condition::name, value.c_str(), negated,
      case_sensitive);
  } else
  if (type == "name_start") {
    cond = new Condition(Condition::name_start, value.c_str(), negated,
      case_sensitive);
  } else
  if (type == "name_end") {
    cond = new Condition(Condition::name_end, value.c_str(), negated,
      case_sensitive);
  } else
  if (type == "name_regex") {
    cond = new Condition(Condition::name_regex, value.c_str(), negated);
    Node test("");
    if (cond->match(test) < 0) {
      hlog_error("%s:%zd cannot create regex '%s'",
        file_path, line_no, value.c_str());
      delete cond;
      cond = NULL;
    }
  } else
  if (type == "path") {
    cond = new Condition(Condition::path, value.c_str(), negated,
      case_sensitive);
  } else
  if (type == "path_start") {
    cond = new Condition(Condition::path_start, value.c_str(), negated,
      case_sensitive);
  } else
  if (type == "path_end") {
    cond = new Condition(Condition::path_end, value.c_str(), negated,
      case_sensitive);
  } else
  if (type == "path_regex") {
    cond = new Condition(Condition::name_regex, value.c_str(), negated);
    Node test("");
    if (cond->match(test) < 0) {
      hlog_error("%s:%zd cannot create regex '%s'",
        file_path, line_no, value.c_str());
      delete cond;
      cond = NULL;
    }
  } else
  if (type == "size<") {
    if (! failed) {
      cond = new Condition(Condition::size_lt, size, negated);
    }
  } else
  if (type == "size<=") {
    if (! failed) {
      cond = new Condition(Condition::size_le, size, negated);
    }
  } else
  if (type == "size>=") {
    if (! failed) {
      cond = new Condition(Condition::size_ge, size, negated);
    }
  } else
  if (type == "size>") {
    if (! failed) {
      cond = new Condition(Condition::size_gt, size, negated);
    }
  } else
  if (type == "mode&") {
    mode_t mode;
    failed = (sscanf(value.c_str(), "%o", &mode) != 1);
    if (failed) {
      hlog_error("%s:%zd cannot decode octal value '%s'",
        file_path, line_no, value.c_str());
    } else {
      cond = new Condition(Condition::mode_and, mode, negated);
    }
  } else
  if (type == "mode=") {
    mode_t mode;
    failed = (sscanf(value.c_str(), "%o", &mode) != 1);
    if (failed) {
      hlog_error("%s:%zd cannot decode octal value '%s'",
        file_path, line_no, value.c_str());
    } else {
      cond = new Condition(Condition::mode_eq, mode, negated);
    }
  } else
  {
    hlog_error("%s:%zd unsupported condition type '%s'",
      file_path, line_no, type.c_str());
    return NULL;
  }
  if (cond != NULL) {
    add(cond);
  } else {
    hlog_error("%s:%zd failed to add condition '%s'",
      file_path, line_no, type.c_str());
  }
  return cond;
}

bool Filter::match(
    const Node&     node,
    size_t          start) const {
  // Test all conditions
  list<Condition*>::const_iterator condition;
  for (condition = _children.begin(); condition != _children.end();
      condition++) {
    bool matches = (*condition)->match(node, start);
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
  hlog_debug_arrow(level, "Filter: %s %s", _type == any ? "or" : "and",
    _name.c_str());
  // Show all conditions
  list<Condition*>::const_iterator condition;
  for (condition = _children.begin(); condition != _children.end();
      condition++) {
    (*condition)->show(level + 1);
  }
}

Filters::~Filters() {
  // Delete all filters
  list<Filter*>::const_iterator filter;
  for (filter = _children.begin(); filter != _children.end(); filter++) {
    delete *filter;
  }
}

Filter* Filters::find(
    const string&   name) const {
  list<Filter*>::const_iterator filter;
  for (filter = _children.begin(); filter != _children.end(); filter++) {
    if ((*filter)->name() == name) {
      return *filter;
    }
  }
  if (_parent == NULL) {
    return NULL;
  }
  return _parent->find(name);
}

Filter* Filters::configChildFactory(
    const vector<string>& params,
    const char*           file_path,
    size_t                line_no) {
  const string& type = params[1];
  const string& name = params[2];
  Filter::Mode ftype;
  if ((type == "and") || (type == "all")) {
    ftype = Filter::all;
  } else
  if ((type == "or") || (type == "any")) {
    ftype = Filter::any;
  } else
  {
    hlog_error("%s:%zd unsupported filter type '%s'",
      file_path, line_no, type.c_str());
    return NULL;
  }
  _last_filter = new Filter(ftype, name.c_str(), this);
  _children.push_back(_last_filter);
  return _last_filter;
}

void Filters::show(int level) const {
  list<Filter*>::const_iterator filter;
  for (filter = _children.begin(); filter != _children.end(); filter++) {
    (*filter)->show(level);
  }
}
