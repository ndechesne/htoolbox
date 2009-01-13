/*
     Copyright (C) 2007-2008  Herve Fache

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

class List;

class Register {
  struct            Private;
  Private* const    _d;
  friend class List;
  // Buffer relevant line
  ssize_t fetchLine();
public:
  Register(
    Path            path);
  ~Register();
  // Open file (for read)
  int open(List* new_list = NULL, List* journal = NULL);
  // Close file
  int close();
  // File path
  const char* path() const;
  // For progress information
  void setProgressCallback(progress_f progress);
  // Empty list (check right after opening)
  bool isEmpty() const;
  // Something was journalled
  bool isModified() const;
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
    const char      path[]      = NULL,     // Path to search
    time_t          expire      = -1,       // Expiration date
    time_t          remove      = 0);       // Mark records removed at date
  // Add info
  int add(
    const Path&     path,
    const Node*     node);
  // Show the list
  void show(
    time_t          date        = -1,       // Date to select
    time_t          time_start  = 0,        // Origin of time
    time_t          time_base   = 1);       // Time base
  // Encode line from metadata
  static ssize_t encodeLine(
    char*           line[],
    time_t          timestamp,
    const Node*     node);
  // Decode metadata from line
  static int decodeLine(
    const char      line[],             // Line to decode
    time_t*         ts,                 // Line timestamp
    const char      path[]     = NULL,  // File path to store in metadata
    Node**          node       = NULL); // File metadata
  // Write a line, adding the LF character
  static ssize_t putLine(int fd, const Line& line);
};

class List {
  Path              _path;
  int               _stream;
  friend class Register;
public:
  List(Path path) : _path(path) {}
  // Open file, for read or write (no append), with compression (cf. Stream)
  int create();
  // Close file
  int finalize();
  // File path
  const char* path() const  { return _path;       }
  // Merge list and journal into this list
  //    all lists must be open
  // Return code:
  //    -1: error, 0: success, 1: unexpected end of journal
  int merge(
    Register&       list,
    Register&       journal);
  // add line for tests
  int addLine(
    const char      line[]);
};

}

#endif
