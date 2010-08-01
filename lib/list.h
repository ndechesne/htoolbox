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

namespace hbackup {

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
  // For progress information (NOT IMPLEMENTED)
  void setProgressCallback(
    progress_f      progress);
  // Buffer relevant line
  ListReader::Status fetchLine();
  // Reset status to 'no data available'
  void resetStatus();
  // End of list reached
  bool end() const;
  // Encode line from metadata
  static ssize_t encodeLine(
    char*           line[],                 // Line to decode
    time_t          ts,                     // Line timestamp
    const Node*     node);                  // File path and metadata
  // Decode metadata from line
  static int decodeLine(
    const char      line[],                 // Line to decode
    time_t*         ts,                     // Line timestamp
    const char      path[]      = NULL,     // File path to store in metadata
    Node**          node        = NULL);    // File metadata
  // Convert one or several line(s) to data
  // Date:
  //    <0: any (if -2, data is not discarded)
  //     0: latest
  //    >0: as old or just newer than date
  // Return code:
  //    -1: error, 0: end of file, 1: success
  int getEntry(
    time_t*         timestamp,
    char*           path[],
    Node**          node,
    time_t          date = -1);
  // Show the list
  void show(
    time_t          date        = -1,       // Date to select
    time_t          time_start  = 0,        // Origin of time
    time_t          time_base   = 1);       // Time base
};

class ListWriter {
  struct            Private;
  Private* const    _d;
public:
  ListWriter(
    const Path&     path);
  ~ListWriter();
  // Open file
  int open();
  // Close file
  int close();
  // File path
  const char* path() const;
  // Add line to file
  ssize_t putLine(const char* line);
  // Flush all data to file
  int flush();
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
  static int search(
    ListReader*     list,
    const char      path[]      = NULL,   // Path to search
    time_t          expire      = -1,     // Expiration date
    time_t          remove      = 0,      // Mark records removed at date
    ListWriter*     new_list    = NULL,   // Merge list
    ListWriter*     journal     = NULL,   // Journal
    bool*           modified    = NULL);  // To report list modifications
  // Simplified search for merge
  int copy(
    ListReader*     list,                 // Original list
    const char*     path);                // Path to search
  // Merge given list and journal into this list
  //    all lists must be open
  // Return code:
  //    -1: error, 0: success, 1: unexpected end of journal
  int merge(
    ListReader*     list,
    ListReader*     journal     = NULL);
};

}

#endif // _LIST_H
