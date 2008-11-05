/*
     Copyright (C) 2008  Herve Fache

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

#ifndef _OPDATA_H
#define _OPDATA_H

namespace hbackup {

class OpData {
  friend class Database;
  friend class Owner;
  char              _operation;       // Letter showing current operation
  char              _type;            // Letter showing concerned type
  char              _info;            // Letter showing internal information
  int               _id;              // Missing checksum ID
  int               _compression;     // Compression level for regular files
  const char*       _path;            // Real file path, on client
  Node&             _node;            // File metadata
  bool              _same_list_entry; // Don't add a list entry, replace
public:
  // Pointers given to the constructor MUST remain valid during operation!
  OpData(
    const char*     path,             // Real file path, on client
    Node&           node)             // File metadata
    : _operation(0), _type(' '), _info(' '), _id(-1), _compression(0),
      _path(path), _node(node), _same_list_entry(false) {}
  void setCompression(int compression) { _compression = compression; }
  int compression()  { return _compression; }
  bool needsAdding() { return _operation != 0; }
  void verbose(char* code) {
    code[0] = _operation;
    code[2] = _type;
    code[4] = _info;
  }
};

}

#endif
