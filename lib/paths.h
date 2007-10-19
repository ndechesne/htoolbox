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
  StrPath         _path;
  Directory*      _dir;
  Parsers         _parsers;
  list<Filter2*>  _filters2;
  Filter2*        _ignore;
  Filter2*        _compress;
  int             _expiration;
  int             _nodes;
  Filter2* findFilter(const string& name) const;
  int recurse(
    Database&       db,
    const char*     remote_path,      // Dir where the file resides, remotely
    const char*     local_path,       // Dir where the file resides, locally
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
    const string& name);
  // Set compress filter
  int setCompress(
    const string& name);
  // Set append to true to add as condition to last added filter
  int addFilter(
    const string& type,
    const string& name);
  int addCondition(
    const string& type_str,
    const string& value);
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
