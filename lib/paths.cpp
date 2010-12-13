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

#include <string>
#include <list>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "hreport.h"
#include "conditions.h"
#include "filters.h"
#include "parsers.h"
#include "db.h"
#include "attributes.h"
#include "paths.h"

using namespace hbackup;
using namespace htools;

int ClientPath::parse_recurse(
    Database&       db,
    const char*     client_name,
    const char*     remote_path,
    const char*     tree_base_path,
    bool            tree_symlinks,
    size_t          start,
    Node&           dir,
    IParser*        current_parser) {
  if (aborting()) {
    return -1;
  }

  // Check whether directory is under SCM control
  IParser* parser = NULL;
  if (! _parsers.empty()) {
    // We have a parser, check this directory with it
    if (current_parser != NULL) {
      parser = current_parser->createChildIfControlled(dir.path());
    }
    // We don't have a parser [anymore], check this directory
    if (parser == NULL) {
      parser = _parsers.createParserIfControlled(dir.path());
    }
  }

  bool give_up = false;
  char rem_path[PATH_MAX];
  size_t remote_path_len = sprintf(rem_path, "%s/", remote_path);
  char tree_path[PATH_MAX];
  size_t tree_base_path_len;
  if (tree_base_path != NULL) {
    tree_base_path_len = sprintf(tree_path, "%s/", tree_base_path);
  }
  if (dir.hasList()) {
    for (list<Node*>::iterator i = dir.nodesList().begin();
        i != dir.nodesList().end(); delete *i, i = dir.nodesList().erase(i)) {
      if (! aborting() && ! give_up) {
        size_t rem_path_len = remote_path_len +
          sprintf(&rem_path[remote_path_len], "%s", (*i)->name());
        char code[] = "       ";
        // Ignore inaccessible files (should not happen)
        if ((*i)->type() == '?') {
          code[0] = 'I';
          code[1] = (*i)->type();
          code[3] = 'u';
        } else

        // Always ignore a dir named '.hbackup'
        if (((*i)->type() == 'd') && (strcmp((*i)->name(), ".hbackup") == 0)) {
          code[0] = 'I';
          code[1] = (*i)->type();
          code[3] = 's';
        } else

        // Let the parser analyse the file data to know whether to back it up
        if ((parser != NULL) && (parser->ignore(**i))) {
          code[0] = 'I';
          code[1] = (*i)->type();
          code[3] = 'p';
        } else

        // Now pass it through the filters
        if (_attributes.mustBeIgnored(**i, start)) {
          code[0] = 'I';
          code[1] = (*i)->type();
          code[3] = 'f';
        } else
        {
          // Count the nodes considered, for info
          _nodes++;

          // Prepare path for tree
          size_t tree_path_len = tree_base_path_len;
          if (tree_base_path != NULL) {
            tree_path_len += sprintf(&tree_path[tree_base_path_len], "%s",
              (*i)->name());
          }

          // Synchronize with DB records
          Database::OpData op(rem_path, rem_path_len, **i);

          // Parse directory, disregard/re-use some irrelevant fields
          bool create_list_failed = false;
          if ((*i)->type() == 'd') {
            (*i)->setMtime(0);
            (*i)->setSize(0);
            if ((*i)->createList()) {
              (*i)->deleteList();
              create_list_failed = true;
              if ((errno != EACCES) &&  // Ignore access refused
                  (errno != ENOENT)) {  // Ignore directory gone
                // All the rest results in a cease and desist order
                give_up = true;
              } else {
                // Remember the error level
                (*i)->setSize(-1);
              }
            }
          }
          db.sendEntry(op);
          // Add node
          int status = 0;
          if (op.needsAdding()) {
            // Regular file: deal with compression
            if ((*i)->type() == 'f') {
              if ((_no_compress != NULL) && _no_compress->match(**i, start)) {
                /* If in no-compress list, don't compress */
                op.comp_mode = Database::never;
              } else
              if (op.comp_mode == Database::auto_now) {
                /* If auto-compress list, let it be */
                op.compression = compression_level;
              } else
              if ((_compress != NULL) && _compress->match(**i, start)) {
                /* If in compress list, compress */
                op.compression = compression_level;
              } else
              {
                /* Do not compress for now */
                op.compression = 0;
              }
            }
            status = db.add(op, _attributes.reportCopyErrorOnceIsSet());
            if ((status < 0) &&
                ( (errno != EBUSY) &&    // Ignore busy resources
                  (errno != ETXTBSY) &&  // Ignore busy files
                  (errno != ENOENT) &&   // Ignore files gone
                  (errno != EPERM) &&   // Ignore files gone
                  (errno != EACCES))) {  // Ignore access refused
              // All the rest results in a cease and desist order
              give_up = true;
            }
            op.verbose(code);

            if (create_list_failed) {
              if (! (_attributes.reportCopyErrorOnceIsSet() &&
                  op.same_list_entry)) {
                hlog_error("%s reading directory '%s:%s/%s'", strerror(errno),
                  client_name, remote_path, (*i)->name());
              } else {
                hlog_warning("%s reading directory '%s:%s/%s'", strerror(errno),
                  client_name, remote_path, (*i)->name());
              }
            }
            hlog_info("%-8s%s", code, rem_path);
          } else
          if ((tree_base_path != NULL) && ((*i)->type() == 'f')) {
            db.setStorePath(op);
          }

          // For directory, recurse into it
          if ((*i)->type() == 'f') {
            if ((status >= 0) && (tree_base_path != NULL)) {
              if (op.compression > 0) {
                strcpy(&tree_path[tree_path_len], ".gz");
              }
              if (tree_symlinks) {
                symlink(op.store_path.c_str(), tree_path);
              } else {
                link(op.store_path.c_str(), tree_path);
              }
            }
          } else
          if ((*i)->type() == 'l') {
            if (tree_base_path != NULL) {
              symlink((*i)->link(), tree_path);
            }
          } else
          if ((*i)->type() == 'd') {
            if (tree_base_path != NULL) {
              mkdir(tree_path, 0755);
            }
            if ((*i)->size() != -1) {
              hlog_verbose_temp("%s", &rem_path[_path.length() + 1]);
            }
            if (parse_recurse(db, client_name, rem_path,
                tree_base_path != NULL ? tree_path : NULL, tree_symlinks,
                start, **i, parser) < 0) {
              give_up = true;
            }
          }
          continue;
        }
        if (code[0] == 'I') {
          hlog_verbose("%-8s%s", code, rem_path);
        }
      }
    }
  }
  delete parser;
  return give_up ? -1 : 0;
}

ClientPath::ClientPath(
    const char* path,
    const Attributes& a,
    const ParsersManager& parsers_manager)
    : _path(path, Path::NO_TRAILING_SLASHES | Path::NO_REPEATED_SLASHES |
        Path::CONVERT_FROM_DOS),
      _attributes(a), _parsers_manager(parsers_manager),
      _compress(NULL), _no_compress(NULL) {
}

ConfigObject* ClientPath::configChildFactory(
    const vector<string>& params,
    const char*           file_path,
    size_t                line_no) {
  ConfigObject* co = NULL;
  const string& keyword = params[0];
  if ((keyword == "compress") ||  (keyword == "no_compress")) {
    Filter* filter = _attributes.filters().find(params[1]);
    if (filter == NULL) {
      hlog_error("Filter not found '%s' in file '%s', at line %zu",
        params[1].c_str(), file_path, line_no);
    } else {
      if (keyword == "compress") {
        _compress = filter;
      } else
      if (keyword == "no_compress") {
        _no_compress = filter;
      }
      co = this;  // Anything but NULL
    }
  } else
  if (keyword == "parser") {
    switch (addParser(params[1], params[2])) {
      case 1:
        hlog_error("Unsupported parser type '%s' in file '%s', at line %zu",
          params[1].c_str(), file_path, line_no);
        break;
      case 2:
        hlog_error("Unsupported parser mode '%s' in file '%s', at line %zu",
          params[2].c_str(), file_path, line_no);
        break;
      default:
        co = this;  // Anything but NULL
    }
  } else
  {
    co = _attributes.configChildFactory(params, file_path, line_no);
  }
  return co;
}

int ClientPath::addParser(
    const string&   name,
    const string&   mode) {
  IParser* parser = _parsers_manager.createParser(name.c_str(), mode.c_str());
  if (parser == NULL) {
    return -1;
  }
  _parsers.push_back(parser);
  return 0;
}

int ClientPath::parse(
    Database&       db,
    const char*     client_name,
    const char*     backup_path,
    const char*     tree_base_path,
    bool            tree_symlinks) {
  int rc = 0;
  _nodes = 0;
  Node dir(backup_path);
  if (! dir.isDir() || dir.createList()) {
    char* full_name;
    if (asprintf(&full_name, "%s:%s", client_name,
                 static_cast<const char*>(_path)) < 0) {
      hlog_error("%s creating final path '%s'", strerror(errno), _path.c_str());
      rc = -1;
    } else {
      hlog_error("%s reading initial directory '%s'", strerror(errno),
        full_name);
      free(full_name);
    }
    rc = -1;
  } else {
    bool   tree_path_set = true;
    string tree_path;
    if (_attributes.treeIsSet()) {
      tree_path = _attributes.tree();
    } else
    if (tree_base_path != NULL) {
      tree_path = Path(tree_base_path, _path);
    } else
    {
      tree_path_set = false;
    }
    if (tree_path_set) {
      Node::mkdir_p(tree_path.c_str(), 0777);
    }
    if (parse_recurse(db, client_name, _path,
          tree_path_set ? tree_path.c_str() : NULL, tree_symlinks,
          strlen(backup_path) + 1, dir, NULL) || aborting()) {
      rc = -1;
    }
  }
  return rc;
}

void ClientPath::show(int level) const {
  hlog_debug_arrow(level++, "Path: %s", _path.c_str());
  _attributes.show(level);
  if (_compress != NULL) {
    hlog_debug_arrow(level, "Compress filter: %s",
      _compress->name().c_str());
  }
  if (_no_compress != NULL) {
    hlog_debug_arrow(level, "No compress filter: %s",
      _no_compress->name().c_str());
  }
  _parsers.show(level);
}
