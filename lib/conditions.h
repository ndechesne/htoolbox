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

/* Filter types */
typedef enum {
  condition_subfilter   = 0,  // Subfilter
  condition_type        = 1,  // File type
  condition_name        = 11, // Exact file name
  condition_name_start,       // Start of file name
  condition_name_end,         // End of file name
  condition_name_regex,       // Regular expression on file name
  condition_path        = 21, // Exact path
  condition_path_start,       // Start of path
  condition_path_end,         // End of path
  condition_path_regex,       // Regular expression on path
  condition_size_ge     = 31, // Minimum size (only applies to regular files)
  condition_size_gt,          // Minimum size (only applies to regular files)
  condition_size_le,          // Maximum size (only applies to regular files)
  condition_size_lt,          // Maximum size (only applies to regular files)
  condition_mode_and    = 41, // Mode contains some of the given mode bits
  condition_mode_eq,          // Mode contains all of the given mode bits
} condition_type_t;

// Stub
class Filter2;

class Condition {
  condition_type_t  _type;
  bool              _negated;
  const Filter2*    _filter;
  char              _file_type;
  long long         _value;
  string            _string;
public:
  // Sub-filter type-based condition
  Condition(condition_type_t type, const Filter2* filter, bool negated) :
    _type(type), _negated(negated), _filter(filter) {}
  // File type-based condition
  Condition(condition_type_t type, char file_type, bool negated) :
    _type(type), _negated(negated), _file_type(file_type) {}
  // Size-based condition
  Condition(condition_type_t type, mode_t value, bool negated) :
    _type(type), _negated(negated), _value(value) {}
  Condition(condition_type_t type, long long value, bool negated) :
    _type(type), _negated(negated), _value(value) {}
  // Path-based condition
  Condition(condition_type_t type, const string& str, bool negated) :
    _type(type), _negated(negated), _string(str) {}
  bool match(const char* path, const Node& node) const;
  /* Debug only */
  void show() const;
};

}

#endif
