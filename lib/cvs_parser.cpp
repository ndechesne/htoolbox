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
#include "cvs_parser.h"
#include "hbackup.h"

using namespace hbackup;

static const string control_dir = "/CVS";
static const char* entries = "/Entries";

string CvsParser::name() const {
  return "CVS";
}

Parser *CvsParser::isControlled(const string& dir_path) const {
  // Parent under control, this is the control directory
  if (! _dummy
   && (dir_path.size() > control_dir.size())
   && (dir_path.substr(dir_path.size() - control_dir.size()) == control_dir)) {
    return new CvsControlParser;
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
    return new CvsParser(_mode, dir_path);
  }
}

CvsParser::CvsParser(Mode mode, const string& dir_path) {
  string    path = dir_path + control_dir + entries;
  ifstream  entries_file(path.c_str());

  // Save mode
  _mode = mode;

  /* Fill in list of controlled files */
  if (verbosity() > 1) {
    cout << " -> Parsing CVS entries" << endl;
  }
  int line_no = 0;
  while (! entries_file.eof()) {
    string  buffer;
    getline(entries_file, buffer);
    const char* reader = buffer.c_str();
    const char* pos;

    // Empty line
    if (*reader == '\0') {
      continue;
    }
    line_no++;
    // Determine type
    char type;
    if (*reader == 'D') {
      reader++;
      if (*reader == '\0') {
        // If a directory contains no subdirectories, the last line of the
        // entries file is a single 'D'
        continue;
      }
      type = 'd';
    } else {
      type = 'f';
    }
    // We expect a separator
    if (*reader != '/') {
      cerr << "Error parsing CVS entries file for " << dir_path
        << ", line " << line_no << ": can't find first delimiter" << endl;
      continue;
    }
    reader++;
    // Get name
    pos = strchr(reader, '/');
    if (pos == NULL) {
      cerr << "Error parsing CVS entries file for " << dir_path
        << ", line " << line_no << ": can't find second delimiter" << endl;
      continue;
    }
    string name = reader;
    name.erase(pos - reader);
    time_t mtime;
    if (type == 'f') {
      // Skip version
      reader = pos + 1;
      pos = strchr(reader, '/');
      if (pos == NULL) {
        cerr << "Error parsing CVS entries file for " << dir_path
          << ", line " << line_no << ": can't find third delimiter" << endl;
        continue;
      }
      // Get date
      reader = pos + 1;
      pos = strchr(reader, '/');
      if (pos == NULL) {
        cerr << "Error parsing CVS entries file for " << dir_path
          << ", line " << line_no << ": can't find fourth delimiter" << endl;
        continue;
      }
      string date = reader;
      date.erase(pos - reader);
      // Decode the date
      struct tm tm;
      char* rc = strptime(date.c_str(), "%a %b %d %H:%M:%S %Y", &tm);
      if ((rc == NULL) || (*rc != '\0')) {
        mtime = 0;
      } else {
        char *tz = getenv("TZ");

        setenv("TZ", "", 1);
        tzset();
        mtime = mktime(&tm);
        if (tz) {
          setenv("TZ", tz, 1);
        } else {
          unsetenv("TZ");
        }
        tzset();
      }
    } else {
      mtime = 0;
    }
    // Enlist data
    _files.push_back(Node(name.c_str(), type, mtime, 0, 0, 0, 0));
    if (verbosity() > 1) {
      cout << " --> " << type << " " << name << " " << mtime << endl;
    }
  }
  /* Close file */
  entries_file.close();
}

bool CvsParser::ignore(const Node& node) {
  // Do not ignore control directory
  if ((node.type() == 'd') && (strcmp(node.name(), &control_dir[1]) == 0)) {
    return false;
  }

  // Look for match in list
  bool file_controlled = false;
  bool file_modified   = false;
  for (_i = _files.begin(); _i != _files.end(); _i++) {
    if (! strcmp(_i->name(), node.name()) && (_i->type() == node.type())) {
      file_controlled = true;
      if (_i->mtime() != node.mtime()) {
        file_modified = true;
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
      if (file_modified) {
        return false;
      }
      break;
    case Parser::modifiedandothers:
      if (file_modified || ! file_controlled) {
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

void CvsParser::list() {
  cout << "List: " << _files.size() << " file(s)" << endl;
  for (_i = _files.begin(); _i != _files.end(); _i++) {
    cout << "-> " << _i->name() << " (";
    if (_i->type() == 'd') {
      cout << "D";
    } else {
      cout << _i->mtime();
    }
    cout << ")" << endl;
  }
}

string CvsControlParser::name() const {
  return "CVS Control";
}

bool CvsControlParser::ignore(const Node& node) {
  if ((strcmp(node.name(), "Entries") == 0)
   || (strcmp(node.name(), "Root") == 0)) {
    return false;
  }
  return true;
}
