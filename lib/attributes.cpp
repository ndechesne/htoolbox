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

#include <list>
#include <string>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "hreport.h"
#include "configuration.h"
#include "conditions.h"
#include "filters.h"
#include "parsers.h"
#include "attributes.h"

using namespace hbackup;
using namespace hreport;

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
  {
    hlog_error("%s:%zd unknown keyword '%s'",
      file_path, line_no, keyword.c_str());
  }
  return co;
}
