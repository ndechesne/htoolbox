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

#include <list>
#include <sys/stat.h>

using namespace std;

#include "report.h"
#include "filereaderwriter.h"
#include "linereaderwriter.h"

using namespace htools;

#include "parsers.h"

using namespace hbackup;

static const string control_dir = "/CVS";
static const char* entries = "/Entries";

class CvsParser : public IParser {
  list<Node>            _files;       // Files under control in current dir
public:
  // Constructor
  CvsParser(Mode mode = master, const char* dir_path = "");
  // Tell them who we are
  const char* name() const { return "CVS"; };
  const char* code() const { return "cvs"; };
  // Factory
  IParser* createInstance(Mode mode) { return new CvsParser(mode); }
  // This will create an appropriate parser for the directory if relevant
  IParser* createChildIfControlled(const char* dir_path);
  // That tells use whether to ignore the file, i.e. not back it up
  bool ignore(const Node& node);
  // For debug purposes
  void show(int level = 0);
};

// Parser for CVS control directory 'CVS'
class CvsControlParser : public IParser {
public:
  // Just to know the parser used
  const char* name() const { return "CVS Control"; }
  const char* code() const { return "cvs_c"; }
  // This directory has no controlled children
  IParser* createChildIfControlled(const char* dir_path) {
    (void) dir_path;
    return new IgnoreParser;
  }
  // That tells use whether to ignore the file, i.e. not back it up
  bool ignore(const Node& node) {
    if ((strcmp(node.name(), "Entries") == 0)
    || (strcmp(node.name(), "Root") == 0)) {
      return false;
    }
    return true;
  }
};

IParser *CvsParser::createChildIfControlled(const char* dir_path) {
  // Parent under control, this is the control directory
  size_t dir_path_len = strlen(dir_path);
  if (! _no_parsing && (dir_path_len > control_dir.size()) &&
      (strcmp(&dir_path[dir_path_len - control_dir.size()], control_dir.c_str()) == 0)) {
    return new CvsControlParser;
  }

  // If control directory exists and contains an entries file, assume control
  if (! Node(Path((dir_path + control_dir).c_str(), &entries[1])).isReg()) {
    if (! _no_parsing) {
      hlog_warning("Directory '%s' should be under CVS control", dir_path);
      return new IgnoreParser;
    } else {
      return NULL;
    }
  } else {
    return new CvsParser(_mode, dir_path);
  }
}

CvsParser::CvsParser(Mode mode, const char* dir_path)
    : IParser(mode, dir_path) {
  if (dir_path[0] == '\0') {
    return;
  }
  string path = dir_path + control_dir + entries;
  FileReaderWriter fr(path.c_str(), false);
  LineReaderWriter entries_file(&fr, false);

  // Save mode
  _mode = mode;

  /* Fill in list of controlled files */
  entries_file.open();
  hlog_debug_arrow(1, "Parsing CVS entries");
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
      hlog_error("can't find first delimiter parsing CVS entries file '%s', "
        "at line %d", dir_path, line_no);
      continue;
    }
    reader++;
    // Get name
    pos = strchr(reader, '/');
    if (pos == NULL) {
      hlog_error("can't find second delimiter parsing CVS entries file '%s', "
        "at line %d", dir_path, line_no);
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
        hlog_error("can't find third delimiter parsing CVS entries file '%s', "
          "at line %d", dir_path, line_no);
        continue;
      }
      // Get date
      reader = pos + 1;
      pos = strchr(reader, '/');
      if (pos == NULL) {
        hlog_error("can't find fourth delimiter parsing CVS entries file '%s', "
          "at line %d", dir_path, line_no);
        continue;
      }
      string date = reader;
      date.erase(pos - reader);
      // Decode the date
      struct tm tm;
      memset(&tm, 0, sizeof(struct tm));
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

bool CvsParser::ignore(const Node& node) {
  // Do not ignore control directory
  if ((node.type() == 'd') && (strcmp(node.name(), &control_dir[1]) == 0)) {
    return false;
  }

  // Look for match in list
  bool file_controlled = false;
  bool file_modified   = false;
  list<Node>::const_iterator  i;
  for (i = _files.begin(); i != _files.end(); i++) {
    if ((strcmp(i->name(), node.name()) == 0) && (i->type() == node.type())) {
      file_controlled = true;
      if (i->mtime() != node.mtime()) {
        file_modified = true;
      }
      break;
    }
  }

  // Deal with result
  switch (_mode) {
    case IParser::controlled:
      if (file_controlled) {
        return false;
      }
      break;
    case IParser::modified:
      if (file_modified) {
        return false;
      }
      break;
    case IParser::modifiedandothers:
      if (file_modified || ! file_controlled) {
        return false;
      }
      break;
    case IParser::others:
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
      hlog_debug_arrow(level, "%s: D", i->name());
    } else {
      hlog_debug_arrow(level, "%s: %ld", i->name(), i->mtime());
    }
  }
}

// Plugin stuff

// Default constructor creates a manager
CvsParser manager;

ParserManifest manifest = { &manager };
