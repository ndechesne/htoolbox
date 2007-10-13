/*
     Copyright (C) 2007  Herve Fache

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

class List : public Stream {
  string            _line;
  // _line_status meaning:
  //  -2: unexpected end of file
  //  -1: error
  //   0: empty list
  //   1: line contains no valid data
  //   2: line contains found data, will only re-used in getEntry
  //   3: line contains data to be re-used
  int               _line_status;
  // Need to keep prefix status for search()
  int               _prefix_cmp;
  // Decode metadata from current line
  int decodeLine(
    const char*     path,
    Node**          node,
    time_t*         timestamp);
public:
  List(
    const char*     dir_path,
    const char*     name = "") :
    Stream(dir_path, name) {}
  // Open file, for read or write (no append), with compression (cf. Stream)
  int open(
    const char*     req_mode,
    int             compression = 0);
  // Close file
  int close();
  // Empty file (check right after opening for read)
  bool isEmpty() const { return _line_status == 0; }
  // Get relevant line
  ssize_t getLine(bool use_found = false);
  // Put line into list buffer (will fail and return -1 if buffer in use)
  ssize_t putLine(const char* line);
  // Mark current line for re-use
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
    char**          prefix,
    char**          path,
    Node**          node,
    time_t          date = -1);
  // Add prefix to list
  int prefix(
    const char*     prefix);
  // Add path to list
  int path(
    const char*     path);
  // Add data to list
  int data(
    time_t          timestamp,
    const Node*     node = NULL);
  // Get a list of active records for given prefix and paths
  void getList(
    const char*     prefix,
    const char*     base_path,
    const char*     rel_path,
    list<Node*>&    list);
  // Search data in list copying contents on the fly if required, and
  // also expiring data and putting checksums in lists !
  // Searches:                        Prefix      Path        Copy
  //    end of file                   ""          ""          yes
  //    prefix                        prefix      ""          yes
  //    prefix and path               prefix      path        yes
  //    any prefix                    NULL                    no
  //    prefix and any path           prefix      NULL        no
  // Return code:
  //    -1: error, 0: end of file, 1: exceeded, 2: found
  // Caution:
  //    when merging, search for prefix on its own before searching for path
  int search(
    const char*     prefix  = NULL,   // Prefix to search
    const char*     path    = NULL,   // Path to search
    List*           list    = NULL,   // List in which to copy, if any
    time_t          expire  = 0,      // Expiration delay in seconds
    list<string>*   active  = NULL,   // List of active checksums
    list<string>*   expired = NULL);  // List of expired checksums
  // Merge list and backup into this list
  int  merge(
    List&           list,
    List&           journal);
};

}

#endif
