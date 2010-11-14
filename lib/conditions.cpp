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

#include <regex.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "hreport.h"
#include "conditions.h"
#include "filters.h"

using namespace hbackup;
using namespace htools;

struct Condition::Private {
  Type              type;
  bool              negated;
  bool              case_sensitive;
  const Filter*     filter;
  char              file_type;
  long long         value;
  char*             string;
  regex_t*          regex;
  Private() : case_sensitive(false) {}
};

Condition::Condition(
    Type            type,
    const Filter*   filter,
    bool            negated) : _d(new Private) {
  _d->type      = type;
  _d->negated   = negated;
  _d->filter    = filter;
  _d->string    = NULL;
  _d->regex     = NULL;
}

Condition::Condition(
    Type            type,
    char            file_type,
    bool            negated) : _d(new Private) {
  _d->type      = type;
  _d->negated   = negated;
  _d->string    = NULL;
  _d->file_type = file_type;
  _d->regex     = NULL;
}

Condition::Condition(
    Type            type,
    mode_t          value,
    bool            negated) : _d(new Private) {
  _d->type      = type;
  _d->negated   = negated;
  _d->string    = NULL;
  _d->value     = value;
  _d->regex     = NULL;
}

Condition::Condition(
    Type            type,
    long long       value,
    bool            negated) : _d(new Private) {
  _d->type      = type;
  _d->negated   = negated;
  _d->string    = NULL;
  _d->value     = value;
  _d->regex     = NULL;
}

Condition::Condition(
    Type            type,
    const char*     string,
    bool            negated,
    bool            case_sensitive) : _d(new Private) {
  _d->type           = type;
  _d->negated        = negated;
  _d->case_sensitive = case_sensitive;
  _d->string         = strdup(string);
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
  delete _d;
}

bool Condition::match(
    const Node&     node,
    size_t          start) const {
  const char* path = &node.path()[start];
  bool result = false;
  bool failed = false;

  switch(_d->type) {
    case Condition::filter: {
        result = _d->filter->match(node, start);
      } break;
    case Condition::type:
      result = _d->file_type == node.type();
      break;
    case Condition::name:
      if (_d->case_sensitive) {
        result = strcmp(node.name(), _d->string) == 0;
      } else {
        result = strcasecmp(node.name(), _d->string) == 0;
      }
      break;
    case Condition::name_start:
      if (_d->case_sensitive) {
        result = strncmp(node.name(), _d->string, strlen(_d->string)) == 0;
      } else {
        result = strncasecmp(node.name(), _d->string, strlen(_d->string)) == 0;
      }
      break;
    case Condition::name_end: {
      ssize_t diff = strlen(node.name()) - strlen(_d->string);
      if (diff < 0) {
        result = false;
      } else {
        if (_d->case_sensitive) {
          result = strcmp(_d->string, &node.name()[diff]) == 0;
        } else {
          result = strcasecmp(_d->string, &node.name()[diff]) == 0;
        }
      }
    } break;
    case Condition::name_regex:
        if (_d->regex != NULL) {
          result = ! regexec(_d->regex, node.name(), 0, NULL, 0);
        } else {
          hlog_error("Filters: incorrect regular expression");
          failed = true;
        }
      break;
    case Condition::path:
      if (_d->case_sensitive) {
        result = strcmp(path, _d->string) == 0;
      } else {
        result = strcasecmp(path, _d->string) == 0;
      }
      break;
    case Condition::path_start:
      if (_d->case_sensitive) {
        result = strncmp(path, _d->string, strlen(_d->string)) == 0;
      } else {
        result = strncasecmp(path, _d->string, strlen(_d->string)) == 0;
      }
      break;
    case Condition::path_end: {
      ssize_t diff = strlen(path) - strlen(_d->string);
      if (diff < 0) {
        result = false;
      } else {
        if (_d->case_sensitive) {
          result = strcmp(_d->string, &path[diff]) == 0;
        } else {
          result = strcasecmp(_d->string, &path[diff]) == 0;
        }
      }
    } break;
    case Condition::path_regex:
        if (_d->regex != NULL) {
          result = ! regexec(_d->regex, path, 0, NULL, 0);
        } else {
          hlog_error("Filters: incorrect regular expression");
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
      hlog_error("Filters: unknown condition type to match");
  }
  return failed ? -1 : (_d->negated ? ! result : result);
}

void Condition::show(int level) const {
  const char* neg = _d->negated ? "not " : "";
  const char* cas = _d->case_sensitive ? "" : "no case ";
  switch (_d->type) {
    case Condition::filter:
      hlog_debug_arrow(level, "Condition: %sfilter %s", neg,
        _d->filter->name().c_str());
      break;
    case Condition::type:
      hlog_debug_arrow(level, "Condition: %stype %c", neg, _d->file_type);
      break;
    case Condition::name:
    case Condition::name_end:
    case Condition::name_start:
    case Condition::name_regex:
    case Condition::path:
    case Condition::path_end:
    case Condition::path_start:
    case Condition::path_regex: {
      const char* type;
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
      hlog_debug_arrow(level, "Condition: %s%s%s '%s'", neg, cas, type,
        _d->string);
    } break;
    case Condition::size_ge:
    case Condition::size_gt:
    case Condition::size_le:
    case Condition::size_lt: {
      const char* type;
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
      hlog_debug_arrow(level, "Condition: %s%s %lld", neg, type, _d->value);
    } break;
    case Condition::mode_and:
    case Condition::mode_eq: {
      const char* type;
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
      hlog_debug_arrow(level, "Condition: %s%s 0%03llo", neg, type, _d->value);
    } break;
    default:
      hlog_error("Condition: unknown type");
      return;
  }
}
