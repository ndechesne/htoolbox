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
  // Merge journal in case of failure recovery
  int  merge();
  // Shuffle file names around
  int  update(
    const char*     name,
    bool            new_file = false) const;
  // Close database in read-only mode
  int  close_ro();
  // Close database in read/write mode
  int  close_rw();
protected: // So I can test them/use them in tests
  bool isOpen() const;
  bool isWriteable() const;
public:
  // Method to convert from version '3' to version '4a' of DB list
  int convertList(list<string>* clients = NULL);
  Database(
    const char*     path);                // Path where DB resides
  ~Database();
  // Open database in read-only mode
  int  open_ro();
  // Open database in read/write mode
  int  open_rw(
    bool            initialize = false);  // Whether to initialize DB if needed
  // Close database
  int  close();
  // Get list of clientes in DB list (close-open to re-use DB!)
  int  getRecords(
      list<string>& records,          // List of elements to display
      const char*   client  = NULL,   // The client (list clientes)
      const char*   path    = NULL,   // The path (list paths)
      time_t        date    = 0);     // The date (latest)
  // Restore specified data
  int  restore(
    const char*     dest,             // Where the restored path goes
    const char*     client,           // The client to restore
    const char*     path = NULL,      // The path to restore (all)
    time_t          date = 0);        // The date to restore (latest)
  // Scan database for missing/obsolete data
  int  scan(
    bool            remove   = true) const; // Whether to remove obsolete data
  // Scan database for corrupted data
  int  check(
    bool            remove   = false) const;// Whether to remove corrupted data
  // Open list / journal for given client
  int  openClient(
    const char*     client,           // Client to deal with
    time_t          expire = -1);     // Client removed data expiration
  // Close list / journal for current client, can signal if error occurred
  void closeClient(
    bool            abort = false);   // Whether to remove remaining items
  // Send data for comparison
  int  sendEntry(
    const char*     remote_path,      // Dir where the file resides, remotely
    const Node*     node,             // File metadata
    char**          checksum = NULL); // Checksum from current file
  // Add entry to journal/list
  int  add(
    const char*     full_path,        // File path on client
    const Node*     node,             // File metadata
    const char*     checksum = NULL,  // Do not copy data, use given checksum
    int             compress = 0);    // Compression level for regular files
};

}

#endif
