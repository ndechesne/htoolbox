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
#include "conditions.h"
#include "filters.h"
#include "parsers.h"
#include "cvs_parser.h"
#include "list.h"
#include "db.h"
#include "paths.h"
#include "hbackup.h"

using namespace hbackup;

int Path::recurse(
    Database&       db,
    const char*     remote_path,
    Directory*      dir,
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
      parser = parser->isControlled(dir->path());
    }
    // We don't have a parser [anymore], check this directory
    if (parser == NULL) {
      parser = _parsers.isControlled(dir->path());
    }
  }
  if (dir->isValid() && ! dir->createList()) {
    list<Node*>::iterator i = dir->nodesList().begin();
    while (i != dir->nodesList().end()) {
      if (! terminating()) {
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

        // For link, find out linked path
        if ((*i)->type() == 'l') {
          Link *l = new Link(**i);
          delete *i;
          *i = l;
        }

        // Also deal with directory, as some fields should not be considered
        if ((*i)->type() == 'd') {
          Directory *d = new Directory(**i);
          d->resetMtime();
          d->resetSize();
          delete *i;
          *i = d;
        }

        // Synchronize with DB records
        char* rem_path = NULL;
        int last = asprintf(&rem_path, "%s%s/", remote_path, (*i)->name()) - 1;
        rem_path[last] = '\0';
        Node* db_node = NULL;
        int rc = db.sendEntry(rem_path, *i, &db_node);
        const char* checksum = NULL;
        bool        add      = false;
        if (rc < 0) {
          cerr << "Failed to compare entries" << endl;
        } else
        if (rc > 0) {
          if (verbosity() > 0) {
            cout << "A";
          }
          add = true;
        } else
        {
          // Check for differences
          if (*db_node != **i) {
            // Metadata differ
            if (((*i)->type() == 'f')
            && (db_node->type() == 'f')
            && ((*i)->size() == db_node->size())
            && ((*i)->mtime() == db_node->mtime())) {
              // If the file data is there, just add new metadata
              // If the checksum is missing, this shall retry too
              checksum = ((File*)db_node)->checksum();
              if (verbosity() > 0) {
                cout << "~";
              }
            } else {
              // Do it all
              if (verbosity() > 0) {
                cout << "M";
              }
            }
            add = true;
          } else
          // Same metadata, hence same type...
          {
            // Compare linked data
            if (((*i)->type() == 'l')
            && (strcmp(((Link*)*i)->link(), ((Link*)db_node)->link()) != 0)) {
              if (verbosity() > 0) {
                cout << "L";
              }
              add = true;
            } else
            // Check that file data is present
            if (((*i)->type() == 'f')
              && (((File*)db_node)->checksum()[0] == '\0')) {
              // Checksum missing: retry
              if (verbosity() > 0) {
                cout << "!";
              }
              checksum = ((File*)db_node)->checksum();
              add = true;
            }
          }
        }
        if (add) {
          if (verbosity() > 0) {
            cout << " " << rem_path;
            if ((*i)->type() == 'f') {
              cout << " (";
              if ((*i)->size() < 10000) {
                cout << (*i)->size();
                cout << " ";
              } else
              if ((*i)->size() < 10000000) {
                cout << (*i)->size() / 1000;
                cout << " k";
              } else
              if ((*i)->size() < 10000000000ll) {
                cout << (*i)->size() / 1000000;
                cout << " M";
              } else
              {
                cout << (*i)->size() / 1000000000;
                cout << " G";
              }
              cout << "B)";
            }
            cout << endl;
          }
          int compress = 0;
          if (((*i)->type() == 'f')
           && (_compress != NULL) && _compress->match(rel_path, **i)) {
            compress = 5;
          }
          db.add(rem_path, *i, checksum, compress);
        }

        // For directory, recurse into it
        if ((*i)->type() == 'd') {
          rem_path[last] = '/';
          if (verbosity() > 1) {
            cout << " -> Dir " << rel_path << (*i)->name() << endl;
          }
          recurse(db, rem_path, (Directory*) *i, parser);
        }
        free(rem_path);
        delete db_node;
      }
end:
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
  _ignore             = NULL;
  _compress           = NULL;
  // Copy path accross
  _path = path;
  // Change '\' into '/'
  _path.toUnix();
  // Remove trailing '/'s
  _path.noEndingSlash();
}

Path::~Path() {
  delete _dir;
}

int Path::addParser(
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

Filter* Path::findFilter(
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

int Path::parse(
    Database&       db,
    const char*     backup_path) {
  int rc = 0;
  _nodes = 0;
  _dir = new Directory(backup_path);
  if (recurse(db, (_path + '/').c_str(), _dir, NULL)) {
    delete _dir;
    _dir = NULL;
    rc = -1;
  }
  return rc;
}
