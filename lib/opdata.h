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
  char              _letter;          // Letter showing current operation
  int               _id;              // Missing checksum ID
  int               _compression;     // Compression level for regular files
  const char*       _path;            // Real file path, on client
  Node&             _node;            // File metadata
  bool              _get_checksum;    // Metadata exists but checksum missing
public:
  // Pointers given to the constructor MUST remain valid during operation!
  OpData(
    const char*     path,             // Real file path, on client
    Node&           node)             // File metadata
    : _letter(0), _id(-1), _compression(0), _path(path), _node(node),
      _get_checksum(false) {}
  void setCompression(int compression) { _compression = compression; }
  bool needsAdding() { return _letter != 0; }
};

}

#endif
