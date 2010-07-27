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

#ifndef _ATTRIBUTES_H
#define _ATTRIBUTES_H

namespace hbackup {

class Attributes : ConfigObject {
  bool                _report_copy_error_once;
  Filters             _filters;
  list<const Filter*> _ignore_list;
  Attributes& operator=(const Attributes& a);
public:
  Attributes()
    : _report_copy_error_once(false), _filters(NULL) {}
  Attributes(const Attributes& attributes) : _filters(&attributes.filters()) {
    _report_copy_error_once = attributes._report_copy_error_once;
    _ignore_list = attributes._ignore_list;
  }
  bool reportCopyErrorOnceIsSet() const { return _report_copy_error_once; }
  const Filters& filters() const        { return _filters; }
  void setReportCopyErrorOnce()         { _report_copy_error_once = true; }
  virtual ConfigObject* configChildFactory(
    const vector<string>& params,
    const char*           file_path = NULL,
    size_t                line_no   = 0);
  Filter* addFilter(
    const vector<string>& params,
    const char*           file_path = NULL,
    size_t                line_no   = 0) {
    return _filters.configChildFactory(params, file_path, line_no);
  }
  Condition* addFilterCondition(
    const vector<string>& params,
    const char*           file_path = NULL,
    size_t                line_no   = 0) {
    return _filters.addCondition(params, file_path, line_no);
  }
  void addIgnore(const Filter* filter)  { _ignore_list.push_back(filter); }
  bool mustBeIgnored(const Node& node, size_t start = 0) const {
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
      hlog_debug_arrow(level, "No error if same file fails copy again");
    }
    _filters.show(level);
    if (_ignore_list.size() > 0) {
      hlog_debug_arrow(level, "Ignore filters:");
      for (list<const Filter*>::const_iterator filter = _ignore_list.begin();
          filter != _ignore_list.end(); filter++) {
        hlog_debug_arrow(level + 1, "%s", (*filter)->name().c_str());
      }
    }
  }
};

}

#endif
