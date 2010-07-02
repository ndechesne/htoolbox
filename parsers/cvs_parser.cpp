/*
     Copyright (C) 2006-2010  Herve Fache

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

#include <sstream>
#include <list>
#include <sys/stat.h>

using namespace std;

#include "cvs_parser.h"

using namespace hbackup;
using namespace hreport;

static const string control_dir = "/CVS";
static const char* entries = "/Entries";

// Parser for CVS control directory 'CVS'
class CvsControlParser : public Parser {
public:
  // Just to know the parser used
  const char* name() const { return "CVS Control"; }
  const char* code() const { return "cvs_c"; }
  // This directory has no controlled children
  Parser* createChildIfControlled(const string& dir_path) const {
    (void) dir_path;
    return new IgnoreParser;
  }
  // That tells use whether to ignore the file, i.e. not back it up
  bool ignore(const Node& node) const {
    if ((strcmp(node.name(), "Entries") == 0)
    || (strcmp(node.name(), "Root") == 0)) {
      return false;
    }
    return true;
  }
};

Parser *CvsParser::createChildIfControlled(const string& dir_path) const {
  // Parent under control, this is the control directory
  if (! _no_parsing
   && (dir_path.size() > control_dir.size())
   && (dir_path.substr(dir_path.size() - control_dir.size()) == control_dir)) {
    return new CvsControlParser;
  }

  // If control directory exists and contains an entries file, assume control
  if (! File(Path((dir_path + control_dir).c_str(), &entries[1])).isValid()) {
    if (! _no_parsing) {
      out(warning, msg_standard, "Directory should be under CVS control", -1,
        dir_path.c_str());
      return new IgnoreParser;
    } else {
      return NULL;
    }
  } else {
    return new CvsParser(_mode, dir_path);
  }
}

CvsParser::CvsParser(Mode mode, const string& dir_path) : Parser(mode, dir_path) {
  if (dir_path == "") {
    return;
  }
  string path = dir_path + control_dir + entries;
  Stream entries_file(path.c_str());

  // Save mode
  _mode = mode;

  /* Fill in list of controlled files */
  entries_file.open(O_RDONLY);
  out(debug, msg_standard, "Parsing CVS entries", 1, NULL);
  int    line_no = 0;
  char*  buffer = NULL;
  size_t buffer_capacity = 0;
  while (entries_file.getLine(&buffer, &buffer_capacity) > 0) {
    const char* reader = buffer;
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
      // End of line check should go once getLine is fixed
      if ((*reader == '\0') || (*reader == '\n')) {
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
      out(error, msg_number,
        "Can't find first delimiter parsing CVS entries file", line_no,
        dir_path.c_str());
      continue;
    }
    reader++;
    // Get name
    pos = strchr(reader, '/');
    if (pos == NULL) {
      out(error, msg_number,
        "Can't find second delimiter parsing CVS entries file", line_no,
        dir_path.c_str());
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
      out(error, msg_number,
        "Can't find third delimiter parsing CVS entries file", line_no,
        dir_path.c_str());
        continue;
      }
      // Get date
      reader = pos + 1;
      pos = strchr(reader, '/');
      if (pos == NULL) {
      out(error, msg_number,
        "Can't find fourth delimiter parsing CVS entries file", line_no,
        dir_path.c_str());
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
  }
  free(buffer);
  /* Close file */
  entries_file.close();
  _files.sort();
  show(2);
}

bool CvsParser::ignore(const Node& node) const {
  // Do not ignore control directory
  if ((node.type() == 'd') && (strcmp(node.name(), &control_dir[1]) == 0)) {
    return false;
  }

  // Look for match in list
  bool file_controlled = false;
  bool file_modified   = false;
  list<Node>::const_iterator  i;
  for (i = _files.begin(); i != _files.end(); i++) {
    if (! strcmp(i->name(), node.name()) && (i->type() == node.type())) {
      file_controlled = true;
      if (i->mtime() != node.mtime()) {
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

void CvsParser::show(int level) {
  list<Node>::iterator  i;
  for (i = _files.begin(); i != _files.end(); i++) {
    if (i->type() == 'd') {
      out(debug, msg_standard, "D", level, i->name());
    } else {
      stringstream mtime;
      mtime << i->mtime();
      out(debug, msg_standard, mtime.str().c_str(), level, i->name());
    }
  }
}

