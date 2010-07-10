/*
     Copyright (C) 2006-2008  Herve Fache

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

#ifndef FILES_H
#define FILES_H

#include <list>
#include <vector>
#include <string>

using namespace std;

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

#include <line.h>

namespace hbackup {

class Path : public Line {
public:
  Path() : Line() {}
  Path(const char* path) : Line(path) {}
  Path(const char* dir, const char* name);
  Path(const Path& path, const char* name);
  const Path& operator=(const char* line) {
    Line::operator=(line);
    return *this;
  }
  const Path& operator=(const Line& line) {
    Line::operator=(line);
    return *this;
  }
  const Path& fromDos();
  const Path& noTrailingSlashes();
  size_t length() const  { return size(); }
  Path dirname() const;
  const char* basename() const { return basename(*this); }
  int compare(const char* s, ssize_t length = -1) const {
    return compare(*this, s, length);
  }
  int compare(const Path& p, ssize_t length = -1) const {
    return compare(*this, p, length);
  }
  // Some generic methods
  static const char* basename(const char* path);
  static int compare(const char* s1, const char* s2, ssize_t length = -1);
};

class Node {
protected:
  Path      _path;      // file path
  char      _type;      // file type ('?' if metadata not available)
  time_t    _mtime;     // time of last modification
  long long _size;      // file size, in bytes
  uid_t     _uid;       // user ID of owner
  gid_t     _gid;       // group ID of owner
  mode_t    _mode;      // permissions
  bool      _parsed;    // more info available using proper type
public:
  // Default constructor
  Node(const Node& g) :
        _path(g._path),
        _type(g._type),
        _mtime(g._mtime),
        _size(g._size),
        _uid(g._uid),
        _gid(g._gid),
        _mode(g._mode),
        _parsed(false) {}
  // Constructor for path in the VFS
  Node(Path path) :
      _path(path),
      _type('?'),
      _parsed(false) {}
  // Constructor for given file metadata
  Node(
      const char* path,
      char        type,
      time_t      mtime,
      long long   size,
      uid_t       uid,
      gid_t       gid,
      mode_t      mode) :
        _path(path),
        _type(type),
        _mtime(mtime),
        _size(size),
        _uid(uid),
        _gid(gid),
        _mode(mode),
        _parsed(false) {}
  virtual ~Node() {}
  // Stat file metadata
  int stat();
  // Reset some metadata
  void setMtime(time_t mtime)       { _mtime = mtime; }
  void setSize(long long size)      { _size  = size; }
  // Operators
  bool operator<(const Node& right) const {
    // Only compare paths
    return _path.compare(right._path) < 0;
  }
  // Compares names and metadata, not paths
  virtual bool operator!=(const Node&) const;
  // Data read access
  virtual bool  isValid()     const { return _type != '?';     }
  const char*   path()        const { return _path;            }
  size_t        pathLength()  const { return _path.length();   }
  const char*   name()        const { return _path.basename(); }
  char          type()        const { return _type;            }
  time_t        mtime()       const { return _mtime;           }
  long long     size()        const { return _size;            }
  uid_t         uid()         const { return _uid;             }
  gid_t         gid()         const { return _gid;             }
  mode_t        mode()        const { return _mode;            }
  bool          parsed()      const { return _parsed;          }
  // Remove node
  int   remove();
};

class File : public Node {
protected:
  char*     _checksum;
public:
  // Constructor for existing Node
  File(const File& g) :
      Node(g),
      _checksum(NULL) {
    _parsed = true;
    _checksum = strdup(g._checksum);
  }
  File(const Node& g) :
      Node(g),
      _checksum(NULL) {
    _parsed = true;
    _checksum = strdup("");
  }
  // Constructor for path in the VFS
  File(Path path) :
      Node(path),
      _checksum(strdup("")) {
    stat();
    _parsed = true;
  }
  // Constructor for given file metadata
  File(
    const char* name,
    char        type,
    time_t      mtime,
    long long   size,
    uid_t       uid,
    gid_t       gid,
    mode_t      mode,
    const char* checksum) :
      Node(name, type, mtime, size, uid, gid, mode),
      _checksum(NULL) {
    _parsed = true;
    setChecksum(checksum);
  }
  virtual ~File() {
    free(_checksum);
  }
  bool isValid() const { return _type == 'f'; }
  // Create empty file
  int create();
  // Data read access
  const char* checksum() const { return _checksum;  }
  void setChecksum(const char* checksum) {
    free(_checksum);
    _checksum = NULL;
    _checksum = strdup(checksum);
  }
};

class Directory : public Node {
  list<Node*>*  _nodes;
public:
  // Constructor for existing Node
  Directory(const Node& g) :
      Node(g) {
    _parsed = true;
    _nodes  = NULL;
  }
  // Constructor for path in the VFS
  Directory(Path path) :
      Node(path) {
    stat();
    _parsed = true;
    _nodes  = NULL;
  }
  ~Directory() {
    deleteList();
  }
  bool  isValid() const { return _type == 'd'; }
  // Create directory
  int   create();
  // Create list of Nodes contained in directory
  int   createList();
  void  deleteList();
  bool  hasList() const                     { return _nodes != NULL; }
  list<Node*>& nodesList()                  { return *_nodes; }
  const list<Node*>& nodesListConst() const { return *_nodes; }
};

class Link : public Node {
  char*     _link;
public:
  // Constructor for existing Node
  Link(const Link& g) :
      Node(g),
      _link(NULL) {
    _parsed = true;
    _link = strdup(g._link);
  }
  Link(const Node& g) :
      Node(g),
      _link(NULL) {
    _parsed = true;
    _link = static_cast<char*>(malloc(static_cast<int>(_size) + 1));
    ssize_t count = readlink(_path, _link, static_cast<int>(_size));
    if (count >= 0) {
      _size = count;
    } else {
      _size = 0;
    }
    _link[_size] = '\0';
  }
  // Constructor for path in the VFS
  Link(Path path) :
      Node(path),
      _link(NULL) {
    stat();
    _parsed = true;
    _link = static_cast<char*>(malloc(static_cast<int>(_size) + 1));
    ssize_t count = readlink(_path, _link, static_cast<int>(_size));
    if (count >= 0) {
      _size = count;
    } else {
      _size = 0;
    }
    _link[_size] = '\0';
  }
  // Constructor for given file metadata
  Link(
    const char* name,
    char        type,
    time_t      mtime,
    long long   size,
    uid_t       uid,
    gid_t       gid,
    mode_t      mode,
    const char* link) :
        Node(name, type, mtime, size, uid, gid, mode),
        _link(NULL) {
    _parsed = true;
    _link = strdup(link);
  }
  ~Link() {
    free(_link);
  }
  // Operators
  bool operator!=(const Link&) const;
  bool isValid() const { return _type == 'l'; }
  // Data read access
  const char* link()    const { return _link;  }
};

class Stream : public File {
  struct            Private;
  Private* const    _d;
  // Convert MD5 to readable string
  static void md5sum(char* out, const unsigned char* in, int bytes);
  int digest_update(
    const void*     buffer,
    size_t          size);
  ssize_t read_decompress(
    void*           buffer,
    size_t          asked,
    size_t*         given);
  ssize_t write_all(
    const void*     buffer,
    size_t          size);
  ssize_t write_compress(
    const void*     buffer,
    size_t          size,
    bool            finish = false);
public:
  // Magic constructor that auto-selects the extension
  static Stream* select(
    Path            path,
    vector<string>& extensions,
    unsigned int*   no = NULL);
  // Prototype for cancellation function (true cancels)
  typedef bool (*cancel_f) (
    unsigned short  method);
  // Prototype for progress report function (receives size done and total)
  typedef void (*progress_f) (
    long long       previous,
    long long       current,
    long long       total);
  // Constructor for path in the VFS
  Stream(Path path);
  virtual ~Stream();
  bool isOpen() const;
  bool isWriteable() const;
  // Open file, for read or write (no append), with or without compression
  // Flags are either O_RDONLY or O_WRONLY, the latter implies O_CREAT
  // checksum determines whether to compute the checksum as we read
  // header when compressing
  int open(
    int             flags,
    unsigned int    compression   = 0,
    bool            checksum      = true);
  // Close file, for read or write (no append), with or without compression
  int close();
  // Get progress indicator (size read/written)
  long long progress() const;
  // Progress report function for read and write
  void setProgressCallback(progress_f progress);
  // Read file
  ssize_t read(
    void*           buffer,
    size_t          count);
  // Write to file
  ssize_t write(
    const void*     buffer,
    size_t          count);
  // Read a line of characters from file until end of line or file
  virtual ssize_t getLine(
    char**          buffer,
    size_t*         buffer_capacity,
    bool*           end_of_line_found = NULL);
  virtual ssize_t getLine(
    LineBuffer&     line,
    bool*           end_of_line_found = NULL) {
      *line.sizePtr() = getLine(line.bufferPtr(), line.capacityPtr(),
        end_of_line_found);
      return *line.sizePtr();
    }
  // Write line of characters to file and add end of line character
  ssize_t putLine(
    const char*     buffer);
  // Cancel function for the three methods below
  void setCancelCallback(cancel_f cancel);
  // Compute file checksum
  int computeChecksum();
  // Copy file into another, or two others if dest2 is not NULL
  int copy(Stream* dest, Stream* dest2 = NULL);
  // Compare two files
  int compare(Stream& other, long long length = -1);
  // Get real file size
  long long dataSize() const;
  // Line feed MUST be present at EOL
  static const unsigned char flags_need_lf      = 0x1;
  // Silently accept DOS format for end of line
  static const unsigned char flags_accept_cr_lf = 0x2;
  // Treat escape characters ('\') as normal characters
  static const unsigned char flags_no_escape    = 0x4;
  // Do not escape last char (and fail to find ending quote) in an end-of-line
  // parameter such as "c:\foo\" (-> c:\foo\ and not c:\foo")
  static const unsigned char flags_dos_catch    = 0x8;
  // Treat spaces as field separators
  static const unsigned char flags_empty_params = 0x10;
  // Extract parameters from given line
  // Returns 1 if missing ending quote, 0 otherwise
  static int extractParams(
    const char*     line,
    vector<string>& params,
    unsigned char   flags      = 0,
    unsigned int    max_params = 0,     // Number of parameters to decode
    const char*     delims     = "\t ", // Default: tabulation and space
    const char*     quotes     = "'\"", // Default: single and double quotes
    const char*     comments   = "#");  // Default: hash
};

}

#endif
