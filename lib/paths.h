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

#ifndef _PATHS_H
#define _PATHS_H

namespace hbackup {

class ClientPath : public ConfigObject {
  char*             _path;
  size_t            _path_len;
  Attributes        _attributes;
  const ParsersManager& _parsers_manager;
  Parsers           _parsers;
  const Filter*     _compress;
  const Filter*     _no_compress;
  int               _nodes;
  int parse_recurse(
    Database&       db,
    const char*     remote_path,      // Dir where the file resides, remotely
    const char*     client_name,
    size_t          start,
    Node&           dir,
    IParser*        parser);
public:
  ClientPath(
    const char* path,
    const Attributes& attributes,
    const ParsersManager& parsers_manager);
  ~ClientPath();
  virtual ConfigObject* configChildFactory(
    const vector<string>& params,
    const char*           file_path = NULL,
    size_t                line_no   = 0);
  const char* path() const { return _path; }
  int nodes() const { return _nodes; }
  // For tests
  Attributes& attributes() { return _attributes; }
  int addParser(
    const string&   name,
    const string&   mode);
  int parse(
    Database&       db,
    const char*     backup_path,
    const char*     client_name);
  /* For verbosity */
  void show(int level = 0) const;
};

}

#endif
