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

#ifndef FILTERS_H
#define FILTERS_H

namespace hbackup {

// Each filter can be declared as a logical AND or OR of its conditions
// A special condition runs a filter, so expressions of any complexity can be
// defined. This is why the name is needed and must be unique.
class Filter : public list<Condition*> {
public:
  enum Mode {
    any = 1,            //!< matches if at least one condition matches
    all                 //!< matches if all of its conditions match
  };
private:
  Mode              _type;
  string            _name;
public:
  Filter(
    Mode            type,
    const char*     name) :
      _type(type), _name(name) {}
  ~Filter();
  const string& name() const     { return _name; }
  void add(Condition* condition) { push_back(condition); }
  int add(
    const string&   type,
    const char*     value,
    bool            negated);
  bool match(
    const Node&     node,
    int             start = 0) const;
  /* For verbosity */
  void show(int level = 0) const;
};

class Filters : public list<Filter*> {
public:
  ~Filters();
  Filter* find(
    const string&   name) const;
  Filter* add(
    const string&   type,
    const string&   name);
  /* For verbosity */
  void show(int level = 0) const;
};

}

#endif
