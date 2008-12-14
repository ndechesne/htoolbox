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

class ListReader : public Stream {
  struct            Private;
  Private* const    _d;
  friend class List;
  // Buffer relevant line
  ssize_t fetchLine(bool use_found = false);
public:
  ListReader(
    Path            path);
  ~ListReader();
  // Open file (for read)
  int open();
  // Version
  bool isOldVersion() const;
  // Empty list (check right after opening)
  bool isEmpty() const;
  // Put line into list buffer (will fail and return -1 if buffer in use)
  ssize_t putLine(const char* line);
  // Mark cached line for re-use
  void keepLine();
  // Get current line type (will get a new line if necessary)
  char getLineType();
  // Convert one or several line(s) to data
  // Date:
  //    -2: do not get lines, only use current
  //    -1: any
  //     0: latest
  //     *: as old or just newer than date
  // Return code:
  //    -1: error, 0: end of file, 1: success
  int getEntry(
    time_t*         timestamp,
    char*           path[],
    Node**          node,
    time_t          date = -1);
  // Search data in list copying contents on the fly if required, and
  // also expiring data and putting checksums in lists!!!
  // Searches:             Path        Copy
  //    given path         path        all up to path (appended if not found)
  //    next path          NULL        all before path
  //    end of file        ""          all
  // Expiration:
  //    -1: no expiration, 0: only keep last, otherwise use given date
  // Return code:
  //    -1: error, 0: end of file, 1: exceeded, 2: found
  int search(
    const char      path[]  = NULL,   // Path to search
    List*           list    = NULL,   // List in which to merge changes
    time_t          expire  = -1);    // Expiration date
  // Show the list
  void show(
    time_t          date       = -1,  // Date to select
    time_t          time_start = 0,   // Origin of time
    time_t          time_base  = 1);  // Time base
};

class List : public Stream {
  struct            Private;
  Private* const    _d;
  friend class ListReader;
  // Buffer relevant line
  ssize_t fetchLine(bool use_found = false);
  // Encode line from metadata
public:
  static int encodeLine(
    char*           line[],
    time_t          timestamp,
    const Node*     node);
  // Decode metadata from line
  static int decodeLine(
    const char      line[],             // Line to decode
    time_t*         ts,                 // Line timestamp
    const char      path[]     = NULL,  // File path to store in metadata
    Node**          node       = NULL); // File metadata
  List(
    Path            path);
  ~List();
  // Open file, for read or write (no append), with compression (cf. Stream)
  int open();
  // Close file
  int close();
  // Nothing was journaled
  bool isEmpty() const;
  // Put line into list buffer (will fail and return -1 if buffer in use)
  ssize_t putLine(const char* line);
  // Add entry to list
  int add(
    const char      path[],
    time_t          timestamp = -1,
    const Node*     node      = NULL,
    bool            bufferize = false);
  // Merge list and journal into this list
  //    all lists must be open (checked)
  // Return code:
  //    -1: error, 0: success, 1: unexpected end of journal
  int  merge(
    ListReader&     list,
    ListReader&     journal);
};

}

#endif
