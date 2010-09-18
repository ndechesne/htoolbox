/*
     Copyright (C) 2007-2010  Herve Fache

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

#ifndef _LIST_H
#define _LIST_H

#include <stdio.h>
#include <limits.h>
#include <time.h>

#include <string>

namespace hbackup {

class List {
public:
  enum Type {
    list_read,
    list_write,
    journal_write
  };
private:
  std::string _path;
  Type _type;
  FILE* _fd;
  progress_f _progress;
  long long _size;
  long long _previous_offset;
  long long _offset;
  static const long long _offset_threadshold = 102400;
  bool _old_version;
  char _file_path[PATH_MAX + 1];
  time_t _removed;
  const char* _header() { return "# version 5"; }
  const char* _old_header() { return "# version 4\n"; }
  const char* _footer() { return "# end"; }
  ssize_t putLine(const char* line);
  inline ssize_t flushStored();
public:
  List(const char* path, Type type) : _path(path), _type(type), _fd(NULL),
      _progress(NULL), _size(-1), _previous_offset(0), _offset(0),
      _old_version(false), _removed(0) {
    _file_path[0] = '\0';
  }
  ~List() { if (_fd != NULL) close(); }
  const char* path() const { return _path.c_str(); }
  int open(bool quiet_if_not_exists = false);
  int close();
  void setProgressCallback(progress_f progress);
  int flush() const { return fflush(_fd); }
  ssize_t putPath(const char* path);              // file path
  ssize_t putData(const char* ts_metadata_extra); // pre-encoded line
  ssize_t putData(time_t ts, const char* metadata, const char* extra);
  ssize_t getLine(char** buffer_p, size_t* cap_p);
  // Encode line from metadata
  static size_t encode(
    const Node&     node,
    char*           line,
    size_t*         sep_offset_p);
  // Decode metadata from line
  static int decodeLine(
    const char*     line,           // Line to decode
    time_t*         ts,             // Line timestamp
    Node**          node,           // File metadata
    const char*     path = "");   // Optional file path to store in metadata
  // Decode time stamp from line
  static int decodeTimeStamp(
    const char*     line,           // Line to decode from
    time_t*         ts);            // Time stamp
  // Decode type from line
  static int decodeType(
    const char*     line,           // Line to decode from
    char*           type);          // Type as one letter
  // Decode type from line
  static const int MAX_CHECKSUM_LENGTH = 64;
  static int decodeSizeChecksum(
    const char*     line,           // Line to decode from
    long long*      size,           // Size
    char            checksum[MAX_CHECKSUM_LENGTH + 1]);  // Checksum or empty
};

class ListReader {
  struct            Private;
  Private* const    _d;
public:
  enum Status {
    eof         = -2,         // unexpected end of file
    failed      = -1,         // an error occured
    eor         = 0,          // end of records
    got_nothing,              // No relevant data
    got_path,                 // Got a path
    got_data                  // Got data
  };
  ListReader(
    const Path&     path);
  ~ListReader();
  // Open file (for read)
  int open(bool quiet_if_not_exists = false);
  // Close file
  int close();
  // File path
  const char* path() const;
  // Current path
  const char* getPath() const;
  // Current data line
  const char* getData() const;
  // For progress information
  void setProgressCallback(
    progress_f      progress);
  // Buffer relevant line
  ListReader::Status fetchLine();
  // Reset status to 'no data available'
  void resetStatus();
  // End of list reached
  bool end() const;
  const char* fetchData();
  int fetchSizeChecksum(
    long long*      size,
    char            checksum[List::MAX_CHECKSUM_LENGTH + 1]);
  int searchPath();
  // Convert one or several line(s) to data
  // Date:
  //    <0: any
  //     0: latest
  //    >0: as old or just newer than date
  // Return code:
  //    -1: error, 0: end of file, 1: success
  int getEntry(
    time_t*         timestamp,
    char            path[],
    Node**          node,
    time_t          date);
  // Show the list
  void show(
    time_t          date        = -1,       // Date to select
    time_t          time_start  = 0,        // Origin of time
    time_t          time_base   = 1);       // Time base
};

class ListWriter : public List {
public:
  ListWriter(
    const Path&     path,
    bool            journal = false)
      : List(path, journal ? journal_write : list_write) {}
  // Search data in list copying contents to new list/journal, marking files
  // removed, and also expiring data on the fly when told to
  // Searches:             Path        Copy
  //    given path         path        all up to path (appended if not found)
  //    next path          NULL        all before path
  //    end of file        ""          all
  // Expiration:
  //    -1: no expiration, 0: only keep last, otherwise use given date
  // Return code:
  //    -1: error, 0: end of file, 1: exceeded, 2: found
  int search(
    ListReader*     list,             // List to read from
    const char*     path,             // Path to search
    time_t          expire,           // Expiration date
    time_t          remove,           // Mark records removed at date
    ListWriter*     journal = NULL,   // Journal
    bool*           modified = NULL); // To report list modifications
  // Simplified search for merge
  static int copy(
    ListWriter&     final,            // Final list
    ListReader&     list,             // Original list
    const char*     path);            // Path to search
  // Merge given list and journal into this list
  //    all lists must be open
  // Return code:
  //    -1: error, 0: success, 1: unexpected end of journal
  static int merge(
    ListWriter&     final,            // Final list
    ListReader&     list,             // Original list
    ListReader&     journal);         // Journal to merge
};

}

#endif // _LIST_H
