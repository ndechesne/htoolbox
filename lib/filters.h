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

#ifndef FILTERS_H
#define FILTERS_H

namespace hbackup {

/* The filter stores a list of rules, each containing a list of conditions.
 * A rule matches if all conditions in it match (AND)
 *    rule = condition AND condition AND ... AND condition
 * A match is obtained if any rule matches (OR)
 *    result = rule OR rule OR ... OR rule
 * True is returned when a match was found.
 */

class Filter: public list<Condition> {
public:
  Filter() {}
  Filter(Condition condition) {
    push_back(condition);
  }
};

class Filters: public list<Filter> {
public:
  bool match(const char* path, const Node& node) const;
};

// This new class will eventually replace the old one
// Each filter can be declared as a logical AND or OR of its conditions
// A special condition runs a filter, so expressions of any complexity can be
// defined. This is why the name is needed and must be unique.
// Note: the tag is only application defined
typedef enum {
  filter_or         = 1,      // Matches if at least one condition matches
  filter_and,                 // Matches if all of its conditions match
} filter_type_t;

class Filter2 {
  filter_type_t     _type;
  string            _name;
  int               _tag;
  list<Condition*>  _conditions;
public:
  Filter2(filter_type_t type, const char* name, int tag = 0) :
    _type(type), _name(name), _tag(tag) {}
  ~Filter2();
  const string& name() const     { return _name; }
  void add(Condition* condition) { _conditions.push_back(condition); }
  bool match(const char* path, const Node& node) const;
};

}

#endif
