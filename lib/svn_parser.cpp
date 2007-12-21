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

#include <fstream>
#include <iostream>
#include <string>
#include <list>
#include <sys/stat.h>

using namespace std;

#include "strings.h"
#include "files.h"
#include "parsers.h"
#include "svn_parser.h"
#include "hbackup.h"

using namespace hbackup;

static const string control_dir = "/.svn";
static const char* entries = "/entries";

string SvnParser::name() const {
  return "Subversion";
}

Parser *SvnParser::isControlled(const string& dir_path) const {
  // Parent under control, this is the control directory
  if (! _dummy
   && (dir_path.size() > control_dir.size())
   && (dir_path.substr(dir_path.size() - control_dir.size()) == control_dir)) {
    return new SvnControlParser;
  }

  // If control directory exists and contains an entries file, assume control
  if (! File((dir_path + control_dir).c_str(), &entries[1]).isValid()) {
    if (! _dummy) {
      cerr << "Directory should be under " << name() << " control: "
        << dir_path << endl;
      return new IgnoreParser;
    } else {
      return NULL;
    }
  } else {
    return new SvnParser(_mode, dir_path);
  }
}

SvnParser::SvnParser(Mode mode, const string& dir_path) {
  // Save mode
  _mode = mode;

  /* Fill in list of files */
  if (verbosity() > 1) {
    cout << " -> Parsing Subversion entries" << endl;
  }
  string command = "svn status --no-ignore --non-interactive -N " + dir_path;
  FILE* fd = popen(command.c_str(), "r");
  if (fd != NULL) {
    char*   buffer = NULL;
    size_t  buflen = 0;
    ssize_t length;
    while ((length = getline(&buffer, &buflen, fd)) != -1) {
      // TODO: add to list of Nodes, with some status
      if (verbosity() > 1) {
        cout << " --> " << buffer[0] << ": " << &buffer[7] << endl;
      }
    }
    pclose(fd);
  }
}

bool SvnParser::ignore(const Node& node) {
  // TODO: implement it!
  return false;
}

string SvnControlParser::name() const {
  return "Subversion Control";
}

bool SvnControlParser::ignore(const Node& node) {
  if (strcmp(node.name(), "entries") == 0) {
    return false;
  }
  return true;
}
