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

#include <iostream>

#include <regex.h>

using namespace std;

#include "files.h"
#include "conditions.h"
#include "filters.h"
#include "hbackup.h"

using namespace hbackup;

Condition::Condition(
    Type            type,
    const Filter*   filter,
    bool            negated) :
      _type(type), _negated(negated), _filter(filter), _regex_ok(false) {}

Condition::Condition(
    Type            type,
    char            file_type,
    bool            negated) :
      _type(type), _negated(negated), _file_type(file_type), _regex_ok(false){}

Condition::Condition(
    Type            type,
    mode_t          value,
    bool            negated) :
      _type(type), _negated(negated), _value(value), _regex_ok(false) {}

Condition::Condition(
    Type            type,
    long long       value,
    bool            negated) :
      _type(type), _negated(negated), _value(value), _regex_ok(false) {}

Condition::Condition(
    Type            type,
    const string&   str,
    bool            negated) :
      _type(type), _negated(negated), _string(str) {
  if (((type == Condition::name_regex) || (type == Condition::path_regex))
  &&  (regcomp(&_regex, _string.c_str(), REG_EXTENDED) == 0)) {
    _regex_ok = true;
  } else {
    _regex_ok = false;
  }
}

Condition::~Condition() {
  if (_regex_ok) {
    regfree(&_regex);
  }
}

bool Condition::match(const char* npath, const Node& node) const {
  // TODO Use char arrays
  string name   = node.name();
  string path   = npath + name;
  bool   result = false;
  bool   failed = false;

  switch(_type) {
    case Condition::filter: {
        result = _filter->match(npath, node);
      } break;
    case Condition::type:
      result = _file_type == node.type();
      break;
    case Condition::name:
      result = strcmp(node.name(), _string.c_str()) == 0;
      break;
    case Condition::name_start:
      result = name.substr(0, _string.size()) == _string;
      break;
    case Condition::name_end: {
      signed int diff = name.size() - _string.size();
      if (diff < 0) {
        result = false;
      } else {
        result = _string == name.substr(diff); }
      }
      break;
    case Condition::name_regex:
        if (_regex_ok) {
          result = ! regexec(&_regex, name.c_str(), 0, NULL, 0);
        } else {
          out(error, msg_standard, "incorrect regular expression", -1,
            "Filters");
          failed = true;
        }
      break;
    case Condition::path:
      result = path == _string;
      break;
    case Condition::path_start:
      result = path.substr(0, _string.size()) == _string;
      break;
    case Condition::path_end: {
        signed int diff = path.size() - _string.size();
        if (diff < 0) {
          result = false;
        } else {
          result = _string == path.substr(diff);
        }
      } break;
    case Condition::path_regex:
        if (_regex_ok) {
          result = ! regexec(&_regex, path.c_str(), 0, NULL, 0);
        } else {
          out(error, msg_standard, "incorrect regular expression", -1,
            "Filters");
          failed = true;
        }
      break;
    case Condition::size_ge:
      result = node.size() >= _value;
      break;
    case Condition::size_gt:
      result = node.size() > _value;
      break;
    case Condition::size_le:
      result = node.size() <= _value;
      break;
    case Condition::size_lt:
      result = node.size() < _value;
      break;
    case Condition::mode_and:
      result = (node.mode() & _value) != 0;
      break;
    case Condition::mode_eq:
      result = node.mode() == _value;
      break;
    default:
      out(error, msg_standard, "unknown condition type to match", -1,
        "Filters");
  }
  return failed ? -1 : (_negated ? ! result : result);
}

void Condition::show(int level) const {
  stringstream s;
  switch (_type) {
    case Condition::filter:
      s << (_negated ? "not " : "") << "filter " << _filter->name();
      break;
    case Condition::type:
      s << (_negated ? "not " : "") << "type " << _file_type;
      break;
    case Condition::name:
    case Condition::name_end:
    case Condition::name_start:
    case Condition::name_regex:
    case Condition::path:
    case Condition::path_end:
    case Condition::path_start:
    case Condition::path_regex: {
      string type;
      switch (_type) {
        case Condition::name:
          type = "name";
          break;
        case Condition::name_end:
          type = "name_end";
          break;
        case Condition::name_start:
          type = "name_start";
          break;
        case Condition::name_regex:
          type = "name_regex";
          break;
        case Condition::path:
          type = "path";
          break;
        case Condition::path_end:
          type = "path_end";
          break;
        case Condition::path_start:
          type = "path_start";
          break;
        case Condition::path_regex:
          type = "path_regex";
          break;
        default:
          type = "unknown";
      }
      s << (_negated ? "not " : "") << type << " " << _string;
      } break;
    case Condition::size_ge:
    case Condition::size_gt:
    case Condition::size_le:
    case Condition::size_lt: {
      string type;
      switch (_type) {
        case Condition::size_ge:
          type = "size>=";
          break;
        case Condition::size_gt:
          type = "size>";
          break;
        case Condition::size_le:
          type = "size<=";
          break;
        case Condition::size_lt:
          type = "size<";
          break;
        default:
          type = "unknown";
      }
      s << (_negated ? "not " : "") << type << " " << _value;
      } break;
    case Condition::mode_and:
    case Condition::mode_eq: {
      string type;
      switch (_type) {
        case Condition::mode_and:
          type = "mode&";
          break;
        case Condition::mode_eq:
          type = "mode=";
          break;
        default:
          type = "unknown";
      }
      s << (_negated ? "not " : "") << type << " " << oct << _value << dec;
      } break;
    default:
      out(error, msg_standard, "Unknown type", -1, "Condition");
      return;
  }
  out(verbose, msg_standard, s.str().c_str(), level, "Condition");
}
