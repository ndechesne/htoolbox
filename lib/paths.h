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

#ifndef _PATHS_H
#define _PATHS_H

namespace hbackup {

class ClientPath {
  Path              _path;
  Parsers           _parsers;
  Filters           _filters;
  const Filter*     _ignore;
  const Filter*     _compress;
  int               _nodes;
  int parse_recurse(
    Database&       db,
    const char*     remote_path,      // Dir where the file resides, remotely
    int             start,
    Directory&      dir,
    Parser*         parser);
public:
  ClientPath(const char* path);
  const char* path() const               { return _path;  }
  int nodes() const                      { return _nodes; }
  // Set ignore filter
  void setIgnore(const Filter* filter)   { _ignore   = filter; }
  // Set compress filter
  void setCompress(const Filter* filter) { _compress = filter; }
  Filter* addFilter(
    const string&   type,
    const string&   name);
  Filter* findFilter(
    const string&   name) const;
  int addParser(
    const string&   type,
    const string&   string);
  int parse(
    Database&       db,
    const char*     backup_path);
  /* For verbosity */
  void show(int level = 0) const;
};

}

#endif
