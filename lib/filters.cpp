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

#include <stdarg.h>
#include <regex.h>
#include <sys/stat.h>

using namespace std;

#include "strings.h"
#include "files.h"
#include "filters.h"

using namespace hbackup;

bool Condition::match(const char* npath, const Node& node) const {
  // TODO Use char arrays
  string name = node.name();
  string path = npath + name;

  switch(_type) {
  case filter_type:
    return _file_type == node.type();
  case filter_name:
    return strcmp(node.name(), _string.c_str()) == 0;
  case filter_name_start:
    return name.substr(0, _string.size()) == _string;
  case filter_name_end: {
    signed int diff = name.size() - _string.size();
    if (diff < 0) {
      return false;
    }
    return _string == name.substr(diff); }
  case filter_name_regex: {
    regex_t regex;
    if (! regcomp(&regex, _string.c_str(), REG_EXTENDED)) {
      return ! regexec(&regex, name.c_str(), 0, NULL, 0);
    }
    cerr << "filters: regex: incorrect expression" << endl; }
  case filter_path:
    return path == _string;
  case filter_path_start:
    return path.substr(0, _string.size()) == _string;
  case filter_path_end: {
    signed int diff = path.size() - _string.size();
    if (diff < 0) {
      return false;
    }
    return _string == path.substr(diff); }
  case filter_path_regex: {
    regex_t regex;
    if (! regcomp(&regex, _string.c_str(), REG_EXTENDED)) {
      return ! regexec(&regex, path.c_str(), 0, NULL, 0);
    }
    cerr << "filters: regex: incorrect expression" << endl; }
  case filter_size_above:
    return node.size() >= _size;
  case filter_size_below:
    return node.size() <= _size;
  default:
    cerr << "filters: match: unknown condition type" << endl;
  }
  return false;
}

void Condition::show() const {
  switch (_type) {
    case filter_name:
    case filter_name_end:
    case filter_name_start:
    case filter_name_regex:
    case filter_path:
    case filter_path_end:
    case filter_path_start:
    case filter_path_regex:
      cout << "--> " << _string << " " << _type << endl;
      break;
    case filter_size_above:
    case filter_size_below:
      cout << "--> " << _size << " " << _type << endl;
      break;
    default:
      cout << "--> unknown condition type " << _type << endl;
  }
}

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
