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
  // Method to update the DB organization and list formats
  int convert(list<string>* clients = NULL);
  Database(
    const char*     path);                // Path where DB resides
  ~Database();
  // Open database
  int  open(
    bool            read_only  = false,   // Open for read only
    bool            initialize = false);  // Initialize DB if needed
  // Close database
  int  close();
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
    time_t          date = 0);            // The date to restore (latest)
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
