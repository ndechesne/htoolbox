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

#include <iostream>
#include <string>
#include <list>

#include <cctype>
#include <cstdio>

#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

using namespace std;

#include "buffer.h"
#include "filereaderwriter.h"
#include "hasher.h"
#include "unzipreader.h"
#include "zipwriter.h"
#include "asyncwriter.h"
#include "multiwriter.h"
#include "files.h"

using namespace hbackup;
using namespace htools;

Path::Path(const char* dir, const char* name) {
  if (name[0] != '\0') {
    _capacity = asprintf(&_path, "%s/%s", dir, name);
  } else {
    _path = strdup(dir);
    _capacity = strlen(_path);
  }
}

Path::Path(const Path& dir, const char* name) {
  if (name[0] != '\0') {
    _capacity = asprintf(&_path, "%s/%s", dir._path, name);
  } else {
    _path = strdup(dir._path);
    _capacity = dir._capacity;
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
  char* pos = strrchr(_path, '/');
  if (pos == NULL) {
    return ".";
  } else {
    Path rc(*this);
    pos = rc._path + (pos - _path);
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

const char* Path::noTrailingSlashes(char* path) {
  char* end = &path[strlen(path)];
  while ((--end >= path) && (*end == '/')) {}
  end[1] = '\0';
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
  }
  return rc;
}

bool Node::operator!=(const Node& right) const {
  return (_type != right._type)   || (_mtime != right._mtime)
      || (_size != right._size)   || (_uid != right._uid)
      || (_gid != right._gid)     || (_mode != right._mode)
      || (strcmp(basename(_path), basename(right._path)) != 0);
}

int Node::remove() {
  int rc = ::remove(_path);
  if (! rc) {
    _type = '?';
  }
  return rc;
}

int File::create() {
  if (_type == '?') {
    stat();
  }
  if (isValid()) {
    errno = EEXIST;
    // Only a warning
    return 1;
  } else {
    int readfile = ::open(_path, O_WRONLY | O_CREAT, 0666);
    if (readfile < 0) {
      return -1;
    }
    close(readfile);
    stat();
  }
  return 0;
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

int Directory::createList() {
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
  while (size--) {
    Node *g = new Node(Path(_path, direntList[size]->d_name));
    free(direntList[size]);
    if (! g->stat()) {
      switch (g->type()) {
        case 'f': {
          File *f = new File(*g);
          delete g;
          g = f;
        } break;
        case 'd': {
          Directory *d = new Directory(*g);
          delete g;
          g = d;
        } break;
        case 'l': {
          Link *l = new Link(*g);
          delete g;
          g = l;
        } break;
        default:;
      }
    } else {
      failed = true;
    }
    _nodes->push_front(g);
  }
  free(direntList);
  return failed ? -1 : 0;
}

void Directory::deleteList() {
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

int Directory::create() {
  if (_type == '?') {
    stat();
  }
  if (isValid()) {
    errno = EEXIST;
    // Only a warning
    return 1;
  } else {
    if (mkdir(_path, 0777)) {
      return -1;
    }
    stat();
  }
  return 0;
}


bool Link::operator!=(const Link& right) const {
  if (static_cast<Node>(*this) != static_cast<Node>(right)) {
    return true;
  }
  return strcmp(_link, right._link) != 0;
}


class GzipExtraFieldSize {
  union {
    char            bytes[12];
    struct {
      char          si1;
      char          si2;
      short         len;
      long long     size;
    } fields;
  } extra;
public:
  GzipExtraFieldSize(long long size) {
    extra.fields.si1  = 'S';
    extra.fields.si2  = 'Z';
    extra.fields.len  = 8;
    extra.fields.size = size;
  }
  GzipExtraFieldSize(const unsigned char* field) {
    extra.fields.size = -1;
    if ((field[0] == 'S') && (field[1] == 'Z')) {
      memcpy(extra.bytes, field, 4);
      if (extra.fields.len <= 8) {
        memcpy(&extra.bytes[4], &field[4], extra.fields.len);
      } else {
        extra.fields.len  = 0;
      }
    } else {
      extra.fields.si1  = '-';
    }
  }
  operator unsigned char* () {
    return reinterpret_cast<unsigned char*>(extra.bytes);
  }
  operator long long () {
    return extra.fields.size;
  }
  int length() const {
    if (extra.fields.si1 != 'S') {
      return -1;
    }
    if (extra.fields.len == 0) {
      return 0;
    }
    return extra.fields.len + 4;
  }
};


struct Stream::Private {
  FileReaderWriter*   fd;                 // underlying file reader/writer
  IReaderWriter*      rw;                 // reader/writer
  bool                writer;             // know which mode we're in
  bool                open;               // file is open
  long long           size;               // uncompressed file data size (bytes)
  long long           progress;           // transfered size, for progress
  progress_f          progress_callback;  // function to report progress
  cancel_f            cancel_callback;    // function to check for cancellation
};

Stream::Stream(const char* path, bool writer, bool need_hash, int compression) :
  File(path), _d(new Private) {
  _d->writer            = writer;
  _d->open              = false;
  _d->progress_callback = NULL;
  _d->cancel_callback   = NULL;
  _d->size              = -1;
  _d->rw = _d->fd = new FileReaderWriter(path, writer);
  if (writer) {
    _size = 0;
    if (need_hash) {
      _d->rw = new Hasher(_d->rw, true, Hasher::md5, _hash);
    } else {
      strcpy(_hash, "");
    }
    if (compression > 0) {
      _d->rw = new ZipWriter(_d->rw, true, compression);
    }
  } else {
    if (compression > 0) {
      _d->rw = new UnzipReader(_d->rw, true);
    }
    if (need_hash) {
      _d->rw = new Hasher(_d->rw, true, Hasher::md5, _hash);
    } else {
      strcpy(_hash, "");
    }
  }
}

Stream::~Stream() {
  if (isOpen()) {
    close();
  }
  delete _d->rw;
  delete _d;
}

bool Stream::isOpen() const {
  return (_d->open);
}

bool Stream::isWriteable() const {
  return _d->writer;
}

int Stream::open() {
  if (isOpen()) {
    errno = EBUSY;
    return -1;
  }
  _d->size     = 0;
  _d->progress = 0;
  if (_d->rw->open() < 0) {
    return -1;
  }
  _d->open = true;
  return 0;
}

int Stream::close() {
  if (! isOpen()) {
    errno = EBADF;
    return -1;
  }
  int rc = _d->rw->close();
  if (_d->writer) {
    // Update metadata
    stat();
  }
  _d->open = false;
  return rc;
}

long long Stream::progress() const {
  return _d->progress;
}

void Stream::setProgressCallback(progress_f progress) {
  _d->progress_callback = progress;
}

ssize_t Stream::read(void* buffer, size_t asked) {
  if (! isOpen()) {
    errno = EBADF;
    return -1;
  }
  if (isWriteable()) {
    errno = EPERM;
    return -1;
  }
  // Get data
  ssize_t size = _d->rw->read(buffer, asked);
  // Check result
  if (size < 0) {
    // errno set by read
    return -1;
  }
  // Update progress indicator (size read)
  if (size > 0) {
    long long previous = _d->progress;
    _d->progress = _d->fd->offset();
    if ((_d->progress_callback != NULL) && (_d->progress != previous)) {
      (*_d->progress_callback)(previous, _d->progress, _size);
    }
    _d->size += size;
  }
  return size;
}

ssize_t Stream::write(
    const void*     buffer,
    size_t          size) {
  if (! isOpen()) {
    errno = EBADF;
    return -1;
  }
  if (! isWriteable()) {
    errno = EPERM;
    return -1;
  }
  // Write data to file
  ssize_t ssize = size;
  if (_d->rw->write(buffer, size) != ssize) {
    return -1;
  }
  // Update progress indicator (size written)
  if (size > 0) {
    long long previous = _d->progress;
    _d->progress += size;
    if (_d->progress_callback != NULL) {
      (*_d->progress_callback)(previous, _d->progress, _size);
    }
    // Writing, so uncompressed data size and size are the same
    _size += size;
    _d->size += size;
  }
  return size;
}

void Stream::setCancelCallback(cancel_f cancel) {
  _d->cancel_callback = cancel;
}

long long Stream::dataSize() const {
  return _d->size;
}
