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

#ifndef CONDITIONS_H
#define CONDITIONS_H

namespace hbackup {

/* The filter stores a list of rules, each containing a list of conditions.
 * A rule matches if all conditions in it match (AND)
 *    rule = condition AND condition AND ... AND condition
 * A match is obtained if any rule matches (OR)
 *    result = rule OR rule OR ... OR rule
 * True is returned when a match was found.
 */

// Stub
class Filter;

class Condition {
public:
  enum Type {
    subfilter   = 1,    //!< subfilter
    type        = 11,   //!< file type
    name        = 21,   //!< exact file name
    name_start,         //!< start of file name
    name_end,           //!< end of file name
    name_regex,         //!< regular expression on file name
    path        = 31,   //!< exact path
    path_start,         //!< start of path
    path_end,           //!< end of path
    path_regex,         //!< regular expression on path
    size_ge     = 41,   //!< minimum size (only applies to regular files)
    size_gt,            //!< minimum size (only applies to regular files)
    size_le,            //!< maximum size (only applies to regular files)
    size_lt,            //!< maximum size (only applies to regular files)
    mode_and    = 51,   //!< mode contains some of the given mode bits
    mode_eq,            //!< mode contains all of the given mode bits
  };
private:
  Type              _type;
  bool              _negated;
  const Filter*     _filter;
  char              _file_type;
  long long         _value;
  string            _string;
public:
  // Sub-filter type-based condition
  Condition(
    Type            type,
    const Filter*   filter,
    bool            negated) :
      _type(type), _negated(negated), _filter(filter) {}
  // File type-based condition
  Condition(
    Type            type,
    char            file_type,
    bool            negated) :
      _type(type), _negated(negated), _file_type(file_type) {}
  // Size-based condition
  Condition(
    Type            type,
    mode_t          value,
    bool            negated) :
      _type(type), _negated(negated), _value(value) {}
  Condition(
    Type            type,
    long long       value,
    bool            negated) :
      _type(type), _negated(negated), _value(value) {}
  // Path-based condition
  Condition(
    Type            type,
    const string&   str,
    bool            negated) :
      _type(type), _negated(negated), _string(str) {}
  bool match(
    const char*     path,
    const Node&     node) const;
  /* Debug only */
  void show() const;
};

}

#endif
