/*
    Copyright (C) 2006 - 2011  Hervé Fache

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _FILES_H
#define _FILES_H

#include <list>
#include <vector>

using namespace std;

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

namespace htoolbox {

class Path {
  struct Buffer {
    char*  path;
    size_t size;
    size_t ref_cnt;
    Buffer() : path(NULL), size(0), ref_cnt(1) {}
    ~Buffer() { free(path); }
  };
  Buffer* _buffer;
  Path() {}
  const Path& operator=(const Path&);
public:
  enum {
    NO_TRAILING_SLASHES = 1 << 0,
    NO_REPEATED_SLASHES = 1 << 1,
    CONVERT_FROM_DOS    = 1 << 2
  };
  Path(const Path& path);
  Path(Path& path);
  Path(const char* path, int flags = 0);
  Path(const char* dir, const char* name);
  ~Path();
  operator const char* () const { return _buffer->path; }
  const char* c_str() const { return _buffer->path; }
  size_t size() const { return _buffer->size; }
  size_t length() const { return _buffer->size; }
  const char* basename() const { return basename(_buffer->path); }
  Path dirname() const;
  // Some generic methods
  static const char* basename(const char* path);
  static int pathcmp(const char* s1, const char* s2, ssize_t length = -1);
  static const char* fromDos(char* path);
  static const char* noTrailingSlashes(char* path, size_t* size_p = NULL);
  static const char* noRepeatedSlashes(char* path, size_t* size_p = NULL);
};

class Node;

class Node {
protected:
  Path          _path;      // file path
  const char*   _basename;  // file cached base name
  char          _type;      // file type ('?' if metadata not available)
  time_t        _mtime;     // time of last modification
  int64_t       _size;      // file size, in bytes
  uid_t         _uid;       // user ID of owner
  gid_t         _gid;       // group ID of owner
  mode_t        _mode;      // permissions
  dev_t         _device;    // ID of device containing file
  char          _hash[129]; // regular file hash
  char*         _link;      // symbolic link value
  list<Node*>*  _nodes;     // directory entries
  // Stat file metadata
  int stat();
  Node();
  const Node& operator=(const Node&);
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
        _device(g._device),
        _link(NULL),
        _nodes(NULL) {
      _basename = basename(_path);
      if (g._link != NULL) {
        _link = strdup(g._link);
      }
      strcpy(_hash, g._hash);
    }
  // Constructor for path in the VFS
  Node(const Path& path) : _path(path), _type('?'), _link(NULL), _nodes(NULL) {
    _basename = basename(_path);
    if (_path[0] != '\0') {
      stat();
    }
    strcpy(_hash, "");
  }
  // Constructor for given file metadata
  Node(
      const char* path,
      char        type,
      time_t      mtime,
      int64_t     size,
      uid_t       uid,
      gid_t       gid,
      mode_t      mode,
      dev_t       device,
      const char* link_or_hash = "") :
        _path(path),
        _type(type),
        _mtime(mtime),
        _size(size),
        _uid(uid),
        _gid(gid),
        _mode(mode),
        _device(device),
        _link(NULL),
        _nodes(NULL) {
      _basename = basename(_path);
      if (_type == 'l') {
        _link = strdup(link_or_hash);
      } else
      if (_type == 'f') {
        strcpy(_hash, link_or_hash);
      }
    }
  ~Node() {
    free(_link);
    deleteList();
  }
  // Reset some metadata
  void setMtime(time_t mtime)    { _mtime = mtime; }
  void setSize(int64_t size)   { _size  = size;  }
  // Only used in list test
  void setHash(const char* hash) { strcpy(_hash, hash); }
  void setLink(const char* link) {
    free(_link);
    _link = strdup(link);
  }
  // Operators
  bool operator<(const Node& right) const {
    // Only compare paths
    return Path::pathcmp(_path, right._path) < 0;
  }
  // Data read access
  bool          exists()  const { return _type != '?';  }
  bool          isDir()   const { return _type == 'd';  }
  bool          isReg()   const { return _type == 'f';  }
  bool          isLink()  const { return _type == 'l';  }
  const char*   path()    const { return _path;         }
  const char*   name()    const { return _basename;     }
  char          type()    const { return _type;         }
  time_t        mtime()   const { return _mtime;        }
  int64_t       size()    const { return _size;         }
  uid_t         uid()     const { return _uid;          }
  gid_t         gid()     const { return _gid;          }
  mode_t        mode()    const { return _mode;         }
  dev_t         device()  const { return _device;       }
  const char*   hash()    const { return _hash;         }
  const char*   link()    const {
    if (_link != NULL) {
      return _link;
    } else {
      return "";
    }
  }
  // Create empty/truncate file
  static int touch(const char* path);
  // Create directory
  int mkdir();
  static int mkdir_p(const char* path, mode_t mode);
  // Directory entries management
  int   createList();
  void  deleteList();
  bool  hasList() const                     { return _nodes != NULL; }
  list<Node*>& nodesList()                  { return *_nodes; }
  const list<Node*>& nodesListConst() const { return *_nodes; }
  // Find out whether path can be opened
  static bool isReadable(
    const char*     path);
  // Add extensions from array to path (which must be large enough) and find
  // first readable file.  Last element of array must be NULL.  Returns
  // extension index in array, or -1 if none matches.
  // Example:
  // const char* extensions[] = { "", ".gz", ".bz2", NULL };
  // int no = Node::findExtension(path, extensions);
  static int findExtension(
    char*           path,
    const char*     extensions[],
    ssize_t         original_length = -1);
};

}

#endif // _FILES_H
