/*
     Copyright (C) 2010  Herve Fache

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

#include "stdio.h"

#include <list>
#include <string>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "report.h"
#include "configuration.h"
#include "conditions.h"
#include "filters.h"
#include "parsers.h"
#include "attributes.h"

using namespace hbackup;
using namespace htools;

ConfigObject* Attributes::configChildFactory(
    const vector<string>& params,
    const char*           file_path,
    size_t                line_no) {
  ConfigObject* co = NULL;
  const string& keyword = params[0];
  if (keyword == "report_copy_error_once") {
    _report_copy_error_once = true;
    co = this;  // anything but NULL
  } else
  if (keyword == "ignore") {
    Filter* filter = _filters.find(params[1]);
    if (filter == NULL) {
      hlog_error("%s:%zd filter '%s' not found",
        file_path, line_no, params[1].c_str());
    } else {
      _ignore_list.push_back(filter);
      co = this;  // anything but NULL
    }
  } else
  if (keyword == "filter") {
    co = _filters.configChildFactory(params, file_path, line_no);
  } else
  if (keyword == "tree") {
    _tree_path = new Path(params[1].c_str(), Path::NO_TRAILING_SLASHES |
      Path::NO_REPEATED_SLASHES | Path::CONVERT_FROM_DOS);
    _tree_symlinks = false;
    _check_data = false;
    _compressed_data = false;
    _hourly = 1;
    _daily = 0;
    _weekly = 0;
    co = this;
  } else
  if (_tree_path != NULL) {
    if (keyword == "links") {
      if (params[1] == "symbolic") {
        _tree_symlinks = true;
        co = this;
      } else
      if (params[1] == "hard") {
        _tree_symlinks = false;
        co = this;
      } else
      {
        hlog_error("%s:%zd unknown option '%s' for keyword '%s'",
          file_path, line_no, params[1].c_str(), keyword.c_str());
        return NULL;
      }
    } else
    if (keyword == "compressed") {
      if (params[1] == "yes") {
        _check_data = false;
        _compressed_data = true;
        co = this;
      } else
      if (params[1] == "no") {
        _check_data = false;
        _compressed_data = false;
        co = this;
      } else
      if (params[1] == "check") {
        _check_data = true;
        _compressed_data = false;
        co = this;
      } else
      {
        hlog_error("%s:%zd unknown option '%s' for keyword '%s'",
          file_path, line_no, params[1].c_str(), keyword.c_str());
        return NULL;
      }
    } else
    if ((keyword == "hourly") &&
        (sscanf(params[1].c_str(), "%zu", &_hourly) == 1)) {
      co = this;
    } else
    if ((keyword == "daily") &&
        (sscanf(params[1].c_str(), "%zu", &_daily) == 1)) {
      co = this;
    } else
    if ((keyword == "weekly") &&
        (sscanf(params[1].c_str(), "%zu", &_weekly) == 1)) {
      co = this;
    }
  }
  if (co == NULL) {
    hlog_error("%s:%zd unknown keyword '%s'",
      file_path, line_no, keyword.c_str());
  }
  return co;
}

void Attributes::show(int level) const {
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
  if (_tree_path != NULL) {
    hlog_debug_arrow(level, "Trees path: '%s'", _tree_path->c_str());
    hlog_debug_arrow(level + 1, "create %s links",
      _tree_symlinks ? "symbolic" : "hard");
    if (_check_data) {
      hlog_debug_arrow(level + 1, "check for compressed data (very slow)");
    } else {
      hlog_debug_arrow(level + 1, "assume %scompressed data",
        _compressed_data ? "" : "un");
    }
    hlog_debug_arrow(level + 1, "%zu hourly", _hourly);
    hlog_debug_arrow(level + 1, "%zu daily", _daily);
    hlog_debug_arrow(level + 1, "%zu weekly", _weekly);
  }
}
