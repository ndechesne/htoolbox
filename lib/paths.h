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

#ifndef PATHS_H
#define PATHS_H

namespace hbackup {

class Path {
  StrPath     _path;
  Directory*  _dir;
  Parsers     _parsers;
  Filters     _filters;
  int         _expiration;
  int         _nodes;
  int recurse(
    Database&       db,
    const char*     remote_path,      // Dir where the file resides, remotely
    const char*     local_path,       // Dir where the file resides, locally
    Directory*      dir,
    Parser*         parser);
  void recurse_remove(
    Database&       db,
    const char*     remote_path,      // Dir where the file resides, remotely
    const Node*     node);
public:
  Path(const char* path);
  ~Path() {
    delete _dir;
  }
  const char* path() const     { return _path.c_str(); }
  const Directory* dir() const { return _dir;          }
  int expiration() const       { return _expiration;   }
  int nodes() const            { return _nodes;        }
  void setExpiration(int expiration) { _expiration = expiration; }
  // Set append to true to add as condition to last added filter
  int addFilter(
    const string& type,
    const string& string,
    bool          append = false);
  int addParser(
    const string& type,
    const string& string);
  int parse(
    Database&   db,
    const char* backup_path);
  // Information
  void showParsers() {
    _parsers.list();
  }
};

}

#endif
