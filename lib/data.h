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
  // Get size of orginal data for given checksum path (-1 means failed)
  int getMetadata(
    const char*     path,
    long long*      size,
    char*           comp_status) const;
  // Set size of orginal data for given checksum path
  int setMetadata(
    const char*     path,
    long long       size,
    char            comp_status) const;
  // Remove given path
  int removePath(const char* path) const;
  // Make sure we don't have too many files per directory
  int organise(
    const char*     path,
    int             number) const;
  // Scan database for missing/corrupted data, return a list of valid checksums
  int crawl_recurse(
    Directory&      dir,              // Base directory
    const string&   checksum_part,    // Checksum part
    list<CompData>* data,             // List of collected data
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
    Stream&         source,           // Stream to read from
    const char*     temp_name,        // Name for temporary data file
    char**          checksum,         // Copy checksum here
    int             compress  = 0,    // Compression to apply (negative: never)
    bool            auto_comp = false,// Choose whether to store compressed
    int*            acompress = NULL, // Compression actually applied
    bool            src_open  = true) const;// Open source file
  // Check existence/consistence of given checksum's data
  int check(
    const char*     checksum,
    bool            thorough   = true,
    bool            repair     = false,
    long long*      size       = NULL,
    bool*           compressed = NULL) const;
  // Remove given checksum's data
  int remove(
    const char*     checksum) const;
  // Scan database for missing/corrupted data, return a list of valid checksums
  int crawl(
    bool            thorough,         // Whether to check for data corruption
    bool            repair,           // Whether to repair/remove damaged data
    list<CompData>* data = NULL) const;// List of collected data
};

}

#endif
