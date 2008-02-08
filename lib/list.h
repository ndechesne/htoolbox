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
  struct          Private;
  Private*        _d;
  // Buffer relevant line
  ssize_t fetchLine(bool use_found = false);
  // Decode metadata from current line
  int decodeDataLine(
    const string&   line,
    const char*     path,
    Node**          node,
    time_t*         timestamp);
public:
  List(
    const char*     dir_path,
    const char*     name = "");
  ~List();
  // Open file, for read or write (no append), with compression (cf. Stream)
  int open(
    const char*     req_mode,
    int             compression = 0);
  // Close file
  int close();
  // Version
  bool isOldVersion() const;
  // Empty file (check right after opening for read)
  bool isEmpty() const;
  // Put line into list buffer (will fail and return -1 if buffer in use)
  ssize_t putLine(const char* line);
  // Read a line from file/cache
  ssize_t getLine(
    string&         buffer);
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
    char**          client,
    char**          path,
    Node**          node,
    time_t          date = -1);
  // Add client to list
  int addClient(
    const char*     client);
  // Add path to list
  int addPath(
    const char*     path);
  // Add data to list
  int addData(
    time_t          timestamp,
    const Node*     node = NULL,
    bool            bufferize = false);
  // Search data in list copying contents on the fly if required, and
  // also expiring data and putting checksums in lists!!!
  // Searches:                        Client      Path        Copy
  //    end of file                   ""          ""          all
  //    client                        client      ""          all up to client
  //    client and path               client      path        all up to path
  //    any client                    NULL                    all before client
  //    client and any path           client      NULL        all before path
  // Expiration:
  //    -1: no expiration, 0: only keep last, otherwise use given date
  // Return code:
  //    -1: error, 0: end of file, 1: exceeded, 2: found
  // Caution:
  //    when merging, search for client on its own before searching for path
  int search(
    const char*     client  = NULL,   // Client to search
    const char*     path    = NULL,   // Path to search
    List*           list    = NULL,   // List in which to copy, if any
    time_t          expire  = -1,     // Expiration date
    list<string>*   active  = NULL,   // List of active checksums
    list<string>*   expired = NULL);  // List of expired checksums
  // Merge list and backup into this list
  int  merge(
    List&           list,
    List&           journal);
};

}

#endif
