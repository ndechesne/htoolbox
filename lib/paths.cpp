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
#include <errno.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "hreport.h"
#include "conditions.h"
#include "filters.h"
#include "parsers.h"
#include "opdata.h"
#include "db.h"
#include "attributes.h"
#include "paths.h"

using namespace hbackup;
using namespace hreport;

int ClientPath::parse_recurse(
    Database&       db,
    const char*     remote_path,
    const char*     client_name,
    size_t          start,
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
      parser = current_parser->createChildIfControlled(dir.path());
    }
    // We don't have a parser [anymore], check this directory
    if (parser == NULL) {
      parser = _parsers.createParserIfControlled(dir.path());
    }
  }

  bool give_up = false;
  if (dir.hasList()) {
    for (list<Node*>::iterator i = dir.nodesList().begin();
        i != dir.nodesList().end(); delete *i, i = dir.nodesList().erase(i)) {
      if (! aborting() && ! give_up) {
        Path rem_path(remote_path, (*i)->name());
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

          // Synchronize with DB records
          OpData op(rem_path, **i);

          // Parse directory, disregard/re-use some irrelevant fields
          bool create_list_failed = false;
          if ((*i)->type() == 'd') {
            Directory& d = static_cast<Directory&>(**i);
            d.setMtime(0);
            d.setSize(0);
            if (d.createList()) {
              d.deleteList();
              create_list_failed = true;
              if ((errno != EACCES)     // Ignore access refused
              &&  (errno != ENOENT)) {  // Ignore directory gone
                // All the rest results in a cease and desist order
                give_up = true;
              } else {
                // Remember the error level
                d.setSize(-1);
              }
            }
          }

          db.sendEntry(op);
          // Add node
          if (op.needsAdding()) {
            // Regular file: deal with compression
            if ((*i)->type() == 'f') {
              if ((_no_compress != NULL) && _no_compress->match(**i, start)) {
                /* If in no-compress list, don't compress */
                op.setCompressionMode(OpData::never);
              } else
              if (op.compressionMode() == OpData::auto_now) {
                /* If auto-compress list, let it be */
                op.setCompression(compression_level);
              } else
              if ((_compress != NULL) && _compress->match(**i, start)) {
                /* If in compress list, compress */
                op.setCompression(compression_level);
              } else
              {
                /* Do not compress for now */
                op.setCompression(0);
              }
            }
            if (db.add(op, _attributes.reportCopyErrorOnceIsSet())
            &&  (  (errno != EBUSY)       // Ignore busy resources
                && (errno != ETXTBSY)     // Ignore busy files
                && (errno != ENOENT)      // Ignore files gone
                && (errno != EACCES))) {  // Ignore access refused
              // All the rest results in a cease and desist order
              give_up = true;
            }
            op.verbose(code);

            if (create_list_failed) {
              if (! (_attributes.reportCopyErrorOnceIsSet() &&
                  op.sameListEntry())) {
                hlog_error("%s reading directory '%s:%s/%s'", strerror(errno),
                  client_name, remote_path, (*i)->name());
              } else {
                hlog_warning("%s reading directory '%s:%s/%s'", strerror(errno),
                  client_name, remote_path, (*i)->name());
              }
            }
            out(info, msg_standard, rem_path, -2, code);
          }

          // For directory, recurse into it
          if ((*i)->type() == 'd') {
            if ((*i)->size() != -1) {
              out(verbose, msg_standard, &rem_path[_path.length() + 1], -3, NULL);
            }
            if (parse_recurse(db, rem_path, client_name, start,
                static_cast<Directory&>(**i), parser) < 0) {
              give_up = true;
            }
          }
          continue;
        }
        if (code[0] == 'I') {
          out(verbose, msg_standard, rem_path, -2, code);
        }
      }
    }
  }
  delete parser;
  return give_up ? -1 : 0;
}

ClientPath::ClientPath(const char* path, const Attributes& a)
    : _path(path), _attributes(a), _compress(NULL), _no_compress(NULL) {
  // Change '\' into '/'
  _path.fromDos();
  // Remove trailing '/'s
  _path.noTrailingSlashes();
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
      hlog_error("filter not found '%s' in file '%s', at line %zu",
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
        hlog_error("unsupported parser type '%s' in file '%s', at line %zu",
          params[1].c_str(), file_path, line_no);
        break;
      case 2:
        hlog_error("unsupported parser mode '%s' in file '%s', at line %zu",
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
  Parser* parser = parsers_registered.createParser(name, mode);
  if (parser == NULL) {
    return -1;
  }
  _parsers.push_back(parser);
  return 0;
}

int ClientPath::parse(
    Database&       db,
    const char*     backup_path,
    const char*     client_name) {
  int rc = 0;
  _nodes = 0;
  Directory dir(backup_path);
  if (! dir.isValid() || dir.createList()) {
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
  } else
  if (parse_recurse(db, _path, client_name, strlen(backup_path) + 1, dir, NULL)
  || aborting()) {
    rc = -1;
  }
  return rc;
}

void ClientPath::show(int level) const {
  out(debug, msg_standard, _path, level++, "Path");
  _attributes.show(level);
  if (_compress != NULL) {
    out(debug, msg_standard, _compress->name().c_str(), level,
      "Compress filter");
  }
  if (_no_compress != NULL) {
    out(debug, msg_standard, _no_compress->name().c_str(), level,
      "No compress filter");
  }
  _parsers.show(level);
}
