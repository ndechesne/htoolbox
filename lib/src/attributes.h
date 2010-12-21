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

#include <files.h>

namespace hbackup {

class Attributes : htools::Config::Object {
  const Attributes*   _parent;
  bool                _report_copy_error_once;
  htools::Path*       _tree_path;
  bool                _tree_symlinks;
  bool                _check_data;
  bool                _compressed_data;
  size_t              _hourly;
  size_t              _daily;
  size_t              _weekly;
  Filters             _filters;
  list<const Filter*> _ignore_list;
  Attributes& operator=(const Attributes& a);
public:
  Attributes()
    : _parent(NULL),
      _report_copy_error_once(false),
      _tree_path(NULL),
      _check_data(false),
      _compressed_data(false),
      _filters(NULL) {}
  Attributes(const Attributes& attributes)
    : _parent(&attributes),
      _report_copy_error_once(false),
      _tree_path(NULL),
      _check_data(attributes._check_data),
      _compressed_data(attributes._compressed_data),
      _filters(&attributes.filters()) {
    _ignore_list = attributes._ignore_list;
  }
  ~Attributes() {
    delete _tree_path;
  }
  bool reportCopyErrorOnceIsSet() const {
    const Attributes* attributes = this;
    do {
      if (attributes->_report_copy_error_once) {
        return true;
      }
      attributes = attributes->_parent;
    } while (attributes != NULL);
    return false;
  }
  bool treeIsSet() const          { return _tree_path != NULL; }
  const char* tree() const        {
    if (_tree_path != NULL) {
      return _tree_path->c_str();
    } else {
      return NULL;
    }
  }
  bool treeSymlinks() const       { return _tree_symlinks; }
  bool treeCheckData() const      { return _check_data; }
  bool treeCompressedData() const { return _compressed_data; }
  const Filters& filters() const  { return _filters; }
  void setReportCopyErrorOnce()   { _report_copy_error_once = true; }
  virtual Object* configChildFactory(
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
  bool mustBeIgnored(const htools::Node& node, size_t start = 0) const {
    for (list<const Filter*>::const_iterator filter = _ignore_list.begin();
        filter != _ignore_list.end(); filter++) {
      if ((*filter)->match(node, start)) {
        return true;
      }
    }
    return false;
  }
  void show(int level = 0) const;
};

}

#endif
