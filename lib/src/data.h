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

namespace hbackup {

class Data {
  struct            Private;
  Private* const    _d;
public:
  enum CompressionCase {
    db_no = 'd',      // global no
    db_yes = 'r',     // global yes
    forced_no = '-',  // filter no
    forced_yes = 'f', // filter yes
    size_no = '.',    // auto no
    size_yes = '+',   // auto yes
    auto_now = 'a',   // auto need to check
    auto_later = ' ', // auto might check later
    unknown = '?',    // not decided
  };
  // Data collector for the crawler
  class Collector {
  public:
    virtual void add(const char* h, long long d, long long f) = 0;
  };
protected: // So I can test them
  // Compare two files
  ssize_t compare(
    htools::IReaderWriter&  left,
    htools::IReaderWriter&  right) const;
  // Copy file to one or two destinations
  long long copy(
    htools::IReaderWriter*  source,
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
  int removePath(const char* path, const char* hash) const;
  // Check existence/consistence of given checksum's data
  int check(
    const char*     checksum,
    bool            thorough  = true,
    bool            repair    = false,
    long long*      data_size = NULL,
    long long*      file_size = NULL) const;
public:
  Data(
    const char*     path,
    Backup&         backup);
  ~Data();
  // Open
  int open(
    bool            create = false);
  // Set progress callback function
  void setProgressCallback(progress_f progress);
  // Get file name for given checksum
  int name(
    const char*     checksum,
    char*           path,             // Needs to be large enough! [PATH_MAX]
    char*           extension) const; // Needs to be large enough! [5 should do]
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
    CompressionCase auto_case,        // Choose whether to store compressed
    char*           store_path) const;
  // Remove given checksum's data
  int remove(
    const char*     checksum) const;
  // Scan database for missing/corrupted data, return a list of valid checksums
  int crawl(
    bool            thorough,         // Whether to check for data corruption
    bool            repair,           // Whether to repair/remove damaged data
    Collector*      collector = NULL) const; // Collector to give data to
};

}

#endif // _DATA_H
