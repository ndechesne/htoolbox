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

#ifndef _DB_H
#define _DB_H

namespace hbackup {

class Database {
  struct            Private;
  Private* const    _d;
  // Try to lock DB
  int  lock();
  // Unlock DB
  void unlock();
  // Shuffle file names around
  int  update(
    const char*     name,
    bool            new_file = false) const;
public:
  Database(
    const char*     path);                // Path where DB resides
  ~Database();
  // Get DB path
  const char* path() const;
  // Open database
  int  open(
    bool            read_only  = false,   // Open for read only
    bool            initialize = false);  // Initialize DB if needed
  // Close database
  int  close();
  // Compression modes
  enum CompressionMode {
    always,
    auto_now,
    auto_later,
    never
  };
  // Set compression mode
  void setCompressionMode(
    CompressionMode mode);
  // Set copy progress callback function
  void setCopyProgressCallback(progress_f progress);
  // Set previous list read progress callback function
  void setListProgressCallback(progress_f progress);
  // Get list of clients in DB
  int getClients(
    list<string>&   clients) const;       // List of clients
  // Get list of paths and data from DB list (close-open to re-use DB!)
  int  getRecords(
    list<string>&   records,              // List of elements to display
    const char*     path    = "",         // The path (list paths)
    time_t          date    = 0);         // The date (latest)
  // Restore specified data
  int  restore(
    const char*     dest,                 // Where the restored path goes
    HBackup::LinkType links = HBackup::none, // Link type
    const char*     path  = "",           // The path to restore (all)
    time_t          date  = 0);           // The date to restore (all)
  // Scan database for missing/obsolete data
  int  scan(
    bool            remove   = true) const; // Whether to remove obsolete data
  // Scan database for corrupted data
  int  check(
    bool            remove   = false) const;// Whether to remove corrupted data
  // Open list / journal for given client
  int  openClient(
    const char*     client,               // Client to deal with
    time_t          expire = -1);         // Client removed data expiration
  // Close list / journal for current client, can signal if error occurred
  int  closeClient(
    bool            abort = false);       // Whether to remove remaining items
  // Class for data exchange
  struct OpData {
    char              operation;        // Letter showing current operation
    char              type;             // Letter showing concerned type
    char              info;             // Letter showing internal information
    int               id;               // Missing checksum ID
    Database::CompressionMode comp_mode;  // Compression decision
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
    string            store_path;       // Where data is stored in DB
    // Pointers given to the constructor MUST remain valid during operation!
    OpData(
      const char*     p,                // Real file path, on client
      size_t          l,                // Its length
      htools::Node&   n)                // File metadata
      : operation(' '), type(' '), info(' '), id(-1),
        comp_mode(Database::auto_later), compression(0),
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
  // Send data for comparison
  void sendEntry(
    OpData&         operation);           // Operation data
  // Add entry to journal/list
  int  add(
    OpData&         operation,            // Operation data
    bool            report_copy_error_once = false);
  // Make sure we have the file storage path info
  int  setStorePath(
    OpData&         operation);           // Operation data
};

}

#endif // _DB_H
