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

#ifndef FILTERS_H
#define FILTERS_H

#include "configuration.h"

namespace hbackup {

class Filters;

// Each filter can be declared as a logical AND or OR of its conditions
// A special condition runs a filter, so expressions of any complexity can be
// defined. This is why the name is needed and must be unique.
class Filter : public ConfigObject {
public:
  enum Mode {
    any = 1,            //!< matches if at least one condition matches
    all                 //!< matches if all of its conditions match
  };
private:
  Mode              _type;
  string            _name;
  const Filters*    _parent;
  list<Condition*>  _children;
  Filter();
  Filter(const Filter&);
  const Filter& operator=(const Filter&);
public:
  Filter(
    Mode            type,
    const char*     name,
    const Filters*  parent = NULL) :
      _type(type), _name(name), _parent(parent) {}
  ~Filter();
  const string& name() const     { return _name; }
  void add(Condition* condition) {
    _children.push_back(condition);
  }
  /* this is only to simplify path_test.cpp... for now */
  int add(const string& type, const string& value, bool negated) {
    string type_str;
    if (negated) {
      type_str = "!";
    }
    type_str += type;
    vector<string> params;
    params.push_back("");
    params.push_back(type_str);
    params.push_back(value);
    return configChildFactory(params) == NULL ? -1 : 0;
  }
  virtual Condition* configChildFactory(
    const vector<string>& params,
    const char*           file_path = NULL,
    size_t                line_no   = 0);
  bool match(
    const htools::Node& node,
    size_t              start = 0) const;
  /* For verbosity */
  void show(int level = 0) const;
};

class Filters : public ConfigObject {
  list<Filter*>   _children;
  const Filters*  _parent;
  Filter*         _last_filter;
public:
  Filters(const Filters* parent = NULL) : _parent(parent), _last_filter(NULL) {}
  ~Filters();
  Filter* find(
    const string&   name) const;
  Condition* addCondition(
    const vector<string>& params,
    const char*           file_path = NULL,
    size_t                line_no   = 0) {
    return _last_filter->configChildFactory(params, file_path, line_no);
  }
  virtual Filter* configChildFactory(
    const vector<string>& params,
    const char*           file_path = NULL,
    size_t                line_no   = 0);
  /* For verbosity */
  void show(int level = 0) const;
};

}

#endif
