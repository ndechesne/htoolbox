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
    while ((length = getline(&buffer, &buflen, fd)) >= 0) {
      if (length == 0) {
        continue;
      }
      buffer[length - 1] = '\0';
      char type;
      switch (buffer[0]) {
        case ' ': // controlled
          continue;
        case 'D': // deleted
          type = 'd';
          break;
        case 'I': // ignored
          type = 'i';
          break;
        case '?': // other
          type = 'o';
          break;
        case 'A': // added
        default:  // modified/conflict/etc... => controlled and not up-to-date
          type = 'm';
      }
      // Add to list of Nodes, with status
      _files.push_back(Node(&buffer[7], type, 0, 0, 0, 0, 0));
      if (verbosity() > 1) {
        cout << " --> " << _files.back().name() << "\t" << _files.back().type()
          << endl;
      }
    }
    pclose(fd);
    _files.sort();
  }
}

bool SvnParser::ignore(const Node& node) {
  // Do not ignore control directory
  if ((node.type() == 'd') && (strcmp(node.name(), &control_dir[1]) == 0)) {
    return false;
  }

  // Look for match in list
  bool file_controlled = true;
  bool file_modified   = false;
  for (_i = _files.begin(); _i != _files.end(); _i++) {
    // Find file
    if (! strcmp(_i->name(), node.name())) {
      if (_i->type() == 'm') {
        file_modified   = true;
      } else {
        file_controlled = false;
      }
      break;
    }
  }

  // Deal with result
  switch (_mode) {
    case Parser::controlled:
      if (file_controlled) {
        return false;
      }
      break;
    case Parser::modified:
      // Directories under control need to get parsed
      if (file_modified || (file_controlled && (node.type() == 'd'))) {
        return false;
      }
      break;
    case Parser::modifiedandothers:
      // Directories under control need to get parsed
      if (file_modified || (file_controlled && (node.type() == 'd'))
       || ! file_controlled) {
        return false;
      }
      break;
    case Parser::others:
      if (! file_controlled) {
        return false;
      }
      break;
    default:
      return false;
  }
  return true;
}

void SvnParser::list() {
  cout << name() << " list: " << _files.size() << " file(s)" << endl;
  for (_i = _files.begin(); _i != _files.end(); _i++) {
    cout << "-> " << _i->name() << " (" << _i->type() << ")" << endl;
  }
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
