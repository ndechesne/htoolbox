/*
     Copyright (C) 2010  Herve Fache

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

#ifndef _OP_DATA_H
#define _OP_DATA_H

namespace hbackup {
  struct Database::OpData {
    char              operation;        // Letter showing current operation
    char              type;             // Letter showing concerned type
    char              info;             // Letter showing internal information
    int               id;               // Missing checksum ID
    Data::CompressionCase comp_case;  // Compression decision
    int               compression;      // Compression level for regular files
    const char*       path;             // Real file path, on client
    size_t            path_len;         // Its length
    htools::Node&     node;             // File metadata
    bool              same_list_entry;  // Don't add a list entry, replace
    // Pre-encoded node metadata, assumes the following max values:
    // * type: 1 char => 1
    // * size: 2^64 => 20
    // * mtime: 2^32 => 10
    // * uid: 2^32 => 10
    // * gid: 2^32 => 10
    // * mode: 4 octal digits => 4
    // * separators: 5 TABs => 5
    // Total = 60 => 64
    char              encoded_metadata[64];
    size_t            sep_offset;
    size_t            end_offset;
    const char*       extra;
    char              store_path[PATH_MAX]; // Where data is stored in DB
    // Pointers given to the constructor MUST remain valid during operation!
    OpData(
      const char*     p,                // Real file path, on client
      size_t          l,                // Its length
      htools::Node&   n)                // File metadata
      : operation(' '), type(' '), info(' '), id(-1),
        comp_case(Data::auto_later), compression(compression_level),
        path(p), path_len(l), node(n), same_list_entry(false), extra(NULL) {}
    bool needsAdding() const { return operation != ' '; }
    void verbose(char* code) {
      // File information
      code[0] = operation;
      code[1] = node.type();
      // Database information
      code[3] = type;
      code[4] = info;
    }
  };
};

#endif // _OP_DATA_H
