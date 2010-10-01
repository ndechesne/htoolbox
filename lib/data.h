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

#ifndef _DATA_H
#define _DATA_H

#include <ireaderwriter.h>
#include <stackhelper.h>

namespace hbackup {

class Data {
  struct            Private;
  Private* const    _d;
protected: // So I can test them
  enum CompressionCase {
    forced_no = '-',
    forced_yes = 'f',
    size_no = '.',
    size_yes = '+',
    later = ' ',
    unknown = '?'
  };
  static int findExtension(
    char*           path,
    const char*     extensions[]);
  static bool isReadable(
    const char*     path);
  static int touch(
    const char*     path);
  // Compare two files
  int compare(
    htools::IReaderWriter&  left,
    htools::IReaderWriter&  right) const;
  // Copy file to one or two destinations
  long long copy(
    htools::StackHelper*    source,
    htools::IReaderWriter*  dest1,
    htools::IReaderWriter*  dest2 = NULL) const;
  // Get path for given checksum
  int getDir(
    const char*     checksum,
    char*           path,
    bool            create = false) const;
  // Get size of orginal data for given checksum path (-1 means failed)
  int getMetadata(
    const char*     path,
    long long*      size,
    CompressionCase* comp_status) const;
  // Set size of orginal data for given checksum path
  int setMetadata(
    const char*     path,
    long long       size,
    CompressionCase comp_status) const;
  // Remove given path
  int removePath(const char* path) const;
  // Make sure we don't have too many files per directory
  int organise(
    const char*     path,
    int             number) const;
  // Check existence/consistence of given checksum's data
  int check(
    const char*     checksum,
    bool            thorough   = true,
    bool            repair     = false,
    long long*      size       = NULL,
    bool*           compressed = NULL) const;
  // Scan database for missing/corrupted data, return a list of valid checksums
  int crawl_recurse(
    Directory&      dir,              // Base directory
    const string&   checksum_part,    // Checksum part
    list<CompData>* data,             // List of collected data
    bool            thorough,         // Whether to check for data corruption
    bool            remove,           // Whether to remove damaged data
    size_t*         valid,            // Number of valid data files found
    size_t*         broken) const;    // Number of broken data files found
public:
  Data(
    const char*     path);
  ~Data();
  // Open
  int open(
    bool            create = false);
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
  // * compress:
  //    < 0  => never compress this file
  //    == 0 => do not compress this file now
  //    > 0  => compress now, or auto-compress (depends on auto_comp)
  enum WriteStatus {
    error   = -1,
    leave   = 0,
    add     = 1,
    replace = 2
  };
  WriteStatus write(
    const char*     path,             // Source file path
    char            checksum[64],     // Copy checksum here
    int*            comp_level,       // Comp. to apply (< 0: never) / applied
    bool            auto_comp) const; // Choose whether to store compressed
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

#endif // _DATA_H
