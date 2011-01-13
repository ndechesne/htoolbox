/*
     Copyright (C) 2006-2011  Herve Fache

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

#include <list>

#include <cctype>
#include <cstdio>

#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

using namespace std;

#include "report.h"
#include "files.h"

using namespace htoolbox;

Path::Path(Path& path) : _buffer(path._buffer) {
  ++_buffer->ref_cnt;
}

Path::Path(const Path& path) : _buffer(path._buffer) {
  ++_buffer->ref_cnt;
}

Path::Path(const char* path, int flags) {
  _buffer = new Buffer;
  _buffer->path = strdup(path);
  _buffer->size = strlen(path);
  if (flags & CONVERT_FROM_DOS) {
    fromDos(_buffer->path);
  }
  if (flags & NO_TRAILING_SLASHES) {
    noTrailingSlashes(_buffer->path, &_buffer->size);
  }
  if (flags & NO_REPEATED_SLASHES) {
    noRepeatedSlashes(_buffer->path, &_buffer->size);
  }
}

Path::Path(const char* dir, const char* name) {
  _buffer = new Buffer;
  if ((name != NULL) && (name[0] != '\0')) {
    _buffer->size = asprintf(&_buffer->path, "%s/%s", dir, name);
  } else {
    _buffer->path = strdup(dir);
    _buffer->size = strlen(dir);
  }
}

Path::~Path() {
  if (--_buffer->ref_cnt == 0) {
    delete _buffer;
  }
}

const char* Path::basename(const char* path) {
  const char* name = strrchr(path, '/');
  if (name != NULL) {
    name++;
  } else {
    name = path;
  }
  return name;
}

Path Path::dirname() const {
  char* pos = strrchr(_buffer->path, '/');
  if (pos == NULL) {
    return ".";
  } else {
    Path rc;
    rc._buffer = new Buffer;
    rc._buffer->path = strdup(_buffer->path);
    rc._buffer->size = pos - _buffer->path;
    pos = rc._buffer->path + rc._buffer->size;
    *pos = '\0';
    return rc;
  }
}

const char* Path::fromDos(char* path) {
  bool proven = false;
  size_t size = 0;
  for (char* reader = path; *reader != '\0'; ++reader) {
    if (*reader == '\\') {
      *reader = '/';
      proven = true;
    }
    ++size;
  }
  // Upper case drive letter
  if (proven && (size >= 3)) {
    if ((path[1] == ':') && (path[2] == '/') && islower(path[0])) {
      path[0] = static_cast<char>(toupper(path[0]));
    }
  }
  return path;
}

const char* Path::noTrailingSlashes(char* path, size_t* size_p) {
  char* end = &path[strlen(path)];
  while ((--end >= path) && (*end == '/')) {}
  end[1] = '\0';
  if (size_p != NULL) {
    *size_p = end - path + 1;
  }
  return path;
}

const char* Path::noRepeatedSlashes(char* path, size_t* size_p) {
  const char* reader = path;
  char*       writer = path;
  bool        slash  = false;
  while (*reader != '\0') {
    if (*reader == '/') {
      if (slash) {
        ++reader;
        continue;
      }
      slash = true;
    } else {
      slash = false;
    }
    if (writer != reader) {
      *writer = *reader;
    }
    ++reader;
    ++writer;
  }
  *writer = '\0';
  if (size_p != NULL) {
    *size_p = writer - path;
  }
  return path;
}

int Path::compare(const char* s1, const char* s2, ssize_t length) {
  while (true) {
    if (length-- == 0) {
      return 0;
    }
    if (*s1 == *s2) {
      if (*s1 != '\0') {
        s1++;
        s2++;
        continue;
      } else {
        return 0;
      }
    }
    if (*s1 == '\0') {
      return -1;
    } else
    if (*s2 == '\0') {
      return 1;
    } else
    if (*s1 == '/') {
      // For TAB and LF
      if ((*s2 == '\t') || (*s2 == '\n')) {
        return 1;
      } else {
        return -1;
      }
    } else
    if (*s2 == '/') {
      if ((*s1 == '\t') || (*s1 == '\n')) {
        return -1;
      } else {
        return 1;
      }
    } else {
      return *s1 < *s2 ? -1 : 1;
    }
  }
}

int Node::stat() {
  struct stat64 metadata;
  int rc = lstat64(_path, &metadata);
  if (rc) {
    // errno set by lstat
    _type = '?';
  } else {
    if (S_ISREG(metadata.st_mode))       _type = 'f';
    else if (S_ISDIR(metadata.st_mode))  _type = 'd';
    else if (S_ISCHR(metadata.st_mode))  _type = 'c';
    else if (S_ISBLK(metadata.st_mode))  _type = 'b';
    else if (S_ISFIFO(metadata.st_mode)) _type = 'p';
    else if (S_ISLNK(metadata.st_mode))  _type = 'l';
    else if (S_ISSOCK(metadata.st_mode)) _type = 's';
    else                                 _type = '?';
    // Fill in file information
    _size  = metadata.st_size;
    _mtime = metadata.st_mtime;
    _uid   = metadata.st_uid;
    _gid   = metadata.st_gid;
    _mode  = metadata.st_mode & ~S_IFMT;
    // Special case for symbolic links
    if (isLink()) {
      _link = static_cast<char*>(malloc(static_cast<int>(_size) + 1));
      ssize_t count = readlink(_path, _link, static_cast<int>(_size));
      if (count >= 0) {
        _size = count;
      } else {
        _size = 0;
      }
      _link[_size] = '\0';
    }
  }
  return rc;
}

int Node::touch(
    const char*     path) {
  FILE* fd = fopen(path, "w");
  if (fd != NULL) {
    fclose(fd);
    return 0;
  }
  return -1;
}

static int direntFilter(const struct dirent* a) {
  if (a->d_name[0] != '.') return 1;
  if (a->d_name[1] == '\0') return 0;
  if (a->d_name[1] != '.') return 1;
  if (a->d_name[2] == '\0') return 0;
  return 1;
}

static int direntCompare(const dirent** a, const dirent** b) {
  return Path::compare((*a)->d_name, (*b)->d_name);
}

int Node::createList() {
  if (_nodes != NULL) {
    errno = EBUSY;
    return -1;
  }
  // Create list
  struct dirent** direntList;
  int size = scandir(_path, &direntList, direntFilter, direntCompare);
  if (size < 0) {
    return -1;
  }
  // Cleanup list
  int size2 = size;
  while (size2--) {
    free(direntList[size2]);
  }
  free(direntList);
  // Bug in CIFS client, usually gets detected by two subsequent dir reads
  size2 = scandir(_path, &direntList, direntFilter, direntCompare);
  if (size != size2) {
    errno = EAGAIN;
    size = size2;
    size2 = -1;
  }
  if (size2 < 0) {
    while (size--) {
      free(direntList[size]);
    }
    free(direntList);
    return -1;
  }
  // Ok, let's parse
  _nodes = new list<Node*>;
  bool failed = false;
  char path[PATH_MAX];
  strcpy(path, _path);
  size_t path_len = strlen(path);
  path[path_len++] = '/';
  while (size--) {
    strcpy(&path[path_len], direntList[size]->d_name);
    Node *g = new Node(path);
    free(direntList[size]);
    if (g->type() == '?') {
      failed = true;
    }
    _nodes->push_front(g);
  }
  free(direntList);
  return failed ? -1 : 0;
}

void Node::deleteList() {
  if (_nodes != NULL) {
    list<Node*>::iterator i = _nodes->begin();
    while (i != _nodes->end()) {
      delete *i;
      i++;
    }
    delete _nodes;
    _nodes = NULL;
  }
}

int Node::mkdir_p(const char* path, mode_t mode) {
  // This is not meant to be efficient... at all!
  char dir_path[PATH_MAX] = "";
  size_t dir_path_len = 0;
  const char* pos;
  do {
    // "[/]a/b/c"
    pos = strchr(path, '/');
    if (pos != NULL) {
      memcpy(&dir_path[dir_path_len], path, pos - path);
      dir_path_len += pos - path;
      dir_path[dir_path_len] = '\0';
      path = pos + 1;
    } else {
      strcpy(&dir_path[dir_path_len], path);
    }
    // create dir
    if (dir_path_len > 0) {
      hlog_regression("dir_path (%zu) = '%s'", dir_path_len, dir_path);
      if ((::mkdir(dir_path, mode) < 0) && (errno != EEXIST)) {
        return -1;
      }
    }
    dir_path[dir_path_len++] = '/';
  } while (pos != NULL);
  return 0;
}

int Node::mkdir() {
  if (_type == '?') {
    stat();
  }
  if (isDir()) {
    errno = EEXIST;
    // Only a warning
    return 1;
  } else {
    if (::mkdir(_path, 0777)) {
      return -1;
    }
    stat();
  }
  return 0;
}

bool Node::isReadable(
    const char*     path) {
  int fd = ::open64(path, O_RDONLY|O_NOATIME|O_LARGEFILE, 0666);
  if (fd < 0) {
    return false;
  }
  ::close(fd);
  return true;
}

int Node::findExtension(
    char*           path,
    const char*     extensions[],
    ssize_t         original_length) {
  if (original_length < 0) {
    original_length = strlen(path);
  }
  int i = 0;
  while (extensions[i] != NULL) {
    strcpy(&path[original_length], extensions[i]);
    if (Node::isReadable(path)) {
      break;
    }
    ++i;
  }
  if (extensions[i] == NULL) {
    path[original_length] = '\0';
    return -1;
  }
  return i;
}