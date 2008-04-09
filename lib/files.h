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

using namespace std;

namespace hbackup {

class Path {
  char*       _path;
  const char* _const_path;
  int         _length;
public:
  Path() :
      _path(NULL),
      _const_path(_path),
      _length(0) {}
  Path(const Path& p) :
      _path(strdup(p._path)),
      _const_path(_path),
      _length(p._length) {}
  Path(const char* path) :
      _path(strdup(path)),
      _const_path(_path),
      _length(strlen(_path)) {}
  Path(const char* path, int length) :
      _path(NULL),
      _const_path(path) {
    if (length < 0) {
      _length = strlen(_const_path);
    } else {
      _length = length;
    }
  }
  Path(const char *dir, const char* name);
  Path(const Path& path, const char* name);
  ~Path();
  const char* operator=(const char* path);
  const Path& operator=(const Path& path);
  operator const char*() const;
  int length() const           { return _length; }
  const char* basename() const { return basename(_const_path); }
  const char* noTrailingSlashes() {
    noTrailingSlashes(_path);
    _length = strlen(_path);
    return _path;
  }
  int compare(const char* s, size_t length = -1) const {
    return compare(_const_path, s, length);
  }
  int compare(const Path& p, size_t length = -1) const {
    return compare(_const_path, p, length);
  }
  int countBlocks(char c) const;
  // Some generic methods
  static const char* basename(const char* path);
  static char* dirname(const char* path);
  static char* fromDos(const char* dos_path);
  static char* noTrailingSlashes(char* dir_path);
  static int compare(const char* s1, const char* s2, size_t length = -1);
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
  // Constructor not copying the path
  Node(const char* path, int length) :
      _path(path, length),
      _type('?'),
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
  void resetMtime() { _mtime = 0; }
  void resetSize()  { _size  = 0; }
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
  int           pathLength()  const { return _path.length();   }
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
  // Constructor not copying the path
  File(const char* path, int length) :
      Node(path, length),
      _checksum(strdup("")) {
    stat();
    _parsed = true;
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
  list<Node*> _nodes;
public:
  // Constructor for existing Node
  Directory(const Node& g) :
      Node(g) {
    _parsed = true;
    _nodes.clear();
  }
  // Constructor not copying the path
  Directory(const char* path, int length) :
      Node(path, length) {
    stat();
    _parsed = true;
    _nodes.clear();
  }
  // Constructor for path in the VFS
  Directory(Path path) :
      Node(path) {
    stat();
    _parsed = true;
    _nodes.clear();
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
  list<Node*>& nodesList()                  { return _nodes; }
  const list<Node*>& nodesListConst() const { return _nodes; }
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
    _link = (char*) malloc(_size + 1);
    readlink(_path, _link, _size);
    _link[_size] = '\0';
  }
  // Constructor for path in the VFS
  Link(Path path) :
      Node(path),
      _link(NULL) {
    stat();
    _parsed = true;
    _link = (char*) malloc(_size + 1);
    readlink(_path, _link, _size);
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
  struct          Private;
  Private*        _d;
  // Convert MD5 to readable string
  static void md5sum(char* out, const unsigned char* in, int bytes);
  ssize_t write_all(
    const void*     buffer,
    size_t          count) const;
  int digest_update_all(
    const void*     buffer,
    size_t          count);
public:
  // Max buffer size for read/write (here for testing purposes)
  static const size_t chunk = 409600;
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
  // A negative value for compression will disbable the cache for write operations
  // Checksum determines whether to compute the checksum as we read
  int open(
    const char*     req_mode,
    int             compression = 0,
    bool            checksum    = true);
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
    int*            buffer_capacity,
    bool*           end_of_line_found = NULL);
#ifdef HAVE_LINE_H
  virtual ssize_t getLine(
    Line&           line,
    bool*           end_of_line_found = NULL) {
      return getLine(line.bufferPtr(), line.capacityPtr(), end_of_line_found);
    }
#endif
  // Write line of characters to file and add end of line character
  ssize_t putLine(
    const char*     buffer);
  // Cancel function for the three methods below
  void setCancelCallback(cancel_f cancel);
  // Compute file checksum
  int computeChecksum();
  // Copy file into another
  int copy(Stream& source);
  // Compare two files
  int compare(Stream& source, long long length = -1);
  // Line feed MUST be present at EOL
  static const int flags_need_lf      = 0x1;
  // Silently accept DOS format for end of line
  static const int flags_accept_cr_lf = 0x2;
  // Treat escape characters ('\') as normal characters
  static const int flags_no_escape    = 0x4;
  // Do not escape last char (and fail to find ending quote) in an end-of-line
  // parameter such as "c:\foo\" (-> c:\foo\ and not c:\foo")
  static const int flags_dos_catch    = 0x8;
  // Treat spaces as field separators
  static const int flags_empty_params = 0x10;
  // Extract parameters from line read from file
  int getParams(                          // -1: error, 0: eof, 1: success
    vector<string>& params,
    char            flags    = 0,
    const char*     delims   = "\t ",   // Default: tabulation and space
    const char*     quotes   = "'\"",   // Default: single and double quotes
    const char*     comments = "#");    // Default: hash
  // Extract parameters from given line
  // Returns 1 if missing ending quote, 0 otherwise
  static int extractParams(
    const char*     line,
    vector<string>& params,
    char            flags      = 0,
    unsigned int    max_params = 0,     // Number of parameters to decode
    const char*     delims     = "\t ", // Default: tabulation and space
    const char*     quotes     = "'\"", // Default: single and double quotes
    const char*     comments   = "#");  // Default: hash
};

}

#endif
