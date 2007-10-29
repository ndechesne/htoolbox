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

#include <fstream>
#include <list>

using namespace std;

#include <fcntl.h>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <zlib.h>

namespace hbackup {

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
  void  metadata();
  const char* basename(const char* path) const {
    const char* name = strrchr(path, '/');
    if (name != NULL) {
      name++;
    } else {
      name = path;
    }
    return name;
  }
public:
  // Default constructor
  Node(const Node& g) :
        _path(NULL),
        _type(g._type),
        _mtime(g._mtime),
        _size(g._size),
        _uid(g._uid),
        _gid(g._gid),
        _mode(g._mode),
        _parsed(false) {
    _path = strdup(g._path);
  }
  // Constructor for path in the VFS
  Node(const char *path, const char* name = "");
  // Constructor for given file metadata
  Node(
      const char* path,
      char        type,
      time_t      mtime,
      long long   size,
      uid_t       uid,
      gid_t       gid,
      mode_t      mode) :
        _path(NULL),
        _type(type),
        _mtime(mtime),
        _size(size),
        _uid(uid),
        _gid(gid),
        _mode(mode),
        _parsed(false) {
    _path = strdup(path);
  }
  virtual ~Node() {
    free(_path);
    _path = NULL;
  }
  // Operators
  bool operator<(const Node& right) const {
    // Only compare names
    return pathCompare(_path, right._path) < 0;
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
  static char* path(const char* dir_path, const char* name) {
    char* full_path = NULL;
    if (dir_path[0] == '\0') {
      full_path = strdup(name);
    } else if (name[0] == '\0') {
      full_path = strdup(dir_path);
    } else {
      asprintf(&full_path, "%s/%s", dir_path, name);
    }
    return full_path;
  }
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
  ~File() {
    free(_checksum);
    _checksum = NULL;
  }
  // Create empty file
  int create();
  bool isValid() const { return _type == 'f'; }
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
    _size  = 0;
    _mtime = 0;
    _parsed = true;
    _nodes.clear();
  }
  // Constructor for path in the VFS
  Directory(const char *path, const char* name = "") :
      Node(path, name) {
    _size  = 0;
    _mtime = 0;
    _parsed = true;
    _nodes.clear();
  }
  ~Directory() {
    deleteList();
  }
  // Create directory
  int   create();
  // Create list of Nodes contained in directory
  int   createList();
  void  deleteList();
  bool  isValid() const                     { return _type == 'd'; }
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
    _link = NULL;
  }
  // Operators
  bool operator!=(const Link&) const;
  bool isValid() const { return _type == 'l'; }
  // Data read access
  const char* link()    const { return _link;  }
};

class Stream : public File {
  char*           _path;      // file path
  int             _fd;        // file descriptor
  mode_t          _fmode;     // file open mode
  long long       _dsize;     // uncompressed file data size, in bytes
  unsigned char*  _fbuffer;   // buffer for read/write operations
  unsigned char*  _freader;   // buffer read pointer
  ssize_t         _flength;   // buffer length
  EVP_MD_CTX*     _ctx;       // openssl resources
  z_stream*       _strm;      // zlib resources
  // Convert MD5 to readable string
  static void md5sum(char* out, const unsigned char* in, int bytes);
public:
  // Max buffer size for read/write
  static const size_t chunk = 409600;
  // Prototype for cancellation function
  typedef bool (*cancel_f)();
//   // Constructor for existing File
//   Stream(const File& g, const char* dir_path) {}
  // Constructor for path in the VFS
  Stream(const char *dir_path, const char* name = "") :
      File(dir_path, name),
      _fd(-1) {
    _path = path(dir_path, name);
  }
  virtual ~Stream();
  bool isOpen() const      { return _fd != -1; }
  bool isWriteable() const { return (_fmode & O_WRONLY) != 0; }
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
  // Data access
  long long dsize() const   { return _dsize; };
  // Read parameters from line
  static int decodeLine(const string& line, list<string>& params);
};

}

#endif
