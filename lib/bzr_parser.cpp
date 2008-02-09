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

#include <iostream>
#include <string>
#include <list>
#include <sys/stat.h>

using namespace std;

#include "files.h"
#include "parsers.h"
#include "bzr_parser.h"
#include "hbackup.h"

using namespace hbackup;

static const string control_dir = "/.bzr";
static const char* format = "/branch-format";

string BzrParser::name() const {
  return "Bazaar";
}

Parser *BzrParser::isControlled(const string& dir_path) const {
  // Parent under control, this is the control directory
  if (! _dummy
   && (dir_path.size() > control_dir.size())
   && (dir_path.substr(dir_path.size() - control_dir.size()) == control_dir)) {
    return new BzrControlParser;
  }

  // If control directory exists and contains an format file, assume control
  if (! File(Path((dir_path + control_dir).c_str(), &format[1])).isValid()) {
    return NULL;
  } else {
    return new BzrParser(_mode, dir_path);
  }
}

BzrParser::BzrParser(Mode mode, const string& dir_path) {}

bool BzrParser::ignore(const Node& node) {
  return false;
}

void BzrParser::list() {}

string BzrControlParser::name() const {
  return "Bazaar Control";
}

bool BzrControlParser::ignore(const Node& node) {
  return true;
}
