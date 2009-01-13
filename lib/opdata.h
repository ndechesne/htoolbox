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
public:
  enum CompressionStatus {
    maybe,
    never,
    automatic
  };
private:
  friend class Database;
  friend class Owner;
  char              _operation;       // Letter showing current operation
  char              _type;            // Letter showing concerned type
  char              _info;            // Letter showing internal information
  int               _id;              // Missing checksum ID
  CompressionStatus _comp_status;     // Compression decision
  int               _compression;     // Compression level for regular files
  const Path&       _path;            // Real file path, on client
  Node&             _node;            // File metadata
  bool              _same_list_entry; // Don't add a list entry, replace
public:
  // Pointers given to the constructor MUST remain valid during operation!
  OpData(
    const Path&     path,             // Real file path, on client
    Node&           node)             // File metadata
    : _operation(' '), _type(' '), _info(' '), _id(-1), _comp_status(maybe),
      _compression(0), _path(path), _node(node), _same_list_entry(false) {}
  void setCompressionStatus(CompressionStatus s) { _comp_status = s; }
  CompressionStatus compressionStatus() const { return _comp_status; }
  void setCompression(int compression) { _compression = compression; }
  int compression() const              { return _compression;        }
  bool sameListEntry() const           { return _same_list_entry;    }
  bool needsAdding() const             { return _operation != ' ';   }
  void verbose(char* code) {
    // File information
    code[0] = _operation;
    code[1] = _node.type();
    // Database information
    code[3] = _type;
    code[4] = _info;
  }
};

}

#endif
