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

#ifndef DB_H
#define DB_H

namespace hbackup {

class Database {
  struct        Private;
  Private*      _d;
  int  convertList();
  int  lock();
  void unlock();
  int  merge();
  int  update(
    string          name,
    bool            new_file = false);
protected: // So I can test them/use them in tests
  bool isOpen() const;
  bool isWriteable() const;
  // Scan database for missing/corrupted data, return a list of valid checksums
  int crawl(
    Directory&      dir,              // Base directory
    const string&   checksumPart,     // Checksum part
    bool            thorough,         // Whether to do a corruption check
    list<string>&   checksums) const; // List of collected checksums
public:
  Database(const string& path);
  ~Database();
  // Open database
  int  open_ro();
  int  open_rw();
  // Close database (put removed data into trash)
  int  close(int trash_expire = -1);
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
  // Scan database for missing/corrupted (if thorough) data
  // If checksum is empty, scan all contents
  int  scan(
    bool            thorough = true,
    const string&   checksum = "") const;
  // Set the current client and its expiration delay (seconds)
  void setClient(
    const char*     client,
    time_t          expire = -1);
  // Tell DB that this client failed (skip last records)
  void failedClient();
  // Send data for comparison
  int  sendEntry(
    const char*     remote_path,      // Dir where the file resides, remotely
    const Node*     node,             // File metadata
    Node**          db_node);         // DB File metadata
  // Add entry to journal/list
  int  add(
    const char*     full_path,        // File path on client
    const Node*     node,             // File metadata
    const char*     checksum = NULL,  // Do not copy data, use given checksum
    int             compress = 0);    // Compression level for regular files
};

}

#endif
