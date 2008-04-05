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

#include <sstream>
#include <regex.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "report.h"
#include "conditions.h"
#include "filters.h"

using namespace hbackup;

struct Condition::Private {
  Type              type;
  bool              negated;
  const Filter*     filter;
  char              file_type;
  long long         value;
  char*             string;
  regex_t*          regex;
};

Condition::Condition(
    Type            type,
    const Filter*   filter,
    bool            negated) {
  _d            = new Private;
  _d->type      = type;
  _d->negated   = negated;
  _d->filter    = filter;
  _d->string    = NULL;
  _d->regex     = NULL;
}

Condition::Condition(
    Type            type,
    char            file_type,
    bool            negated) {
  _d            = new Private;
  _d->type      = type;
  _d->negated   = negated;
  _d->string    = NULL;
  _d->file_type = file_type;
  _d->regex     = NULL;
}

Condition::Condition(
    Type            type,
    mode_t          value,
    bool            negated) {
  _d            = new Private;
  _d->type      = type;
  _d->negated   = negated;
  _d->string    = NULL;
  _d->value     = value;
  _d->regex     = NULL;
}

Condition::Condition(
    Type            type,
    long long       value,
    bool            negated) {
  _d            = new Private;
  _d->type      = type;
  _d->negated   = negated;
  _d->string    = NULL;
  _d->value     = value;
  _d->regex     = NULL;
}

Condition::Condition(
    Type            type,
    const string&   string,
    bool            negated) {
  _d            = new Private;
  _d->type      = type;
  _d->negated   = negated;
  _d->string    = strdup(string.c_str());
  if ((type == Condition::name_regex) || (type == Condition::path_regex)) {
    _d->regex = new regex_t;
    if (regcomp(_d->regex, _d->string, REG_EXTENDED)) {
      free(_d->regex);
      _d->regex = NULL;
    }
  } else {
    _d->regex = NULL;
  }
}

Condition::~Condition() {
  free(_d->string);
  if (_d->regex != NULL) {
    regfree(_d->regex);
    delete _d->regex;
  }
}

bool Condition::match(const char* npath, const Node& node) const {
  // TODO Use char arrays
  string name   = node.name();
  string path   = npath + name;
  bool   result = false;
  bool   failed = false;

  switch(_d->type) {
    case Condition::filter: {
        result = _d->filter->match(npath, node);
      } break;
    case Condition::type:
      result = _d->file_type == node.type();
      break;
    case Condition::name:
      result = strcmp(node.name(), _d->string) == 0;
      break;
    case Condition::name_start:
      result = strncmp(name.c_str(), _d->string, strlen(_d->string)) == 0;
      break;
    case Condition::name_end: {
      signed int diff = name.size() - strlen(_d->string);
      if (diff < 0) {
        result = false;
      } else {
        result = strcmp(_d->string, name.substr(diff).c_str()) == 0;
      }
    } break;
    case Condition::name_regex:
        if (_d->regex != NULL) {
          result = ! regexec(_d->regex, name.c_str(), 0, NULL, 0);
        } else {
          out(error, msg_standard, "incorrect regular expression", -1,
            "Filters");
          failed = true;
        }
      break;
    case Condition::path:
      result = strcmp(path.c_str(), _d->string) == 0;
      break;
    case Condition::path_start:
      result = strncmp(path.c_str(), _d->string, strlen(_d->string)) == 0;
      break;
    case Condition::path_end: {
        signed int diff = path.size() - strlen(_d->string);
        if (diff < 0) {
          result = false;
        } else {
          result = strcmp(_d->string, path.substr(diff).c_str()) == 0;
        }
      } break;
    case Condition::path_regex:
        if (_d->regex != NULL) {
          result = ! regexec(_d->regex, path.c_str(), 0, NULL, 0);
        } else {
          out(error, msg_standard, "incorrect regular expression", -1,
            "Filters");
          failed = true;
        }
      break;
    case Condition::size_ge:
      result = node.size() >= _d->value;
      break;
    case Condition::size_gt:
      result = node.size() > _d->value;
      break;
    case Condition::size_le:
      result = node.size() <= _d->value;
      break;
    case Condition::size_lt:
      result = node.size() < _d->value;
      break;
    case Condition::mode_and:
      result = (node.mode() & _d->value) != 0;
      break;
    case Condition::mode_eq:
      result = node.mode() == _d->value;
      break;
    default:
      out(error, msg_standard, "unknown condition type to match", -1,
        "Filters");
  }
  return failed ? -1 : (_d->negated ? ! result : result);
}

void Condition::show(int level) const {
  stringstream s;
  switch (_d->type) {
    case Condition::filter:
      s << (_d->negated ? "not " : "") << "filter " << _d->filter->name();
      break;
    case Condition::type:
      s << (_d->negated ? "not " : "") << "type " << _d->file_type;
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
      switch (_d->type) {
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
      s << (_d->negated ? "not " : "") << type << " " << _d->string;
      } break;
    case Condition::size_ge:
    case Condition::size_gt:
    case Condition::size_le:
    case Condition::size_lt: {
      string type;
      switch (_d->type) {
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
      s << (_d->negated ? "not " : "") << type << " " << _d->value;
      } break;
    case Condition::mode_and:
    case Condition::mode_eq: {
      string type;
      switch (_d->type) {
        case Condition::mode_and:
          type = "mode&";
          break;
        case Condition::mode_eq:
          type = "mode=";
          break;
        default:
          type = "unknown";
      }
      s << (_d->negated ? "not " : "") << type << " " << oct << _d->value
        << dec;
      } break;
    default:
      out(error, msg_standard, "Unknown type", -1, "Condition");
      return;
  }
  out(verbose, msg_standard, s.str().c_str(), level, "Condition");
}
