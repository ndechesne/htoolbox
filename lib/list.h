/*
     Copyright (C) 2007-2009  Herve Fache

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

#ifndef LIST_H
#define LIST_H

namespace hbackup {

class List {
  struct            Private;
  Private* const    _d;
  // Read a line, removing the LF character
  ssize_t getLine(
    Line&           line,                   // Line to read
    bool*           eol);                   // Whether EOL was found
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
  ~List();
  // Open file (for write)
  int create();
  // Open file (for read)
  int open();
  // Close file
  int close();
  // Flush all data to file
  int flush();
  // File path
  const Path& path() const;
  // Current path
  const Path& getPath() const;
  // Current data line
  const Line& getData() const;
  // For progress information
  void setProgressCallback(
    progress_f      progress);
  // Buffer relevant line
  List::Status fetchLine(bool init = false);
  // Reset status to 'no data available'
  void resetStatus();
  // End of list reached
  bool end() const;
  // Write a line, adding the LF character
  ssize_t putLine(
    const Line&     line);                  // Line to write
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
  // Add info
  static int add(
    const Path&     path,                   // Path
    const Node*     node,                   // Metadata
    List*           new_list,               // Merge list
    List*           journal     = NULL);    // Journal
  // Empty list (only valid right after opening)
  static bool isEmpty(
    const List*     list);
  // Convert one or several line(s) to data
  // Date:
  //    <0: any (if -2, data is not discarded)
  //     0: latest
  //    >0: as old or just newer than date
  // Return code:
  //    -1: error, 0: end of file, 1: success
  static int getEntry(
    List*           list,
    time_t*         timestamp,
    char*           path[],
    Node**          node,
    time_t          date = -1);
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
    List*           new_list    = NULL,     // Merge list
    List*           journal     = NULL,     // Journal
    bool*           modified    = NULL);    // To report list modifications
  // Merge this list and journal into new_list
  //    all lists must be open
  // Return code:
  //    -1: error, 0: success, 1: unexpected end of journal
  static int merge(
    List*           list,
    List*           new_list,
    List*           journal     = NULL);
  // Show the list
  void show(
    time_t          date        = -1,       // Date to select
    time_t          time_start  = 0,        // Origin of time
    time_t          time_base   = 1);       // Time base
};

}

#endif
