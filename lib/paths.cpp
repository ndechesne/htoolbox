/*
     Copyright (C) 2006-2008  Herve Fache

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

#include <string>
#include <list>
#include <errno.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "report.h"
#include "conditions.h"
#include "filters.h"
#include "parsers.h"
#include "cvs_parser.h"
#include "svn_parser.h"
#include "db.h"
#include "paths.h"

using namespace hbackup;

int ClientPath::parse_recurse(
    Database&       db,
    const char*     remote_path,
    int             start,
    Directory&      dir,
    const Parser*   current_parser) {
  if (aborting()) {
    return -1;
  }

  // Check whether directory is under SCM control
  Parser* parser = NULL;
  if (! _parsers.empty()) {
    // We have a parser, check this directory with it
    if (current_parser != NULL) {
      parser = current_parser->isControlled(dir.path());
    }
    // We don't have a parser [anymore], check this directory
    if (parser == NULL) {
      parser = _parsers.isControlled(dir.path());
    }
  }

  bool give_up = false;
  if (dir.isValid() && ! dir.createList()) {
    for (list<Node*>::iterator i = dir.nodesList().begin();
        i != dir.nodesList().end(); delete *i, i = dir.nodesList().erase(i)) {
      if (! aborting() && ! give_up) {
        // Ignore inaccessible files
        if ((*i)->type() == '?') {
          continue;
        }

        // Always ignore a dir named '.hbackup'
        if (((*i)->type() == 'd') && (strcmp((*i)->name(), ".hbackup") == 0)) {
          continue;
        }

        // Let the parser analyse the file data to know whether to back it up
        if ((parser != NULL) && (parser->ignore(**i))) {
          continue;
        }

        // Now pass it through the filters
        if ((_ignore != NULL) && _ignore->match(**i, start)) {
          continue;
        }

        // Count the nodes considered, for info
        _nodes++;

        // Also deal with directory, as some fields should not be considered
        if ((*i)->type() == 'd') {
          (*i)->resetMtime();
          (*i)->resetSize();
        }

        // Synchronize with DB records
        Path rem_path(remote_path, (*i)->name());
        Database::OpData op(rem_path, **i);
        db.sendEntry(op);
        // Add node
        if (op.needsAdding()) {
          if (((*i)->type() == 'f')
          && (_compress != NULL) && _compress->match(**i, start)) {
            op.setCompression(5);
          }
          if (db.add(op)
          &&  (  (errno != EBUSY)       // Ignore busy files
              && (errno != ENOENT)      // Ignore files gone
              && (errno != EACCES))) {  // Ignore access refused
            // All the rest results in a cease and desist order
            give_up = true;
          }
        }

        // For directory, recurse into it
        if ((*i)->type() == 'd') {
          out(debug, msg_standard, &rem_path[_path.length() + 1], 1, "Dir");
          if (parse_recurse(db, rem_path, start,
              static_cast<Directory&>(**i), parser) < 0) {
            give_up = true;
          }
        }
      }
    }
  } else {
    out(error, msg_errno, "Reading directory", errno, remote_path);
  }
  delete parser;
  return give_up ? -1 : 0;
}

ClientPath::ClientPath(const char* path) {
  _ignore             = NULL;
  _compress           = NULL;
  _path = path;
  // Change '\' into '/'
  _path.fromDos();
  // Remove trailing '/'s
  _path.noTrailingSlashes();
}

int ClientPath::addParser(
    const string&   type,
    const string&   string) {
  Parser::Mode mode;

  /* Determine mode */
  switch (string[0]) {
    case 'c':
      // All controlled files
      mode = Parser::controlled;
      break;
    case 'l':
      // Local files
      mode = Parser::modifiedandothers;
      break;
    case 'm':
      // Modified controlled files
      mode = Parser::modified;
      break;
    case 'o':
      // Non controlled files
      mode = Parser::others;
      break;
    default:
      out(error, msg_standard, "Undefined parser mode", -1, string.c_str());
      return -1;
  }

  /* Add specified parser */
  if (type == "cvs") {
    _parsers.push_back(new CvsParser(mode));
  } else
  if (type == "svn") {
    _parsers.push_back(new SvnParser(mode));
  } else
  {
    out(error, msg_standard, "Unsupported parser", -1, type.c_str());
    return -1;
  }
  return 0;
}

Filter* ClientPath::addFilter(
    const string&   type,
    const string&   name) {
  return _filters.add(type, name);
}

Filter* ClientPath::findFilter(
    const string&   name) const {
  return _filters.find(name);
}

int ClientPath::parse(
    Database&       db,
    const char*     backup_path) {
  int rc = 0;
  _nodes = 0;
  Directory dir(backup_path);
  if (parse_recurse(db, _path, strlen(backup_path) + 1, dir, NULL)
  || aborting()) {
    rc = -1;
  }
  return rc;
}

void ClientPath::show(int level) const {
  out(verbose, msg_standard, _path, level, "Path");
  _filters.show(level + 1);
  _parsers.show(level + 1);
  if (_compress != NULL) {
    out(verbose, msg_standard, _compress->name().c_str(), level + 1,
      "Compress filter");
  }
  if (_ignore != NULL) {
    out(verbose, msg_standard, _ignore->name().c_str(), level + 1,
      "Ignore filter");
  }
}
