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
#include "conditions.h"
#include "filters.h"

using namespace hbackup;

bool Condition::match(const char* npath, const Node& node) const {
  // TODO Use char arrays
  string name = node.name();
  string path = npath + name;
  bool  result = false;

  switch(_type) {
    case condition_subfilter: {
        result = _filter->match(npath, node);
      } break;
    case condition_type:
      result = _file_type == node.type();
      break;
    case condition_name:
      result = strcmp(node.name(), _string.c_str()) == 0;
      break;
    case condition_name_start:
      result = name.substr(0, _string.size()) == _string;
      break;
    case condition_name_end: {
      signed int diff = name.size() - _string.size();
      if (diff < 0) {
        result = false;
      } else {
        result = _string == name.substr(diff); }
      }
      break;
    case condition_name_regex: {
        regex_t regex;
        if (! regcomp(&regex, _string.c_str(), REG_EXTENDED)) {
          result = ! regexec(&regex, name.c_str(), 0, NULL, 0);
        } else {
          cerr << "filters: regex: incorrect expression" << endl;
        }
      } break;
    case condition_path:
      result = path == _string;
      break;
    case condition_path_start:
      result = path.substr(0, _string.size()) == _string;
      break;
    case condition_path_end: {
        signed int diff = path.size() - _string.size();
        if (diff < 0) {
          result = false;
        } else {
          result = _string == path.substr(diff);
        }
      } break;
    case condition_path_regex: {
        regex_t regex;
        if (! regcomp(&regex, _string.c_str(), REG_EXTENDED)) {
          result = ! regexec(&regex, path.c_str(), 0, NULL, 0);
        } else {
          cerr << "filters: regex: incorrect expression" << endl;
        }
      } break;
    case condition_size_ge:
      result = node.size() >= _value;
      break;
    case condition_size_gt:
      result = node.size() > _value;
      break;
    case condition_size_le:
      result = node.size() <= _value;
      break;
    case condition_size_lt:
      result = node.size() < _value;
      break;
    case condition_mode_and:
      result = (node.mode() & _value) != 0;
      break;
    case condition_mode_eq:
      result = node.mode() == _value;
      break;
    default:
      cerr << "filters: match: unknown condition type" << endl;
  }
  return _negated ? ! result : result;
}

void Condition::show() const {
  switch (_type) {
    case condition_subfilter:
      cout << "--> " << _filter->name() << " " << _type << endl;
      break;
    case condition_type:
      break;
    case condition_name:
    case condition_name_end:
    case condition_name_start:
    case condition_name_regex:
    case condition_path:
    case condition_path_end:
    case condition_path_start:
    case condition_path_regex:
      cout << "--> " << _string << " " << _type << endl;
      break;
    case condition_size_ge:
    case condition_size_gt:
    case condition_size_le:
    case condition_size_lt:
      cout << "--> " << _value << " " << _type << endl;
      break;
    case condition_mode_and:
    case condition_mode_eq:
      cout << "--> " << hex << _value << dec << " " << _type << endl;
      break;
    default:
      cout << "--> unknown condition type " << _type << endl;
  }
}
