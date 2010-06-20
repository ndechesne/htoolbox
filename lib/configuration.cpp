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

#include <sstream>
#include <string>
#include <list>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "hreport.h"
#include <configuration.h>

using namespace hbackup;
using namespace hreport;

namespace hbackup {

// To check occurrences
class ConfigCounter {
  const string&     _keyword;
  int               _occurrences;
public:
  ConfigCounter(const string& keyword) : _keyword(keyword), _occurrences(1) {}
  bool operator<(const ConfigCounter& counter) const;
  const string& keyword() const { return _keyword;    }
  int occurrences()        const { return _occurrences; }
  void increment()              { _occurrences++;      }
};

}

ConfigLine::~ConfigLine() {
  clear();
}

void ConfigLine::add(ConfigLine* child) {
  list<ConfigLine*>:: iterator i = _children.begin();
  while (i != _children.end()) {
    if ((**i)[0] > (*child)[0]) break;
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
  for (int j = 0; j < level; j++) {
    s << " ";
  }
  for (unsigned int j = 0; j < size(); j++) {
    if (j != 0) {
      s << " ";
    }
    s << (*this)[j];
  }
  out(debug, msg_number, s.str().c_str(), lineNo());
}

bool ConfigCounter::operator<(const ConfigCounter& counter) const {
  return _keyword < counter._keyword;
}

bool ConfigError::operator<(const ConfigError& error) const {
  return _line_no < error._line_no;
}

void ConfigError::show() const {
  stringstream s;
  if (_line_no >= 0) {
    if (_type != 0) {
      s << "after";
    } else {
      s << "at";
    }
    s << " line";
  }
  out(error, msg_number, _message.c_str(), _line_no, s.str().c_str());
}

ConfigItem::~ConfigItem() {
  list<ConfigItem*>::iterator i;
  for (i = _children.begin(); i != _children.end(); i = _children.erase(i)) {
    delete *i;
  }
  free(_keyword);
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
    unsigned int occurrences = 0;
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
    stringstream s;
    for (int j = 0; j < level; j++) {
      s << " ";
    }
    s << (*i)->_keyword;
    s << ", occ.: ";
    if ((*i)->_min_occurrences != 0) {
      s << "min = " << (*i)->_min_occurrences;
    } else {
      s << "optional";
    }
    s << ", ";
    if ((*i)->_max_occurrences != 0) {
      s << "max = " << (*i)->_max_occurrences;
    } else {
      s << "no max";
    }
    s << "; params: ";
    if ((*i)->_min_params != (*i)->_max_params) {
      s << "min = " << (*i)->_min_params;
      s << ", ";
      if ((*i)->_max_params > (*i)->_min_params) {
        s << "max = " << (*i)->_max_params;
      } else {
        s << "no max";
      }
    } else {
      s << "min = max = " << (*i)->_min_params;
    }
    out(verbose, msg_standard, s.str().c_str());
    (*i)->show(level + 2);
  }
}

int Config::read(
    Stream&         stream,
    unsigned char   flags,
    ConfigSyntax&   syntax,
    ConfigErrors*   errors) {
  // Where we are in the items tree
  list<const ConfigItem*> items_hierarchy;
  items_hierarchy.push_back(&syntax);

  // Where are we in the lines tree
  list<ConfigLine*> lines_hierarchy;
  lines_hierarchy.push_back(&_lines_top);

  // Read through the file
  ConfigLine *params = new ConfigLine;
  int line_no = 0;
  bool failed = false;
  int rc;
  while ((rc = stream.getParams(*params, flags)) >= 0) {
    if (rc == 0) {
      // Force unstacking of elements and final check
      params->push_back("");
    }
    line_no++;
    if (params->size() > 0) {
      // Look for keyword in children of items in the current items hierarchy
      while (items_hierarchy.size() > 0) {
        const ConfigItem* child = items_hierarchy.back()->find((*params)[0]);
        if (child != NULL) {
          // Add under current hierarchy
          items_hierarchy.push_back(child);
          // Add in configuration lines tree, however incorrect it may be
          params->setLineNo(line_no);
          lines_hierarchy.back()->add(params);
          // Add under current hierarchy
          lines_hierarchy.push_back(params);
          // Check number of parameters (params.size() - 1)
          if (   ((params->size() - 1) < child->min_params())
              || ( (child->min_params() <= child->max_params())
                && ((params->size() - 1) > child->max_params()))) {
            if (errors != NULL) {
              ostringstream message;
              message << "keyword '" << (*params)[0]
                << "' requires ";
              if (child->min_params() == child->max_params()) {
                message << child->min_params();
              } else
              if ((params->size() - 1) < child->min_params()) {
                message << "at least " << child->min_params();
              } else
              {
                message << "at most " << child->max_params();
              }
              message << " parameter(s), found " << params->size() - 1;
              errors->push_back(ConfigError(message.str(), line_no));
            }
            failed = true;
          }
          // Prepare new config line
          params = new ConfigLine;
          break;
        } else {
          // Compute children occurrences
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
            failed = true;
          }
          // Keyword not found in children, go up the tree
          items_hierarchy.pop_back();
          lines_hierarchy.back()->sortChildren();
          lines_hierarchy.pop_back();
          if ((items_hierarchy.size() == 0) && (rc != 0)) {
            if (errors != NULL) {
              if ((*params)[0][0] != '\r'){
                errors->push_back(ConfigError("keyword '" + (*params)[0]
                  + "' incorrect or misplaced, aborting", line_no));
              } else {
                errors->push_back(ConfigError("found CR character, aborting",
                  line_no));
              }
            }
            failed = true;
            goto end;
          }
        }
      }
    }
    if (rc == 0) break;
  }
end:
  if (errors != NULL) {
    errors->sort();
  }
  delete params;
  return failed ? -1 : 0;
}

int Config::write(
    Stream&         stream) const {
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
    if (stream.putLine(s.str().c_str()) < 0) {
      return -1;
    }
  }
  return 0;
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
  return parents.size() - 1;
}

void Config::clear() {
  _lines_top.clear();
}

void Config::show() const {
  out(debug, msg_standard, "Config:");
  ConfigLine* params;
  int level;
  while ((level = line(&params)) >= 0) {
    params->show((level + 1) * 2);
  }
}
