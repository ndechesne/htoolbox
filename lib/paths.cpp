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

#include <iostream>
#include <string>
#include <list>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

using namespace std;

#include "files.h"
#include "conditions.h"
#include "filters.h"
#include "parsers.h"
#include "cvs_parser.h"
#include "svn_parser.h"
#include "list.h"
#include "db.h"
#include "paths.h"
#include "hbackup.h"

using namespace hbackup;

int ClientPath::parse_recurse(
    Database&       db,
    const char*     remote_path,
    Directory&      dir,
    Parser*         parser) {
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
      parser = parser->isControlled(dir.path());
    }
    // We don't have a parser [anymore], check this directory
    if (parser == NULL) {
      parser = _parsers.isControlled(dir.path());
    }
  }

  bool give_up = false;
  if (dir.isValid() && ! dir.createList()) {
    list<Node*>::iterator i = dir.nodesList().begin();
    while (i != dir.nodesList().end()) {
      if (! terminating() && ! give_up) {
        // Ignore inaccessible files
        if ((*i)->type() == '?') {
          goto end;
        }

        // Always ignore a dir named '.hbackup'
        if (((*i)->type() == 'd') && (strcmp((*i)->name(), ".hbackup") == 0)) {
          goto end;
        }

        // Let the parser analyse the file data to know whether to back it up
        if ((parser != NULL) && (parser->ignore(**i))) {
          goto end;
        }

        // Now pass it through the filters
        if ((_ignore != NULL) && _ignore->match(rel_path, **i)) {
          goto end;
        }

        // Count the nodes considered, for info
        _nodes++;

        // Also deal with directory, as some fields should not be considered
        if ((*i)->type() == 'd') {
          (*i)->resetMtime();
          (*i)->resetSize();
        }

        // Synchronize with DB records
        char* rem_path = NULL;
        int last = asprintf(&rem_path, "%s%s/", remote_path, (*i)->name()) - 1;
        rem_path[last] = '\0';
        char* checksum = NULL;
        int rc = db.sendEntry(rem_path, *i, &checksum);
        if (rc < 0) {
          give_up = true;
        } else {
          // Add node
          if (rc > 0) {
            int compress = 0;
            if (((*i)->type() == 'f')
              && (_compress != NULL) && _compress->match(rel_path, **i)) {
              compress = 5;
            }
            if (db.add(rem_path, *i, checksum, compress)
            &&  ((errno == EIO) || (errno == EHOSTDOWN))) {
              give_up = true;
            }
          }

          // For directory, recurse into it
          if ((*i)->type() == 'd') {
            rem_path[last] = '/';
            out(debug, 1) << "Dir " << rel_path << (*i)->name() << endl;
            if (parse_recurse(db, rem_path, (Directory&) **i, parser)) {
              give_up = true;
            }
          }
        }
        free(checksum);
        free(rem_path);
      }
end:
      delete *i;
      i = dir.nodesList().erase(i);
    }
  } else {
    out(error) << strerror(errno) << ": " << remote_path << endl;
  }
  return give_up ? -1 : 0;
}

ClientPath::ClientPath(const char* path) {
  _ignore             = NULL;
  _compress           = NULL;
  // Change '\' into '/'
  char* unix_path = Path::fromDos(path);
  _path = unix_path;
  free(unix_path);
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
      out(error) << "Undefined mode " << type << " for parser " << string
        << endl;
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
    out(error) << "Unsupported parser " << type << endl;
    return -1;
  }
  return 0;
}

Filter* ClientPath::findFilter(
    const string&   name,
    const Filters*  local,
    const Filters*  global) const {
  Filter* filter = _filters.find(name);
  if ((filter == NULL) && (local != NULL)) {
    filter = local->find(name);
  }
  if ((filter == NULL) && (global != NULL)) {
    filter = global->find(name);
  }
  return filter;
}

int ClientPath::parse(
    Database&       db,
    const char*     backup_path) {
  int rc = 0;
  _nodes = 0;
  char* rem_path = NULL;
  asprintf(&rem_path, "%s/", _path.c_str());
  Directory dir(backup_path);
  if (parse_recurse(db, rem_path, dir, NULL) || terminating()) {
    rc = -1;
  }
  free(rem_path);
  return rc;
}

void ClientPath::show(int level) const {
  out(verbose, level) << "Path: " << _path << endl;
  _filters.show(level + 1);
  _parsers.show(level + 1);
  if (_compress != NULL) {
    out(verbose, level + 1) << "Compress filter: " << _compress->name()
      << endl;
  }
  if (_ignore != NULL) {
    out(verbose, level + 1) << "Ignore filter:   " << _ignore->name() << endl;
  }
}
