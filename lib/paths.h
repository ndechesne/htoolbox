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

#ifndef _PATHS_H
#define _PATHS_H

namespace hbackup {

class Path {
  StrPath     _path;
  Directory*  _dir;
  Parsers     _parsers;
  Filters     _filters;
  Filter*     _ignore;
  Filter*     _compress;
  int         _expiration;
  int         _nodes;
  int recurse(
    Database&       db,
    const char*     remote_path,      // Dir where the file resides, remotely
    Directory*      dir,
    Parser*         parser);
public:
  Path(const char* path);
  ~Path();
  const char* path() const     { return _path.c_str(); }
  const Directory* dir() const { return _dir;          }
  int expiration() const       { return _expiration;   }
  int nodes() const            { return _nodes;        }
  void setExpiration(int expiration) { _expiration = expiration; }
  // Set ignore filter
  int setIgnore(
    const string&   name);
  // Set compress filter
  int setCompress(
    const string&   name);
  Filter* addFilter(const string& type, const string& name) {
    return _filters.add(type, name);
  }
  Filter* findFilter(const string& name) const {
    return _filters.find(name);
  }
  int addParser(
    const string&   type,
    const string&   string);
  int parse(
    Database&       db,
    const char*     backup_path);
  // Information
  void showParsers() {
    _parsers.list();
  }
};

}

#endif
