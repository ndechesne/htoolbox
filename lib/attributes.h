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

#ifndef _ATTRIBUTES_H
#define _ATTRIBUTES_H

namespace hbackup {

class Attributes {
  bool                _report_copy_error_once;
  Filters             _filters;
  Filter*             _last_filter;
  list<const Filter*> _ignore_list;
  Attributes(const Attributes&);
public:
  Attributes() : _report_copy_error_once(false), _last_filter(NULL) {}
  const Attributes& operator=(const Attributes& attr) {
    _report_copy_error_once = attr._report_copy_error_once;
    _last_filter            = NULL;
    _ignore_list            = attr._ignore_list;
    return *this;
  }
  bool reportCopyErrorOnceIsSet() const { return _report_copy_error_once; }
  const Filters& filters() const        { return _filters;                }
  void setReportCopyErrorOnce()         { _report_copy_error_once = true; }
  Filter* addFilter(const string& type, const string& name) {
    _last_filter = _filters.add(type, name);
    return _last_filter;
  }
  void addFilterCondition(Condition* condition) {
    _last_filter->add(condition);
  }
  int addFilterCondition(const string& type, const char* value, bool negated) {
    return _last_filter->add(type, value, negated);
  }
  Filter* findFilter(const string& name) const {
    return _filters.find(name);
  }
  void addIgnore(const Filter* filter)  { _ignore_list.push_back(filter); }
  bool mustBeIgnored(const Node& node, int start = 0) const {
    for (list<const Filter*>::const_iterator filter = _ignore_list.begin();
        filter != _ignore_list.end(); filter++) {
      if ((*filter)->match(node, start)) {
        return true;
      }
    }
    return false;
  }
  void show(int level = 0) const {
    if (_report_copy_error_once) {
      out(debug, msg_standard, "No error if same file fails copy again",
        level);
    }
    _filters.show(level);
    if (_ignore_list.size() > 0) {
      out(debug, msg_standard, "Ignore filters:", level);
      for (list<const Filter*>::const_iterator filter = _ignore_list.begin();
          filter != _ignore_list.end(); filter++) {
        out(debug, msg_standard, (*filter)->name().c_str(), level + 1);
      }
    }
  }
};

}

#endif
