/*
     Copyright (C) 2006-2010  Herve Fache

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

namespace hbackup {

class Path {
  char*  _path;
  size_t _capacity;
  Path();
  const Path& operator=(const Path&);
public:
  Path(const Path& path) : _path(strdup(path._path)), _capacity(strlen(_path)) {}
  Path(const char* path) : _path(strdup(path)), _capacity(strlen(_path)) {}
  Path(const char* dir, const char* name);
  Path(const Path& dir, const char* name);
  ~Path() { free(_path); }
  operator const char* () const { return _path; }
  const char* c_str() const { return _path; }
  Path dirname() const;
  // Some generic methods
  static const char* basename(const char* path);
  static int compare(const char* s1, const char* s2, ssize_t length = -1);
  static const char* fromDos(char* path);
  static const char* noTrailingSlashes(char* path);
};

class Node {
protected:
  Path        _path;      // file path
  const char* _basename;  // file cached base name
  char        _type;      // file type ('?' if metadata not available)
  time_t      _mtime;     // time of last modification
  long long   _size;      // file size, in bytes
  uid_t       _uid;       // user ID of owner
  gid_t       _gid;       // group ID of owner
  mode_t      _mode;      // permissions
  bool        _parsed;    // more info available using proper type
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
        _parsed(false) {
      _basename = basename(_path);
    }
  // Constructor for path in the VFS
  Node(Path path) :
        _path(path),
        _type('?'),
        _parsed(false) {
      _basename = basename(_path);
    }
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
        _parsed(false) {
      _basename = basename(_path);
    }
  virtual ~Node() {}
  // Stat file metadata
  int stat();
  // Reset some metadata
  void setMtime(time_t mtime)       { _mtime = mtime; }
  void setSize(long long size)      { _size  = size; }
  // Operators
  bool operator<(const Node& right) const {
    // Only compare paths
    return Path::compare(_path, right._path) < 0;
  }
  // Compares names and metadata, not paths
  virtual bool operator!=(const Node&) const;
  // Data read access
  virtual bool  isValid()     const { return _type != '?';  }
  const char*   path()        const { return _path;         }
  const char*   name()        const { return _basename;     }
  char          type()        const { return _type;         }
  time_t        mtime()       const { return _mtime;        }
  long long     size()        const { return _size;         }
  uid_t         uid()         const { return _uid;          }
  gid_t         gid()         const { return _gid;          }
  mode_t        mode()        const { return _mode;         }
  bool          parsed()      const { return _parsed;       }
  // Remove node
  int   remove();
};

class File : public Node {
protected:
  char _hash[64];
public:
  // Constructor for existing Node
  File(const File& g) : Node(g) {
    _parsed = true;
    strcpy(_hash, g._hash);
  }
  File(const Node& g) : Node(g) {
    _parsed = true;
    strcpy(_hash, "");
  }
  // Constructor for path in the VFS
  File(Path path) : Node(path) {
    stat();
    _parsed = true;
    strcpy(_hash, "");
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
    const char* hash) : Node(name, type, mtime, size, uid, gid, mode) {
    _parsed = true;
    setChecksum(hash);
  }
  bool isValid() const { return _type == 'f'; }
  // Create empty file
  int create();
  // Data read access
  const char* checksum() const { return _hash;  }
  void setChecksum(const char* hash) { strcpy(_hash, hash); }
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
    size_t size = static_cast<size_t>(_size);
    _link = static_cast<char*>(malloc(size + 1));
    ssize_t count = readlink(_path, _link, size);
    if (count >= 0) {
      _size = count;
    } else {
      _size = 0;
    }
    _link[_size] = '\0';
  }
  // Constructor for path in the VFS
  Link(Path path) : Node(path), _link(NULL) {
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
  const char* link() const { return _link; }
  void setLink(const char* link) {
    free(_link);
    _link = strdup(link);
  }
};

class Stream : public File {
  struct            Private;
  Private* const    _d;
public:
  // Prototype for cancellation function (true cancels)
  typedef bool (*cancel_f) (
    unsigned short  method);
  // Prototype for progress report function (receives size done and total)
  typedef void (*progress_f) (
    long long       previous,
    long long       current,
    long long       total);
  // Constructor for path in the VFS for read or write (no append), with or
  // without hash computation / compression (give level)
  Stream(const char* path, bool writer, bool hash = false, int compression = 0);
  virtual ~Stream();
  bool isOpen() const;
  bool isWriteable() const;
  // Open file
  int open();
  // Close file
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
};

}

#endif
