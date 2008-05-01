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

#ifndef DATA_H
#define DATA_H

namespace hbackup {

class Data {
  struct            Private;
  Private* const    _d;
protected: // So I can test them
  // Get path for given checksum
  int getDir(
    const string&   checksum,
    string&         path,
    bool            create = false) const;
  // Make sure we don't have too many files per directory
  int organise(
    const char*     path,
    int             number) const;
  // Scan database for missing/corrupted data, return a list of valid checksums
  int crawl_recurse(
    Directory&      dir,              // Base directory
    const string&   checksumPart,     // Checksum part
    list<string>*   checksums,        // List of collected checksums
    bool            thorough,         // Whether to check for data corruption
    bool            remove,           // Whether to remove damaged data
    unsigned int*   valid,            // Number of valid data files found
    unsigned int*   broken) const;    // Number of broken data files found
public:
  Data();
  ~Data();
  // Open
  int open(
    const char*     path,
    bool            create = false);
  // Close
  void close();
  // Set progress callback function
  void setProgressCallback(progress_f progress);
  // Set the path to the temporary data (default is $(path)/temp)
  // Note: this path MUST be on the same filesystem as $(path)
  int setTemp(const char* path);
  // Get file name for given checksum
  int name(
    const char*     checksum,
    string&         path,
    string&         extension) const;
  // Read file with given checksum, extract it to path
  int read(
    const char*     path,
    const char*     checksum) const;
  // Add new item to database
  int write(                          // <0: error, >0: written, =0: no need
    const char*     path,             // Path to read from
    char**          checksum,         // Copy checksum here
    int             compress = 0);    // Compression to apply
  // Check existence/consistence of given checksum's data
  int check(
    const char*     checksum,
    bool            thorough     = true,
    bool            remove       = false) const;
  // Remove given checksum's data
  int remove(
    const char*     checksum);
  // Scan database for missing/corrupted data, return a list of valid checksums
  int crawl(
    list<string>*   checksums,        // List of collected checksums
    bool            thorough,         // Whether to check for data corruption
    bool            remove) const;    // Whether to remove damaged data
};

}

#endif
