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

#ifndef _OWNER_H
#define _OWNER_H

namespace hbackup {

class Owner {
  struct            Private;
  Private* const    _d;
  int finishOff(
    bool            recovery);            // Data is being recovered
public:
  Owner(
    const char*     path,                 // DB base path
    const char*     name,                 // Name of owner
    time_t          expiration = -1);     // Cut-off date for removed data (s)
  ~Owner();
  const char* name() const;
  const char* path() const;
  time_t expiration() const;
  int hold() const;                       // Open for read only
  int release() const;                    // Close when open for read only
  int open(                               // Open for read and write
    bool            initialize = false,   // Initialize if does not exist
    bool            check      = false);  // Check only (no need to close)
  int close(                              // Close, whatever how open
    bool            teardown);
  // Set progress callback function
  void setProgressCallback(progress_f progress);
  int search(
    const char*     path,                 // Path to look for, NULL for all
    Node**          node) const;          // Node found
  // Add entry info to journal
  int add(
    const char*     path,
    const Node*     node,
    time_t          timestamp);
  // Get next record matching path and date
  int getNextRecord(                      // -1: error, 0: eof, 1: success
    const char*     path,                 // The base path
    time_t          date,                 // The date (epoch, 0: all, <0: relative)
    char**          db_path,              // The path found
    Node**          db_node = NULL) const;// The metadata returned
  int getChecksums(
    list<string>&   checksums) const;     // List to add checksums
};

}

#endif
