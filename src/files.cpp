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

int Path::pathcmp(const char* ss1, const char* ss2, ssize_t length) {
  const unsigned char* s1 = reinterpret_cast<const unsigned char*>(ss1);
  const unsigned char* s2 = reinterpret_cast<const unsigned char*>(ss2);
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
      return -1;
    } else
    if (*s2 == '/') {
      return 1;
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
    _size   = metadata.st_size;
    _mtime  = metadata.st_mtime;
    _uid    = metadata.st_uid;
    _gid    = metadata.st_gid;
    _mode   = metadata.st_mode & ~S_IFMT;
    _device = metadata.st_dev;
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
  return strcmp((*a)->d_name, (*b)->d_name);
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

static int mkdir_p_recursive(char* path, size_t path_len, mode_t mode) {
  int rc = mkdir(path, mode);
  if (rc < 0) {
    if (errno == ENOENT) {
      hlog_regression("mkdir_p_recursive(%s,%zu) Needs parent", path, path_len);
      char* reader = &path[path_len];
      while ((--reader >= path) && (*reader != '/'));
      if (reader < path) {
        hlog_regression("mkdir_p_recursive Error: reader < path");
        return -1;
      }
      *reader = '\0';
      if (mkdir_p_recursive(path, reader - path, mode) < 0) {
        return -1;
      }
      *reader = '/';
      rc = mkdir(path, mode);
    } else
    if (errno == EEXIST) {
      hlog_regression("mkdir_p_recursive(%s) Already there", path);
      rc = 0;
    }
  }
  if (rc < 0) {
    hlog_regression("mkdir_p_recursive(%s) %m", path);
  }
  return rc;
}

int Node::mkdir_p(const char* path, mode_t mode) {
  size_t path_len = strlen(path);
  if (path_len >= PATH_MAX) {
    errno = ENAMETOOLONG;
    return -1;
  }
  char dir_path[PATH_MAX];
  strcpy(dir_path, path);
  return mkdir_p_recursive(dir_path, path_len, mode);
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
