/*
     Copyright (C) 2006-2007  Herve Fache

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

using namespace std;

namespace hbackup {

class Path {
  char* _path;
  int   _length;
public:
  Path() : _path(NULL), _length(0) {}
  Path(const char* path) : _path(strdup(path)), _length(strlen(_path)) {}
  ~Path() { free(_path); }
  const char* operator=(const char* path);
  const char* c_str() const { return _path; }
  int length() const        { return _length; }
  const char* noTrailingSlashes() {
    noTrailingSlashes(_path);
    _length = strlen(_path);
    return _path;
  }
  int compare(const char* s, size_t length = -1) {
    return compare(_path, s, length);
  }
  // Some generic methods
  static char* path(const char* dir_path, const char* name);
  static const char* basename(const char* path);
  static char* dirname(const char* path);
  static char* fromDos(const char* dos_path);
  static char* noTrailingSlashes(char* dir_path);
  static int compare(const char* s1, const char* s2, size_t length = -1);
};

class Node {
protected:
  char*     _path;      // file path
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
        _path(strdup(g._path)),
        _type(g._type),
        _mtime(g._mtime),
        _size(g._size),
        _uid(g._uid),
        _gid(g._gid),
        _mode(g._mode),
        _parsed(false) {}
  // Constructor for path in the VFS
  Node(const char *dir, const char* name = "") :
      _path(Path::path(dir, name)),
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
        _path(strdup(path)),
        _type(type),
        _mtime(mtime),
        _size(size),
        _uid(uid),
        _gid(gid),
        _mode(mode),
        _parsed(false) {}
  virtual ~Node() {
    free(_path);
  }
  // Stat file metadata
  int stat();
  // Reset some metadata
  void resetMtime() { _mtime = 0; }
  void resetSize()  { _size  = 0; }
  // Operators
  bool operator<(const Node& right) const {
    // Only compare names
    return Path::compare(_path, right._path) < 0;
  }
  // Compares names and metadata, not paths
  virtual bool operator!=(const Node&) const;
  // Data read access
  virtual bool  isValid() const { return _type != '?'; }
  const char*   path()    const { return _path;   }
  const char*   name()    const { return basename(_path);   }
  char          type()    const { return _type;   }
  time_t        mtime()   const { return _mtime;  }
  long long     size()    const { return _size;   }
  uid_t         uid()     const { return _uid;    }
  gid_t         gid()     const { return _gid;    }
  mode_t        mode()    const { return _mode;   }
  bool          parsed()  const { return _parsed; }
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
  File(const char *path, const char* name = "") :
      Node(path, name),
      _checksum(NULL) {
    stat();
    _parsed = true;
    _checksum = strdup("");
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
  // Constructor for path in the VFS
  Directory(const char *path, const char* name = "") :
      Node(path, name) {
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
  Link(const char *dir_path, const char* name = "") :
      Node(dir_path, name),
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
public:
  // Max buffer size for read/write
  static const size_t chunk = 409600;
  // Prototype for cancellation function
  typedef bool (*cancel_f)();
  // Constructor for path in the VFS
  Stream(const char *dir_path, const char* name = "");
  virtual ~Stream();
  bool isOpen() const;
  bool isWriteable() const;
  // Open file, for read or write (no append), with or without compression
  // A negative value for compression disables the read/write cache
  int open(
    const char*     req_mode,
    int             compression = 0);
  // Close file, for read or write (no append), with or without compression
  int close();
  // Read file
  ssize_t read(
    void*           buffer,
    size_t          count);
  // Write to file
  ssize_t write(
    const void*     buffer,
    size_t          count);
  // Read a line from file
  ssize_t getLine(
    string&         buffer);
  // Compute file checksum
  int computeChecksum();
  // Copy file into another
  int copy(Stream& source, cancel_f cancel = NULL);
  // Compare two files
  int compare(Stream& source, long long length = -1);
  // Read parameters from line
  static int readConfigLine(const string& line, list<string>& params);
};

}

#endif
