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
  string        _path;
  list<string>  _active_checksums;
  int  lock();
  void unlock();
  int  merge();
protected: // So I can test them/use them in tests
  bool isOpen() const;
  bool isWriteable() const;
  int getDir(
    const string&   checksum,
    string&         path,
    bool            create = false);
  int  organise(
    const string&   path,
    int             number);
  int  write(
    const string&   path,
    char**          checksum,
    int             compress = 0);
public:
  Database(const string& path);
  ~Database();
  string path() const { return _path; }
  // Open database
  int  open(bool read_only = false);
  // Close database
  int  close();
  // Get list of prefixes in DB list (close-open to re-use DB!)
  int  getPrefixes(list<string>& prefixes);
  // Restore specified data
  int  restore(
    const char*     dest,             // Where the restored path goes
    const char*     prefix,           // The prefix to restore
    const char*     path = NULL,      // The path to restore (all)
    time_t          date = 0);        // The date to restore (latest)
  // Read file with given checksum, extract it to path
  int  read(
    const string&   path,
    const string&   checksum);
  // Scan database for missing/corrupted (if thorough) data
  // If checksum is empty, scan all contents
  int  scan(
    const string&   checksum = "",
    bool            thorough = true);
  // Set the current prefix
  void setPrefix(
    const char*     prefix);
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
