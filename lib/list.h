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

class List {
protected:
  struct            Private;
  Private* const    _d;
  // Read a line, removing the LF character
  ssize_t getLine(
    Line&           line,             // Line to read
    bool            version = false); // First line that contains version
public:
  enum Status {
    eof         = -2,         // unexpected end of file
    failed      = -1,         // an error occured
    eor         = 0,          // end of records
    got_nothing,              // No relevant data
    got_path,                 // Got a path
    got_data                  // Got data
  };
  List(
    const Path&     path);
  virtual ~List();
  // Open file (for read)
  virtual int open();
  // Close file
  virtual int close();
  // File path
  const Path& path() const;
  // Current path
  const Path& getPath() const;
  // Current data line
  const Line& getData() const;
  // For progress information (NOT IMPLEMENTED)
  void setProgressCallback(
    progress_f      progress);
  // Buffer relevant line
  List::Status fetchLine(bool init = false);
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

class ListWriter : public List {
protected: // for tests...
  // Write a line, adding the LF character
  ssize_t putLine(
    const Line&     line);            // Line to write
public:
  ListWriter(
    const Path&     path) : List(path) {}
  // Open file
  int open();
  // Close file
  int close();
  // Flush all data to file
  int flush();
  // Add info
  int add(
    const Path&     path,             // Path
    const Node*     node,             // Metadata
    ListWriter*     journal = NULL);  // Journal
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
    List*           list,
    const char      path[]      = NULL,     // Path to search
    time_t          expire      = -1,       // Expiration date
    time_t          remove      = 0,        // Mark records removed at date
    ListWriter*     new_list    = NULL,     // Merge list
    ListWriter*     journal     = NULL,     // Journal
    bool*           modified    = NULL);    // To report list modifications
  // Merge given list and journal into this list
  //    all lists must be open
  // Return code:
  //    -1: error, 0: success, 1: unexpected end of journal
  int merge(
    List*           list,
    List*           journal     = NULL);
};

}

#endif // _LIST_H
