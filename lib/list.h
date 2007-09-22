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

class DbData {
  char*   _prefix;
  char*   _path;
  Node*   _node;
public:
  DbData(const DbData& data) :
      _prefix(NULL),
      _path(NULL),
      _node(NULL) {
    asprintf(&_prefix, "%s", data._prefix);
    asprintf(&_path, "%s", data._path);
    switch (data._node->type()) {
      case 'l':
        _node = new Link(*((Link*)data._node));
        break;
      case 'f':
        _node = new File(*((File*)data._node));
        break;
      default:
        _node = new Node(*data._node);
    }
  }
  DbData(
      const char* prefix,
      const char* path,
      const Node* node) :
      _prefix(NULL),
      _path(NULL),
      _node(NULL) {
    asprintf(&_prefix, "%s", prefix);
    asprintf(&_path, "%s", path);
    switch (node->type()) {
      case 'l':
        _node = new Link(*((Link*)node));
        break;
      case 'f':
        _node = new File(*((File*)node));
        break;
      default:
        _node = new Node(*node);
    }
  }
  ~DbData() {
    free(_prefix);
    free(_path);
    free(_node);
  }
  const Node* data() const      { return _node; }
  const char* prefix() const    { return _prefix; }
  const char* path() const      { return _path; }
  int pathCompare(const char* path, int length = -1) const {
    char* full_path = NULL;
    asprintf(&full_path, "%s/%s", _prefix, _path);
    int cmp = Node::pathCompare(full_path, path, length);
    free(full_path);
    return cmp;
  }
  void line() const {
    printf("%s\t%s\t%c\t%lld\t%d\t%u\t%u\t%o",
      _prefix, _path, _node->type(), _node->size(), _node->mtime() != 0,
      _node->uid(), _node->gid(), _node->mode());
    if (_node->type() == 'l') {
      printf("\t");
      printf(((Link*)_node)->link());
    }
    if (_node->type() == 'f') {
      printf("\t");
      printf(((File*)_node)->checksum());
    }
    printf("\n");
  }
};

class List : public Stream {
  string          _line;
  // -2: unexpected eof, -1: error, 0: read again, 1: use current, 2: empty
  int             _line_status;
  // Decode metadata from current line
  int decodeLine(
    const char*   path,
    Node**        node,
    time_t*       timestamp);
public:
  List(
    const char*   dir_path,
    const char*   name = "") :
    Stream(dir_path, name) {}
  // Open file, for read or write (no append), with compression (cf. Stream)
  int open(
    const char*   req_mode,
    int           compression = 0);
  // Close file
  int close();
  // Empty file (check right after opening for read)
  bool isEmpty() const { return _line_status == 2; }
  // Get relevant line
  ssize_t getLine();
  // Skip to given prefix or to next if prefix is NULL
  bool findPrefix(const char* prefix);
  // Convert one 'line' of data
  int getEntry(
    time_t*       timestamp,
    char**        prefix,
    char**        path,
    Node**        node,
    time_t        date = -1); // -1: all, 0: latest, otherwise timestamp to get
  // Add a journal record of added file
  int added(
    const char*   prefix,           // Set to NULL not to add prefix
    const char*   path,
    const Node*   node,
    time_t        timestamp = -1);
  // Add a journal record of removed file
  int removed(
    const char*   prefix,           // Set to NULL not to add prefix
    const char*   path,
    time_t        timestamp = -1);
  // Get a list of active records for given prefix and paths
  void getList(
    const char*   prefix,
    const char*   base_path,
    const char*   rel_path,
    list<Node*>&  list);
  // Search data in list copying contents on the fly
  int searchCopy(
    List&         list,
    StrPath&      prefix,
    StrPath&      path,
    time_t        expire = 0,
    list<string>* active = NULL,
    list<string>* expired = NULL);
  // Merge list and backup into this list
  int  merge(
    List&         list,
    List&         journal);
};

}

#endif
