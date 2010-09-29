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

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>

using namespace std;

#include "parsers.h"

using namespace hbackup;
using namespace htools;

static const string control_dir = "/.svn";
static const char* entries = "/entries";

/* As this is put into a list, it gets constructed, then copied, and the
 * original gets destroyed. As I do not want to copy strings for no reason, I
 * made name a pointer, and made sure only the copied version would free it.
 */
struct SvnNode {
  char* name;
  char status;
  bool is_copy;
  bool operator<(const SvnNode& right) const {
    // Only compare paths
    return Path::compare(name, right.name) < 0;
  }
  SvnNode(const char* a_name, char a_status) {
    name = strdup(a_name);
    status = a_status;
    is_copy = false;
  }
  SvnNode(const SvnNode& source) {
    name = source.name;
    status = source.status;
    is_copy = true;
  }
  ~SvnNode() {
    if (is_copy) {
      free(name);
    }
  }
private:
  SvnNode();
  SvnNode& operator=(const SvnNode&);
};

class SvnParser : public IParser {
  list<SvnNode> _files;       // Files under control in current dir
  bool _head;
public:
  // Constructor
  SvnParser(Mode mode = master, const string& dir_path = "");
  // Tell them who we are
  const char* name() const { return "Subversion"; };
  const char* code() const { return "svn"; };
  // Factory
  IParser* createInstance(Mode mode) { return new SvnParser(mode); }
  // This will create an appropriate parser for the directory if relevant
  IParser* createChildIfControlled(const string& dir_path);
  // That tells us whether to ignore the file, i.e. not back it up
  bool ignore(const Node& node);
  // For debug purposes
  void show(int level = 0);
};

// Parser for Subversion control directory '.svn'
class SvnControlParser : public IParser {
public:
  // Just to know the parser used
  const char* name() const { return "Subversion Control"; }
  const char* code() const { return "svn_c"; }
  // This directory has no controlled children
  IParser* createChildIfControlled(const string& dir_path) {
    (void) dir_path;
    return new IgnoreParser;
  }
  // That tells us whether to ignore the file, i.e. not back it up
  bool ignore(const Node& node) {
    if (strcmp(node.name(), "entries") == 0) {
      return false;
    }
    return true;
  }
};

class SvnParserProxy : public IParser {
  SvnParser* _head;
  SvnParserProxy(Mode mode, const string& dir_path);
public:
  SvnParserProxy(SvnParser* head) : _head(head) {}
  const char* name() const { return "Subversion Proxy"; }
  const char* code() const { return "svn_p"; }
  IParser* createChildIfControlled(const string& dir_path) {
    // Parent under control, this is the control directory
    if ((dir_path.size() > control_dir.size())
    &&  (dir_path.substr(dir_path.size() - control_dir.size()) == control_dir)) {
      hlog_debug("Control dir '%s'", dir_path.c_str());
      return new SvnControlParser;
    }
    // Otherwise just return proxy
    return new SvnParserProxy(_head);
  }
  bool ignore(const Node& node) {
    return _head->ignore(node);
  }
  void show(int level = 0) {
    (void) level;
  }
};

// dir_path is the relative or absolute complete path to the dir
IParser *SvnParser::createChildIfControlled(const string& dir_path) {
  // Parent under control, this is the control directory
  if (! _no_parsing &&
       (dir_path.size() > control_dir.size()) &&
       (dir_path.substr(dir_path.size() - control_dir.size()) == control_dir)) {
    hlog_debug("Control dir '%s'", dir_path.c_str());
    return new SvnControlParser;
  }

  // If control directory exists and contains an entries file, assume control
  if (! File(Path((dir_path + control_dir).c_str(), &entries[1])).isValid()) {
    if (! _no_parsing) {
      hlog_warning("Directory '%s' should be under Subversion control",
        dir_path.c_str());
      return new IgnoreParser;
    } else {
      return NULL;
    }
  } else {
    if (_head) {
      SvnParser* parser = new SvnParser(_mode, dir_path);
      parser->_head = false;
      return parser;
    } else {
      return new SvnParserProxy(this);
    }
  }
}

SvnParser::SvnParser(Mode mode, const string& dir_path) :
    IParser(mode, dir_path), _head(true) {
  if (dir_path == "") {
    return;
  }
  /* Fill in list of files */
  hlog_debug_arrow(1, "Parsing Subversion entries in '%s'", dir_path.c_str());
  string command = "svn status --no-ignore --non-interactive \"" +
    dir_path + "\"";
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
      // Get status
      char status;
      switch (buffer[0]) {
        case ' ': // controlled
          continue;
        case 'D': // deleted
          status = 'd';
          break;
        case 'I': // ignored
          status = 'i';
          break;
        case '?': // other
          status = 'o';
          break;
        case 'A': // added
        default:  // modified/conflict/etc... => controlled and not up-to-date
          status = 'm';
      }
      // Add to list of Nodes, with status
      _files.push_back(SvnNode(&buffer[8], status));
    }
    free(buffer);
    pclose(fd);
    hlog_debug_arrow(1, "Sorting Subversion entries");
    _files.sort();
    show(2);
  } else {
    hlog_error("%s running svn status", strerror(errno));
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
  int  cmp             = -1;
  // Nodes are in path order, and so are SvnNodes, so we can delete as we go
  list<SvnNode>::iterator it;
  for (it = _files.begin(); it != _files.end(); it = _files.erase(it)) {
    // Find file
    cmp = strcmp(it->name, node.path());
    if (cmp >= 0) {
      break;
    }
  }

  if (cmp == 0) {
    if (it->status == 'm') {
      file_modified = true;
    } else {
      file_controlled = false;
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
      // Directories under control need to get parsed
      if (file_modified || (file_controlled && (node.type() == 'd'))) {
        return false;
      }
      break;
    case IParser::modifiedandothers:
      // Directories under control need to get parsed
      if (file_modified || (file_controlled && (node.type() == 'd'))
       || ! file_controlled) {
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

void SvnParser::show(int level) {
  list<SvnNode>::iterator  i;
  for (i = _files.begin(); i != _files.end(); i++) {
    hlog_debug_arrow(level, "%s: %c", i->name, i->status);
  }
}

// Plugin stuff

// Default constructor creates a manager
SvnParser manager;

ParserManifest manifest = { &manager };
