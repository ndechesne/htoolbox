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
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

using namespace std;

#include "strings.h"
#include "files.h"
#include "filters.h"
#include "parsers.h"
#include "cvs_parser.h"
#include "list.h"
#include "db.h"
#include "paths.h"
#include "hbackup.h"

using namespace hbackup;

int Path::recurse(
    Database&     db,
    const char*   remote_path,
    const char*   local_path,
    Directory*    dir,
    Parser*       parser) {
  if (terminating()) {
    errno = EINTR;
    return -1;
  }

  // Get relative path
  const char* rel_path = &remote_path[_path.length() + 1];

  // Check whether directory is under SCM control
  if (! _parsers.empty()) {
    // We have a parser, check this directory with it
    if (parser != NULL) {
      parser = parser->isControlled(local_path);
    }
    // We don't have a parser [anymore], check this directory
    if (parser == NULL) {
      parser = _parsers.isControlled(local_path);
    }
  }
  if (dir->isValid() && ! dir->createList(local_path)) {
    list<Node*>::iterator i = dir->nodesList().begin();
    while (i != dir->nodesList().end()) {
      if (! terminating()) {
        // Ignore inaccessible files
        if ((*i)->type() == '?') {
          i = dir->nodesList().erase(i);
          continue;
        }

        // Let the parser analyse the file data to know whether to back it up
        if ((parser != NULL) && (parser->ignore(*(*i)))) {
          i = dir->nodesList().erase(i);
          continue;
        }

        // Now pass it through the filters
        if (! _filters.empty() && _filters.match(rel_path, *(*i))) {
          i = dir->nodesList().erase(i);
          continue;
        }

        // Count the nodes considered, for info
        _nodes++;

        // For link, find out linked path
        if ((*i)->type() == 'l') {
          Link *l = new Link(*(*i), local_path);
          delete *i;
          *i = l;
        }

        // Also deal with directory, as some fields should not be considered
        if ((*i)->type() == 'd') {
          Directory *d = new Directory(*(*i));
          delete *i;
          *i = d;
        }

        // Synchronize with DB records
        if (db.sendEntry(remote_path, local_path, *i) < 0) {
          cerr << "Failed to compare entries" << endl;
        }

        // For directory, recurse into it
        if ((*i)->type() == 'd') {
          char* rem_path = NULL;
          asprintf(&rem_path, "%s%s/", remote_path, (*i)->name());
          char* loc_path = Node::path(local_path, (*i)->name());
          if (verbosity() > 1) {
            cout << " -> Entering " << rel_path << (*i)->name() << endl;
          }
          recurse(db, rem_path, loc_path, (Directory*) *i, parser);
          if (verbosity() > 1) {
            cout << " -> Leaving " << rel_path << (*i)->name() << endl;
          }
          free(rem_path);
          free(loc_path);
        }
      }
      delete *i;
      i = dir->nodesList().erase(i);
    }
  } else {
    cerr << strerror(errno) << ": " << rel_path << endl;
  }
  return 0;
}

Path::Path(const char* path) {
  _dir                = NULL;
  _expiration         = 0;

  // Copy path accross
  _path = path;

  // Change '\' into '/'
  _path.toUnix();

  // Remove trailing '/'s
  _path.noEndingSlash();
}

int Path::addFilter(
    const string& type,
    const string& value,
    bool          append) {
  Condition *condition;

  if (append && _filters.empty()) {
    // Can't append to nothing
    return 3;
  }

  /* Add specified filter */
  if (type == "type") {
    char file_type;
    if ((value == "file") || (value == "f")) {
      file_type = 'f';
    } else
    if ((value == "dir") || (value == "d")) {
      file_type = 'd';
    } else
    if ((value == "char") || (value == "c")) {
      file_type = 'c';
    } else
    if ((value == "block") || (value == "b")) {
      file_type = 'b';
    } else
    if ((value == "pipe") || (value == "p")) {
      file_type = 'p';
    } else
    if ((value == "link") || (value == "l")) {
      file_type = 'l';
    } else
    if ((value == "socket") || (value == "s")) {
      file_type = 's';
    } else {
      // Wrong value
      return 2;
    }
    condition = new Condition(filter_type, file_type);
  } else
  if (type == "name") {
    condition = new Condition(filter_name, value);
  } else
  if (type == "name_start") {
    condition = new Condition(filter_name_start, value);
  } else
  if (type == "name_end") {
    condition = new Condition(filter_name_end, value);
  } else
  if (type == "name_regex") {
    condition = new Condition(filter_name_regex, value);
  } else
  if (type == "path") {
    condition = new Condition(filter_path, value);
  } else
  if (type == "path_start") {
    condition = new Condition(filter_path_start, value);
  } else
  if (type == "path_end") {
    condition = new Condition(filter_path_end, value);
  } else
  if (type == "path_regex") {
    condition = new Condition(filter_path_regex, value);
  } else
  if (type == "size_below") {
    off_t size = strtoul(value.c_str(), NULL, 10);
    condition = new Condition(filter_size_below, size);
  } else
  if (type == "size_above") {
    off_t size = strtoul(value.c_str(), NULL, 10);
    condition = new Condition(filter_size_above, size);
  } else {
    // Wrong type
    return 1;
  }

  if (append) {
    _filters.back().push_back(*condition);
  } else {
    _filters.push_back(Filter(*condition));
  }
  delete condition;
  return 0;
}

int Path::addParser(
    const string& type,
    const string& string) {
  parser_mode_t mode;

  /* Determine mode */
  switch (string[0]) {
    case 'c':
      // All controlled files
      mode = parser_controlled;
      break;
    case 'l':
      // Local files
      mode = parser_modifiedandothers;
      break;
    case 'm':
      // Modified controlled files
      mode = parser_modified;
      break;
    case 'o':
      // Non controlled files
      mode = parser_others;
      break;
    default:
      cerr << "Undefined mode " << type << " for parser " << string << endl;
      return 1;
  }

  /* Add specified parser */
  if (type == "cvs") {
    _parsers.push_back(new CvsParser(mode));
  } else {
    cerr << "Unsupported parser " << string << endl;
    return 2;
  }
  return 0;
}

int Path::parse(
    Database&   db,
    const char* backup_path) {
  int rc = 0;
  _nodes = 0;
  _dir = new Directory(backup_path);
  if (recurse(db, (_path + '/').c_str(), backup_path, _dir, NULL)) {
    delete _dir;
    _dir = NULL;
    rc = -1;
  }
  return rc;
}
