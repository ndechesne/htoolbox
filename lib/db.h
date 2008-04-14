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

#ifndef DB_H
#define DB_H

namespace hbackup {

class Database {
  struct        Private;
  Private*      _d;
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
  // Set progress callback function
  void setProgressCallback(progress_f progress);
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
    const char*     path = "",            // The path to restore (all)
    time_t          date = 0);            // The date to restore (all)
  // Scan database for missing/obsolete data
  int  scan(
    bool            remove   = true);     // Whether to remove obsolete data
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
  // Data for operation
  class OpData {
    friend class Database;
    char            _letter;          // Letter showing current operation
    int             _id;              // Missing checksum ID
    int             _compression;     // Compression level for regular files
    const char*     _path;            // Real file path, on client
    Node&           _node;            // File metadata
    bool            _get_checksum;    // Metadata exists but checksum missing
  public:
    // Pointers given to the constructor MUST remain valid during operation!
    OpData(
      const char*   path,             // Real file path, on client
      Node&         node)             // File metadata
     : _letter(0), _id(-1), _compression(0), _path(path), _node(node),
       _get_checksum(false) {}
    void setCompression(int compression) { _compression = compression; }
    bool needsAdding() { return _letter != 0; }
  };
  // Send data for comparison
  void sendEntry(
    OpData&         operation);       // Operation data
  // Add entry to journal/list
  int  add(
    const OpData&   operation);       // Operation data
};

}

#endif
