/*
     Copyright (C) 2008  Herve Fache

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
#include <sstream>
#include <string>
#include <list>

using namespace std;

#include <files.h>
#include <configuration.h>

using namespace hbackup;

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

void ConfigLine::clear() {
  list<ConfigLine*>::iterator i;
  for (i = _children.begin(); i != _children.end(); i = _children.erase(i)) {
    delete *i;
  }
}

void ConfigLine::debug(int level) const {
  printf("%2u:", lineNo());
  for (int j = 0; j < level; j++) {
    cout << " ";
  }
  for (unsigned int j = 0; j < size(); j++) {
    if (j != 0) {
      cout << " ";
    }
    cout << (*this)[j];
  }
  cout << endl;
}

bool ConfigCounter::operator<(const ConfigCounter& counter) const {
  return _keyword < counter._keyword;
}

bool ConfigError::operator<(const ConfigError& error) const {
  return _line_no < error._line_no;
}

void ConfigError::print() const {
  cerr << "Error: ";
  if (_line_no >= 0) {
    if (_type != 0) {
      cerr << "after";
    } else {
      cerr << "at";
    }
  }
  cerr << " line " << _line_no << ": " << _message << endl;
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
    list<ConfigError>&        errors,
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
      errors.push_back(ConfigError(message.str(), line_no, 1));
    }
  }
  return is_valid;
}

void ConfigItem::debug(int level) const {
  list<ConfigItem*>::const_iterator i;
  for (i = _children.begin(); i != _children.end(); i++) {
    for (int j = 0; j < level; j++) {
      cout << " ";
    }
    cout << (*i)->_keyword;
    cout << ", occ.: ";
    if ((*i)->_min_occurrences != 0) {
      cout << "min = " << (*i)->_min_occurrences;
    } else {
      cout << "optional";
    }
    cout << ", ";
    if ((*i)->_max_occurrences != 0) {
      cout << "max = " << (*i)->_max_occurrences;
    } else {
      cout << "no max";
    }
    cout << "; params: ";
    if ((*i)->_min_params != 0) {
      cout << "min params = " << (*i)->_min_params;
    } else {
      cout << "no min";
    }
    cout << ", ";
    if ((*i)->_max_params != 0) {
      cout << "max params = " << (*i)->_max_params;
    } else {
      cout << "no max";
    }
    cout << endl;
    (*i)->debug(level + 2);
  }
}

int Config::read(
    Stream&         stream) {
  // Where we are in the items tree
  list<const ConfigItem*> items_hierarchy;
  items_hierarchy.push_back(&_items_top);

  // Where are we in the lines tree
  list<ConfigLine*> lines_hierarchy;
  lines_hierarchy.push_back(&_lines_top);

  // Where we accumulate the errors, to then output them in line no order
  list<ConfigError> errors;

  // Read through the file
  ConfigLine *params = new ConfigLine;
  int line_no = 0;
  bool failed = false;
  int rc;
  while ((rc = stream.getParams(*params)) >= 0) {
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
          if (((params->size() - 1) < child->min_params())
              || ((params->size() - 1) > child->max_params())) {
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
            errors.push_back(ConfigError(message.str(), line_no));
            failed = true;
          }
          // Prepare new config line
          params = new ConfigLine;
          break;
        } else {
          // Compute children occurrencies
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
            failed  = true;
          }
          // Keyword not found in children, go up the tree
          items_hierarchy.pop_back();
          lines_hierarchy.pop_back();
          if ((items_hierarchy.size() == 0) && (rc != 0)) {
            errors.push_back(ConfigError("keyword '" + (*params)[0]
              + "' incorrect or misplaced, aborting", line_no));
            failed = true;
            goto end;
          }
        }
      }
    }
    if (rc == 0) break;
  }
end:
  errors.sort();
  for (list<ConfigError>::const_iterator i = errors.begin(); i != errors.end();
      i++) {
    i->print();
  }
  delete params;
  return failed ? -1 : 0;
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

void Config::debug() const {
  cout << "Items:" << endl;
  _items_top.debug(2);
  
  cout << "Lines:" << endl;
  ConfigLine* params;
  int level;
  while ((level = line(&params)) >= 0) {
    params->debug((level + 1) * 2);
  }
}
