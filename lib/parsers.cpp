/*
     Copyright (C) 2008-2010  Herve Fache

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

#include <dlfcn.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "hreport.h"
#include "parsers.h"

using namespace hbackup;
using namespace hreport;

// Parsers

Parsers::~Parsers() {
  list<IParser*>::iterator i;
  for (i = _children.begin(); i != _children.end(); i++) {
    delete *i;
  }
  _children.clear();
}

IParser* Parsers::createParserIfControlled(const string& dir_path) const {
  IParser *parser;
  list<IParser*>::const_iterator i;
  for (i = _children.begin(); i != _children.end(); i++) {
    parser = (*i)->createChildIfControlled(dir_path);
    if (parser != NULL) {
      return parser;
    }
  }
  return NULL;
}

void Parsers::show(int level) const {
  list<IParser*>::const_iterator i;
  for (i = _children.begin(); i != _children.end(); i++) {
    hlog_debug_arrow(level, "Parser: %s", (*i)->name());
  }
}

// ParsersManager

ParsersManager::~ParsersManager() {
  _children.clear();
  list<void*>::iterator dl_it;
  for (dl_it = _dls.begin(); dl_it != _dls.end(); ++dl_it) {
    dlclose(*dl_it);
  }
  _dls.clear();
}

int ParsersManager::loadPlugins(const char* path) {
  int rc = 0;
  Directory d(path);
  if (d.isValid()) {
    d.createList();
    const list<Node*>& nodes = d.nodesListConst();
    for (list<Node*>::const_iterator node_it = nodes.begin();
        node_it != nodes.end(); ++node_it) {
      size_t path_len = strlen((*node_it)->path());
      if (strcmp(&(*node_it)->path()[path_len - 3], ".so") == 0) {
        void* file_handle = dlopen((*node_it)->path(), RTLD_NOW);
        if (file_handle != NULL) {
          _dls.push_back(file_handle);
          void* manifest_handle = dlsym(file_handle, "manifest");
          if (manifest_handle != NULL) {
            ParserManifest* manifest =
              static_cast<ParserManifest*>(manifest_handle);
            _children.push_back(manifest->parser);
            ++rc;
            hlog_regression("DBG manifest %s", manifest->parser->name());
          } else {
            dlclose(file_handle);
          }
        }
      }
    }
  }
  return rc;
}

IParser* ParsersManager::createParser(
    const string&   name,
    const string&   mode_str) const{
  IParser::Mode mode;

  /* Determine mode */
  switch (mode_str[0]) {
    case 'c':
      // All controlled files
      mode = IParser::controlled;
      break;
    case 'l':
      // Local files
      mode = IParser::modifiedandothers;
      break;
    case 'm':
      // Modified controlled files
      mode = IParser::modified;
      break;
    case 'o':
      // Non controlled files
      mode = IParser::others;
      break;
    default:
      hlog_error("Undefined parser mode '%s'", mode_str.c_str());
      return NULL;
  }

  /* Create instance of specified parser */
  list<IParser*>::const_iterator i;
  for (i = _children.begin(); i != _children.end(); i++) {
    if ((*i)->code() == name) {
      return (*i)->createInstance(mode);
    }
  }
  hlog_error("Unsupported parser '%s'", name.c_str());
  return NULL;
}

void ParsersManager::show(int level) const {
  if (_children.empty()) {
    return;
  }
  hlog_debug_arrow(level, "Parsers found:");
  list<string> codes_found;
  list<IParser*>::const_iterator i;
  for (i = _children.begin(); i != _children.end(); i++) {
    bool found = false;
    for (list<string>::iterator it = codes_found.begin();
         it != codes_found.end(); ++it) {
      if (*it == (*i)->code()) {
        found = true;
        break;
      }
    }
    if (! found) {
      hlog_debug_arrow(level + 1, "%s (%s)", (*i)->code(), (*i)->name());
    }
    codes_found.push_back((*i)->code());
  }
}
