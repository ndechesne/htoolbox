/*
     Copyright (C) 2008-2010  Herve Fache

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sstream>
#include <string>
#include <list>
#include <vector>

using namespace std;

#include "hreport.h"

#include "configuration.h"

using namespace hbackup;
using namespace htools;

namespace hbackup {

// To check occurrences
class ConfigCounter {
  const string&     _keyword;
  size_t            _occurrences;
public:
  ConfigCounter(const string& keyword) : _keyword(keyword), _occurrences(1) {}
  const string& keyword() const { return _keyword; }
  size_t occurrences() const { return _occurrences; }
  void increment() { _occurrences++; }
};

}

ConfigLine::~ConfigLine() {
  clear();
}

void ConfigLine::add(ConfigLine* child) {
  list<ConfigLine*>:: iterator i = _children.begin();
  while (i != _children.end()) {
    if ((*i)->_params[0] > child->_params[0]) break;
    i++;
  }
  _children.insert(i, child);
}

// Need comparator to sort back in file order
struct ConfigLineSorter :
    public std::binary_function<const ConfigLine*, const ConfigLine*, bool> {
  bool operator()(const ConfigLine* left, const ConfigLine* right) const {
    return left->lineNo() < right->lineNo();
  }
};

void ConfigLine::sortChildren() {
  _children.sort(ConfigLineSorter());
}

void ConfigLine::clear() {
  list<ConfigLine*>::iterator i;
  for (i = _children.begin(); i != _children.end(); i = _children.erase(i)) {
    delete *i;
  }
}

void ConfigLine::show(int level) const {
  stringstream s;
  for (unsigned int j = 0; j < _params.size(); j++) {
    if (j != 0) {
      s << " ";
    }
    s << _params[j];
  }
  char format[16];
  sprintf(format, "%%d:%%%ds%%s", level + 1);
  hlog_verbose(format, lineNo(), " ", s.str().c_str());
}

bool ConfigError::operator<(const ConfigError& error) const {
  return _line_no < error._line_no;
}

void ConfigError::show() const {
  char details[32] = "";
  stringstream s;
  if (_line_no >= 0) {
    if (_type != 0) {
      s << "after";
    } else {
      s << "at";
    }
    s << " line";
    sprintf(details, "%s line %d: ", (_type != 0) ? "after" : "at", _line_no);
  }
  hlog_error("%s%s", details, _message.c_str());
}

ConfigItem::~ConfigItem() {
  list<ConfigItem*>::iterator i;
  for (i = _children.begin(); i != _children.end(); i = _children.erase(i)) {
    delete *i;
  }
}

void ConfigItem::add(ConfigItem* child) {
  list<ConfigItem*>:: iterator i = _children.begin();
  while (i != _children.end()) {
    if ((*i)->keyword() > child->keyword()) break;
    i++;
  }
  _children.insert(i, child);
}

const ConfigItem* ConfigItem::find(string& keyword) const {
  list<ConfigItem*>::const_iterator i;
  for (i = _children.begin(); i != _children.end(); i++) {
    if ((*i)->_keyword == keyword) {
      return *i;
    }
  }
  return NULL;
}

bool ConfigItem::isValid(
    const list<ConfigCounter> counters,
    ConfigErrors*             errors,
    int                       line_no) const {
  bool is_valid = true;
  // Check occurrences for each child (both lists are sorted)
  list<ConfigCounter>::const_iterator j = counters.begin();
  for (list<ConfigItem*>::const_iterator i = _children.begin();
      i != _children.end(); i++) {
    // Find keyword in counters list
    size_t occurrences = 0;
    while ((j != counters.end()) && (j->keyword() < (*i)->keyword())) j++;
    if ((j != counters.end()) && (j->keyword() == (*i)->keyword())) {
      occurrences = j->occurrences();
    }
    if ((occurrences < (*i)->min_occurrences())
        || (((*i)->max_occurrences() != 0)
          && (occurrences > (*i)->max_occurrences()))) {
      is_valid = false;
      if (errors != NULL) {
        ostringstream message;
        if (occurrences < (*i)->min_occurrences()) {
          // Too few
          if (occurrences == 0) {
            message << "missing ";
          } else {
            message << "need at least "<< (*i)->min_occurrences()
              << " occurence(s) of ";
          }
        } else {
          // Too many
          message << "need at most " << (*i)->max_occurrences()
            << " occurence(s) of ";
        }
        message << "keyword '" << (*i)->keyword() << "'";
        errors->push_back(ConfigError(message.str(), line_no, 1));
      }
    }
  }
  return is_valid;
}

void ConfigItem::show(int level) const {
  list<ConfigItem*>::const_iterator i;
  for (i = _children.begin(); i != _children.end(); i++) {
    char minimum[64] = "optional";
    if ((*i)->_min_occurrences != 0) {
      sprintf(minimum, "min = %d", (*i)->_min_occurrences);
    }
    char maximum[64] = "no max";
    if ((*i)->_max_occurrences != 0) {
      sprintf(maximum, "max = %d", (*i)->_max_occurrences);
    }
    char min_max[64] = "";
    if ((*i)->_min_params != (*i)->_max_params) {
      int offset = sprintf(min_max, "min = %d, ", (*i)->_min_params);
      if ((*i)->_max_params > (*i)->_min_params) {
        sprintf(&min_max[offset], "max = %d", (*i)->_max_params);
      } else {
        sprintf(&min_max[offset], "no max");
      }
    } else {
      sprintf(min_max, "min = max = %d", (*i)->_min_params);
    }
    char format[128];
    sprintf(format, "%%%ds%%s, occ.: %%s, %%s; params: %%s", level);
    hlog_verbose(format, " ", (*i)->_keyword.c_str(), minimum, maximum, min_max);
    (*i)->show(level + 2);
  }
}

ssize_t Config::read(
    const char*     path,
    unsigned char   flags,
    ConfigSyntax&   syntax,
    ConfigObject*   root,
    ConfigErrors*   errors) {
  // Open client configuration file
  FILE* fd = fopen(path, "r");
  if (fd == NULL) {
    hlog_error("failed to open configuration file '%s': %s",
      path, strerror(errno));
    return -1;
  }
  // Where we are in the items tree
  list<const ConfigItem*> items_hierarchy;
  items_hierarchy.push_back(&syntax);

  // Where are we in the lines tree
  list<ConfigLine*> lines_hierarchy;
  lines_hierarchy.push_back(&_lines_top);

  // Where we are in the objects tree (follows the items/syntax closely)
  list<ConfigObject*> objects_hierarchy;
  objects_hierarchy.push_back(root);

  // Read through the file
  int line_no = 0;
  bool failed = false;
  ssize_t rc;
  char* buffer = NULL;
  size_t buffer_capacity = 0;
  do {
    line_no++;
    rc = getline(&buffer, &buffer_capacity, fd);
    bool eof = false;
    vector<string> params;
    if (rc < 0) {
      if (feof(fd)) {
        eof = true;
        // Force unstacking of elements and final check
        params.push_back("");
      } else {
        hlog_error("getline failure");
        failed = true;
        goto end;
      }
    } else {
      // Remove LF
      if ((rc > 0) && (buffer[rc - 1] == '\n')) {
        buffer[rc - 1] = '\0';
        --rc;
        // Remove CR
        if ((rc > 0) && (buffer[rc - 1] == '\r')) {
          buffer[rc - 1] = '\0';
          --rc;
        }
      }
      if ((rc > 0) && (extractParams(buffer, params, flags) < 0)) {
        hlog_error("extractParams failure");
        failed = true;
        break;
      }
    }
    if (params.size() > 0) {
      // Look for keyword in children of items in the current items hierarchy
      while (items_hierarchy.size() > 0) {
        // Is this a child of the current item?
        const ConfigItem* child = items_hierarchy.back()->find(params[0]);
        if (child != NULL) {
          // Yes. Add under current hierarchy
          items_hierarchy.push_back(child);
          // Add in configuration lines tree, however incorrect it may be
          ConfigLine* ln = new ConfigLine(params, line_no);
          lines_hierarchy.back()->add(ln);
          // Add under current hierarchy
          lines_hierarchy.push_back(ln);
          // Check number of parameters (params.size() - 1)
          if (   ((params.size() - 1) < child->min_params())
              || ( (child->min_params() <= child->max_params())
                && ((params.size() - 1) > child->max_params()))) {
            if (errors != NULL) {
              ostringstream message;
              message << "keyword '" << params[0]
                << "' requires ";
              if (child->min_params() == child->max_params()) {
                message << child->min_params();
              } else
              if ((params.size() - 1) < child->min_params()) {
                message << "at least " << child->min_params();
              } else
              {
                message << "at most " << child->max_params();
              }
              message << " parameter(s), found " << params.size() - 1;
              errors->push_back(ConfigError(message.str(), line_no));
            }
            // Stop creating objects
            root = NULL;
            // Keep going to find all errors
            failed = true;
          } else
          if (root != NULL) {
            ConfigObject* child_object =
              objects_hierarchy.back()->configChildFactory(params, path,
                line_no);
            if (child_object == NULL) {
              // Stop creating objects
              root = NULL;
              // Keep going to find all errors
              failed = true;
            } else {
              // Add under current hierarchy
              objects_hierarchy.push_back(child_object);
            }
          }
          break;
        } else {
          // No: we are going to go up one level, but compute/check children
          // occurrences first
          list<ConfigCounter> entries;
          for (list<ConfigLine*>::const_iterator
              i = lines_hierarchy.back()->begin();
              i != lines_hierarchy.back()->end(); i++) {
            list<ConfigCounter>::iterator j = entries.begin();
            while (j != entries.end()) {
              if (j->keyword() == (**i)[0]) {
                j->increment();
                break;
              }
              j++;
            }
            // Not found
            if (j == entries.end()) {
              entries.push_back((**i)[0]);
            }
          }
          // Check against expected
          if (! items_hierarchy.back()->isValid(entries, errors,
                  lines_hierarchy.back()->lineNo())) {
            // Stop creating objects
            root = NULL;
            // Keep going to find all errors
            failed = true;
          }
          // Keyword not found in children, go up the tree
          items_hierarchy.pop_back();
          if (root != NULL) {
            objects_hierarchy.pop_back();
          }
          lines_hierarchy.back()->sortChildren();
          lines_hierarchy.pop_back();
          if ((items_hierarchy.size() == 0) && ! eof) {
            if (errors != NULL) {
              errors->push_back(ConfigError("keyword '" + params[0]
                + "' incorrect or misplaced, aborting", line_no));
            }
            failed = true;
            goto end;
          }
        }
      }
    }
  } while (! feof(fd));
end:
  free(buffer);
  if (fclose(fd) < 0) {
    failed = true;
  }
  if (errors != NULL) {
    errors->sort();
  }
  return failed ? -1 : 0;
}

int Config::write(
    const char*     path) const {
  // Open client configuration file
  FILE* fd = fopen(path, "w");
  if (fd == NULL) {
    hlog_error("failed to open configuation file '%s': %s",
      path, strerror(errno));
    return -1;
  }
  ConfigLine* params;
  int level;
  while ((level = line(&params)) >= 0) {
    stringstream s;
    for (int j = 0; j < level << 1; j++) {
      s << " ";
    }
    for (unsigned int j = 0; j < params->size(); j++) {
      if (j != 0) {
        s << " \"";
      }
      s << (*params)[j];
      if (j != 0) {
        s << "\"";
      }
    }
    s << "\n";
    const string& str = s.str();
    if (fwrite(str.c_str(), str.size(), 1, fd) != 1) {
      break;
    }
  }
  return fclose(fd);
}

int Config::line(
    ConfigLine**    params,
    bool            reset) const {
  static list<ConfigLine*>::const_iterator       i;
  static list<const ConfigLine*>                 parents;
  static list<list<ConfigLine*>::const_iterator> iterators;

  // Reset
  if (reset || (parents.size() == 0)) {
    parents.clear();
    iterators.clear();
    i = _lines_top.begin();
    parents.push_back(&_lines_top);
    iterators.push_back(i);
  } else
  // Next
  {
    // Any children?
    if ((*i)->begin() != (*i)->end()) {
      parents.push_back(*i);
      iterators.push_back(i);
      i = (*i)->begin();
    } else {
      i++;
      while ((parents.size() > 0) && (i == parents.back()->end())) {
        i = iterators.back();
        i++;
        parents.pop_back();
        iterators.pop_back();
      }
    }
  }
  if (parents.size() > 0) {
    *params = *i;
  }
  return static_cast<int>(parents.size()) - 1;
}

void Config::clear() {
  _lines_top.clear();
}

void Config::show() const {
  hlog_debug("Config:");
  ConfigLine* params;
  int level;
  while ((level = line(&params)) >= 0) {
    params->show((level + 1) * 2);
  }
}

// This one is static
int Config::extractParams(
    const char*     line,
    vector<string>& params,
    unsigned char   flags,
    unsigned int    max_params,
    const char*     delims,
    const char*     quotes,
    const char*     comments) {
  const char* read   = line;
  char* param        = static_cast<char*>(malloc(strlen(line) + 1));
  char* write        = NULL;
  bool no_increment  = true;  // Set to re-use character next time round
  bool skip_delims;           // Set to ignore any incoming blank
  bool check_comment;         // Set after a blank was found
  bool escaped       = false; // Set after a \ was found
  bool was_escaped   = false; // To deal with quoted paths ending in '\' (DOS!)
  bool decode_next   = false; // Set when data is expected to follow
  char quote         = -1;    // Last quote when decoding, -1 when not decoding
  bool value_end     = false; // Set to signal the end of a parameter
  bool ended_well    = true;  // Set when all quotes were matched

  if (flags & flags_empty_params) {
    skip_delims   = false;
    check_comment = true;
  } else {
    skip_delims   = true;
    check_comment = false;
  }

  do {
    if (no_increment) {
      no_increment = false;
    } else {
      read++;
      if (*read != 0) {
        // This is only used at end of line
        was_escaped = false;
      }
    }
    if (check_comment) {
      // Before deconfig, verify it's not a comment
      bool comment = false;
      for (const char* c = comments; *c != '\0'; c++) {
        if (*c == *read) {
          comment = true;
          break;
        }
      }
      if (comment) {
        // Nothing more to do
        break;
      } else if (decode_next && (*read == 0)) {
        // Were expecting one more parameter but met eof
        *param    = '\0';
        value_end = true;
      } else {
        check_comment = false;
        // Do not increment, as this a meaningful character
        no_increment = true;
      }
    } else if (quote > 0) {
      // Decoding quoted string
      if (*read == quote) {
        if (escaped) {
          *write++ = *read;
          escaped  = false;
          if (flags & flags_dos_catch) {
            was_escaped = true;
          }
        } else {
          // Found match, stop decoding
          value_end = true;
        }
      } else if (! (flags & flags_no_escape) && (*read == '\\')) {
        escaped = true;
      } else {
        if (escaped) {
          *write++ = '\\';
          escaped  = false;
        }
        // Do the DOS trick if eof met and escape found
        if (*read == 0) {
          if (was_escaped) {
            write--;
            *write++ = '\\';
          } else {
            ended_well = false;
          }
          value_end = true;
        } else {
          *write++ = *read;
        }
      }
    } else if (skip_delims || (quote == 0)) {
      bool delim = false;
      for (const char* c = delims; *c != '\0'; c++) {
        if (*c == *read) {
          delim = true;
          break;
        }
      }
      if (quote == 0) {
        // Decoding unquoted string
        if (delim) {
          // Found blank, stop decoding
          value_end = true;
          if ((flags & flags_empty_params) != 0) {
            // Delimiter found, data follows
            decode_next = true;
          }
        } else {
          if (*read == 0) {
            value_end = true;
          } else {
            // Take character into account for parameter
            *write++ = *read;
          }
        }
      } else if (! delim) {
        // No more blanks, check for comment
        skip_delims   = false;
        check_comment = true;
        // Do not increment, as this is the first non-delimiting character
        no_increment = true;
      }
    } else {
      // Start decoding new string
      write = param;
      quote = 0;
      for (const char* c = quotes; *c != '\0'; c++) {
        if (*c == *read) {
          quote = *c;
          break;
        }
      }
      if (quote == 0) {
        // Do not increment,as this is no quote and needs to be used
        no_increment = true;
      }
      decode_next = false;
    }
    if (value_end) {
      *write++ = '\0';
      params.push_back(string(param));
      if ((max_params > 0) && (params.size () >= max_params)) {
        // Reached required number of parameters
        break;
      }
      if (flags & flags_empty_params) {
        // Do not skip blanks but check for comment
        check_comment = true;
      } else {
        // Skip any blank on the way
        skip_delims = true;
      }
      quote       = -1;
      value_end   = false;
    }
  } while (*read != '\0');

  free(param);
  if (! ended_well) {
    return 1;
  }
  return 0;
}
