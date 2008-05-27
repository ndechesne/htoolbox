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

#include <iostream>
#include <string>
#include <list>

#include <cctype>
#include <cstdio>

#include <pthread.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <openssl/md5.h>
#include <openssl/evp.h>
#include <zlib.h>

// I want to use the C file functions
#undef open
#undef open64
#undef close
#undef lseek
#undef read
#undef write
namespace std {
  using ::open;
  using ::open64;
  using ::close;
  using ::lseek;
  using ::read;
  using ::write;
}

using namespace std;

#include "lock.h"
#include "buffer.h"
#include "line.h"
#include "files.h"

using namespace hbackup;

Path::operator const char*() const {
  if (_length > 0) {
    return _const_path;
  } else {
    return "";
  }
}

const char* Path::operator=(const char* path) {
  if (_path != NULL) {
    free(_path);
  }
  _path = strdup(path);
  _const_path = _path;
  _length = strlen(_path);
  return _path;
}

const Path& Path::operator=(const Path& path) {
  if (_path != NULL) {
    free(_path);
  }
  _path = strdup(path._path);
  _const_path = _path;
  _length = path._length;
  return *this;
}

Path::Path(const char* dir, const char* name) {
  if (name[0] == '\0') {
    _path = strdup(dir);
  } else {
    asprintf(&_path, "%s/%s", dir, name);
  }
  _const_path = _path;
  _length = strlen(_path);
}

Path::Path(const Path& path, const char* name) {
  asprintf(&_path, "%s/%s", path._path, name);
  _const_path = _path;
  _length = strlen(_path);
}

Path::~Path() {
  free(_path);
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
  const char* end = strrchr(_path, '/');
  if (end == NULL) {
    return ".";
  }
  int   length = end - _path;
  char* dir    = (char*) malloc(length + 1);
  strncpy(dir, _path, length);
  dir[length] = '\0';
  Path rc = dir;
  free(dir);
  return rc;
}

const Path& Path::fromDos() {
  int   count  = _length;
  bool  proven = false;
  char* reader = _path;
  while (count--) {
    if (*reader == '\\') {
      *reader = '/';
      proven = true;
    }
    reader++;
  }
  // Upper case drive letter
  if (proven && (_length >= 2)) {
    if ((_path[1] == ':') && (_path[2] == '/')
      && (_path[0] >= 'a') && (_path[0] <= 'z')) {
      _path[0] -= 0x20;
    }
  }
  return *this;
}

const Path& Path::noTrailingSlashes() {
  char* end = &_path[_length];
  while ((--end >= _path) && (*end == '/')) {
    *end = '\0';
    _length--;
  }
  return *this;
}

int Path::compare(const char* s1, const char* s2, size_t length) {
  while (true) {
    if (length == 0) {
      return 0;
    }
    if (*s1 == '\0') {
      if (*s2 == '\0') {
        return 0;
      } else {
        return -1;
      }
    } else {
      if (*s2 == '\0') {
        return 1;
      } else {
        if (*s1 == '/') {
          if (*s2 == '/') {
            s1++;
            s2++;
          } else {
            if (*s2 < ' ') {
              return 1;
            } else {
              return -1;
            }
          }
        } else {
          if (*s2 == '/') {
            if (*s1 < ' ') {
              return -1;
            } else {
              return 1;
            }
          } else {
            if (*s1 < *s2) {
              return -1;
            } else if (*s1 > *s2) {
              return 1;
            } else {
              s1++;
              s2++;
            }
          }
        }
      }
    }
    if (length > 0) {
      length--;
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
      || (strcmp(_path.basename(), right._path.basename()) != 0);
}

int Node::remove() {
  int rc = std::remove(_path);
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
    int readfile = open(_path, O_WRONLY | O_CREAT, 0666);
    if (readfile < 0) {
      return -1;
    }
    close(readfile);
    stat();
  }
  return 0;
}

int Directory::createList() {
  DIR* directory = opendir(_path);
  if (directory == NULL) return -1;

  struct dirent *dir_entry;
  while (((dir_entry = readdir(directory)) != NULL)) {
    // Ignore . and ..
    if (!strcmp(dir_entry->d_name, ".") || !strcmp(dir_entry->d_name, "..")) {
      continue;
    }
    Node *g = new Node(Path(_path, dir_entry->d_name));
    g->stat();
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
    list<Node*>::iterator i = _nodes.begin();
    while ((i != _nodes.end()) && (*(*i) < *g)) {
      i++;
    }
    _nodes.insert(i, g);
  }

  closedir(directory);
  return 0;
}

void Directory::deleteList() {
  list<Node*>::iterator i = _nodes.begin();
  while (i != _nodes.end()) {
    delete *i;
    i++;
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
  if (*((Node*)this) != (Node)right) {
    return true;
  }
  return strcmp(_link, right._link) != 0;
}


struct Stream::Private {
  int               fd;                 // file descriptor
  mode_t            mode;               // file open mode
  long long         size;               // uncompressed file data size (bytes)
  long long         progress;           // transfered size, for progress
  Buffer            buffer_comp;        // Buffer for [de]compression
  Buffer            buffer_data;        // Buffer for data
  EVP_MD_CTX*       ctx;                // openssl resources
  z_stream*         strm;               // zlib resources
  progress_f        progress_callback;  // function to report progress
  cancel_f          cancel_callback;    // function to check for cancellation
};

void Stream::md5sum(char* out, const unsigned char* in, int bytes) {
  const char* hex = "0123456789abcdef";

  while (bytes-- != 0) {
    *out++ = hex[*in >> 4];
    *out++ = hex[*in & 0xf];
    in++;
  }
  *out = '\0';
}

Stream* Stream::select(
    Path            path,
    vector<string>& extensions,
    unsigned int*   no) {
  const char* char_base = path;
  string base = char_base;
  for (unsigned int i = 0; i < extensions.size(); i++) {
    Stream* stream = new Stream((base + extensions[i]).c_str());
    if (stream->isValid()) {
      if (no != NULL) {
        *no = i;
      }
      return stream;
    }
    delete stream;
  }
  return NULL;
}

Stream::Stream(Path path) : File(path), _d(new Private) {
  _d->fd                = -1;
  _d->progress_callback = NULL;
  _d->cancel_callback   = NULL;
  _d->size              = -1;
}

Stream::~Stream() {
  if (isOpen()) {
    close();
  }
  delete _d;
}

bool Stream::isOpen() const {
  return _d->fd != -1;
}

bool Stream::isWriteable() const {
  return (_d->mode & O_WRONLY) != 0;
}

int Stream::open(
    const char*     req_mode,
    unsigned int    compression,
    int             cache,
    bool            checksum) {
  if (isOpen()) {
    errno = EBUSY;
    return -1;
  }

  _d->mode = O_NOATIME | O_LARGEFILE;

  switch (req_mode[0]) {
  case 'w':
    _d->mode = O_WRONLY | O_CREAT | O_TRUNC;
    _size = 0;
    break;
  case 'r':
    _d->mode = O_RDONLY;
    break;
  default:
    errno = EPERM;
    return -1;
  }

  _d->size     = 0;
  _d->progress = 0;
  _d->fd = std::open64(_path, _d->mode, 0666);
  if (! isOpen()) {
    // errno set by open
    return -1;
  }

  // Create buffer for data cacheing
  if (cache > 0) {
    _d->buffer_data.create(cache);
  } else
  if (isWriteable() && (cache < 0)) {
    // Create buffer to cache input data
    _d->buffer_data.create();
  }

  // Create openssl resources
  if (checksum) {
    _d->ctx = new EVP_MD_CTX;
    if (_d->ctx != NULL) {
      EVP_DigestInit(_d->ctx, EVP_md5());
    }
  } else {
    _d->ctx = NULL;
  }

  // Create zlib resources
  if (compression > 0) {
    _d->strm           = new z_stream;
    _d->strm->zalloc   = Z_NULL;
    _d->strm->zfree    = Z_NULL;
    _d->strm->opaque   = Z_NULL;
    _d->strm->avail_in = 0;
    _d->strm->next_in  = Z_NULL;
    if (isWriteable()) {
      // Compress
      if (deflateInit2(_d->strm, compression, Z_DEFLATED, 16 + 15, 9,
          Z_DEFAULT_STRATEGY)) {
        return -2;
      }
    } else {
      // De-compress
      if (inflateInit2(_d->strm, 32 + 15)) {
        return -2;
      }
    }
    // Create buffer for decompression
    _d->buffer_comp.create();
  } else {
    _d->strm = NULL;
  }
  return 0;
}

int Stream::close() {
  if (! isOpen()) {
    errno = EBADF;
    return -1;
  }
  bool failed = false;

  // Write last few bytes in case of compressed write
  if (isWriteable()) {
    if (write(NULL, 0) < 0) {
      failed = true;
    }
  }

  // Compute checksum
  if (_d->ctx != NULL) {
    unsigned char checksum[36];
    size_t        length;

    EVP_DigestFinal(_d->ctx, checksum, &length);
    free(_checksum);
    _checksum = (char*) malloc(2 * length + 1);
    md5sum(_checksum, checksum, length);
    delete _d->ctx;
    _d->ctx = NULL;
  }

  // Destroy zlib resources
  if (_d->strm != NULL) {
    if (isWriteable()) {
      deflateEnd(_d->strm);
    } else {
      inflateEnd(_d->strm);
    }
    delete _d->strm;
    _d->strm = NULL;
    _d->buffer_comp.destroy();
  }

  if (std::close(_d->fd)) {
    failed = true;
  }
  _d->fd = -1;

  // Destroy cacheing buffer if any
  if (_d->buffer_data.exists()) {
    _d->buffer_data.destroy();
  }

  // Update metadata
  stat();
  return failed ? -1 : 0;
}

long long Stream::progress() const {
  return _d->progress;
}

void Stream::setProgressCallback(progress_f progress) {
  _d->progress_callback = progress;
}

int Stream::digest_update(
    const void*     buffer,
    size_t          size) {
  size_t      max = 409600; // That's as much as openssl/md5 accepts
  const char* reader = (const char*) buffer;
  while (size > 0) {
    size_t length;
    if (size >= max) {
      length = max;
    } else {
      length = size;
    }
    if (EVP_DigestUpdate(_d->ctx, reader, length) == 0) {
      return -1;
    }
    reader += length;
    size   -= length;
  }
  return 0;
}

ssize_t Stream::read_decompress(
    const void*     buffer,
    size_t          asked,
    size_t*         given) {
  ssize_t size = 0;
  // Fill in buffer
  if (_d->buffer_comp.isEmpty()) {
    size = std::read(_d->fd, _d->buffer_comp.writer(),
      _d->buffer_comp.writeable());
    if (size < 0) {
      return -1;
    }
    _d->buffer_comp.written(size);
    _d->strm->avail_in = _d->buffer_comp.readable();
    _d->strm->next_in  = (unsigned char*) _d->buffer_comp.reader();
  }
  _d->strm->avail_out = asked;
  _d->strm->next_out  = (unsigned char*) buffer;
  switch (inflate(_d->strm, Z_NO_FLUSH)) {
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
      return -2;
  }
  // Used all decompression buffer
  if (_d->strm->avail_out != 0) {
    _d->buffer_comp.empty();
  }
  *given = asked - _d->strm->avail_out;
  return size;
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
  ssize_t size  = 0;
  size_t  given = 0;
  if (! _d->buffer_data.exists()) {
    if (_d->strm != NULL) {
      size = read_decompress(buffer, asked, &given);
    } else {
      size = given = std::read(_d->fd, buffer, asked);
    }
  } else if (_d->buffer_data.isEmpty()) {
    if (_d->strm != NULL) {
      size = read_decompress(_d->buffer_data.writer(),
        _d->buffer_data.writeable(), &given);
    } else {
      size = given = std::read(_d->fd, _d->buffer_data.writer(),
        _d->buffer_data.writeable());
    }
  }

  // Check result
  if (size < 0) {
    // errno set by read
    return -1;
  }

  // Update progress indicator (size read)
  if (size > 0) {
    long long previous = _d->progress;
    _d->progress += size;
    if (_d->progress_callback != NULL) {
      (*_d->progress_callback)(previous, _d->progress, _size);
    }
  }

  // Update hash
  if (_d->buffer_data.exists()) {
    if ((_d->ctx != NULL) && digest_update(_d->buffer_data.writer(), given)) {
      return -3;
    }
    _d->buffer_data.written(given);
    // Special case for getLine
    if ((asked == 0) && (buffer == NULL)) {
      return _d->buffer_data.readable();
    }
    given = _d->buffer_data.read(buffer, asked);
  } else {
    if ((_d->ctx != NULL) && digest_update(buffer, given)) {
      return -3;
    }
  }

  _d->size += given;
  return given;
}

ssize_t Stream::write_all(
    const void*     buffer,
    size_t          count) {
  // Checksum computation
  if (_d->ctx != NULL) {
    if (digest_update(buffer, count)) {
      return -2;
    }
  }

  const char* reader = (const char*) buffer;
  size_t  size = count;
  ssize_t wlength;
  do {
    wlength = std::write(_d->fd, reader, size);
    if (wlength < 0) {
      // errno set by write
      return -1;
    }
    reader += wlength;
    size    -= wlength;
  } while ((size != 0) && (errno == 0));  // errno maybe set with wlength == 0
  return size != 0 ? -1 : count;
}

ssize_t Stream::write_compress(
    const void*     buffer,
    size_t          count,
    bool            finish) {
  _d->strm->avail_in = count;
  _d->strm->next_in  = (unsigned char*) buffer;

  // Flush result to file (no real cacheing)
  do {
    // Buffer is considered empty to start with. and is always flushed
    _d->strm->avail_out = _d->buffer_comp.writeable();
    // Casting away the constness here!!!
    _d->strm->next_out  = (unsigned char*) _d->buffer_comp.writer();
    deflate(_d->strm, finish ? Z_FINISH : Z_NO_FLUSH);
    ssize_t length = _d->buffer_comp.writeable() - _d->strm->avail_out;
    if (length != write_all(_d->buffer_comp.reader(), length)) {
      return -1;
    }
  } while (_d->strm->avail_out == 0);
  return count;
}

ssize_t Stream::write(
    const void*     buffer,
    size_t          given) {
  static bool finish = true;

  if (! isOpen()) {
    errno = EBADF;
    return -1;
  }

  if (! isWriteable()) {
    errno = EPERM;
    return -1;
  }

  _d->size += given;

  if ((given == 0) && (buffer == NULL)) {
    if (finish) {
      // Finished last time
      return 0;
    }
    finish = true;
  } else {
    finish = false;
  }

  bool direct_write = false;
  ssize_t size = 0;
  if (_d->buffer_data.exists()) {
    // If told to finish or buffer is going to overflow, flush it to file
    if (finish || (given > _d->buffer_data.writeable())) {
      // One or two writes (if end of buffer + beginning of it)
      while (_d->buffer_data.readable() > 0) {
        ssize_t length = _d->buffer_data.readable();
        if (_d->strm == NULL) {
          if (length != write_all(_d->buffer_data.reader(), length)) {
            return -1;
          }
        } else {
          if (length != write_compress(_d->buffer_data.reader(), length)) {
            return -1;
          }
        }
        _d->buffer_data.readn(length);
        size += length;
      }
      // Buffer is now flushed, restore full writeable capacity
      _d->buffer_data.empty();
    }

    // If told to finish or more data than buffer can handle, just write
    if (finish || (given > _d->buffer_data.writeable())) {
      direct_write = true;
    } else {
      // Refill buffer
      _d->buffer_data.write(buffer, given);
    }
  } else {
    direct_write = true;
  }

  // Write data to file
  if (direct_write) {
    ssize_t length = given;
    if (_d->strm == NULL) {
      if ((buffer != NULL) && (length != write_all(buffer, length))) {
        return -1;
      }
    } else {
      if (length != write_compress(buffer, length, finish)) {
        return -1;
      }
    }
    size += length;
  }

  // Update progress indicator (size written)
  if (size > 0) {
    long long previous = _d->progress;
    _d->progress += size;
    if (_d->progress_callback != NULL) {
      (*_d->progress_callback)(previous, _d->progress, _size);
    }
  }

  _size += size;
  return given;
}

ssize_t Stream::getLine(
    char**          buffer,
    unsigned int*   buffer_capacity,
    bool*           end_of_line_found) {
  // Need a buffer to speed things up
  if (! _d->buffer_data.exists()) {
    _d->buffer_data.create();
  }

  // Initialize return values
  bool         found = false;
  unsigned int count  = 0;
  char*        writer = &(*buffer)[count];

  // Find end of line or end of file
  do {
    // Make read fill up the buffer
    ssize_t size = 1;
    if (_d->buffer_data.isEmpty()) {
      size = read(NULL, 0);
      if (size < 0) {
        return -1;
      }
    }
    // Make sure we have a buffer
    if (*buffer == NULL) {
      *buffer_capacity = 100;
      *buffer = (char*) malloc(*buffer_capacity);
      writer = *buffer;
    }
    if (size == 0) {
      break;
    }
    const char* reader = _d->buffer_data.reader();
    size_t      length = _d->buffer_data.readable();
    while (length > 0) {
      length--;
      // Check for space
      if (count >= *buffer_capacity) {
        *buffer_capacity += 100;
        *buffer = (char*) realloc(*buffer, *buffer_capacity);
        writer = &(*buffer)[count];
      }

      if (*reader == '\n') {
        found = true;
        break;
      }
      *writer++ = *reader++;
      count++;
    }
    _d->buffer_data.readn(_d->buffer_data.readable() - length);
  } while (! found);
  *writer = '\0';

  if (end_of_line_found != NULL) {
    *end_of_line_found = found;
  }
  return count;
}

ssize_t Stream::putLine(
    const char*     buffer) {
  ssize_t length = strlen(buffer);
  ssize_t size   = write(buffer, length);
  if (size < length) {
    return -1;
  }
  if (write("\n", 1) != 1) {
    return -1;
  }
  return size + 1;
}

void Stream::setCancelCallback(cancel_f cancel) {
  _d->cancel_callback = cancel;
}

int Stream::computeChecksum() {
  if (! isOpen()) {
    errno = EBADF;
    return -1;
  }
  // Need a buffer to store temporary data
  if (! _d->buffer_data.exists()) {
    _d->buffer_data.create();
  }
  long long size = 0;
  bool      eof  = false;

  do {
    ssize_t length = read(NULL, 0);
    _d->buffer_data.empty();
    if (length < 0) {
      return -1;
    }
    eof = (length == 0);
    size += length;
    if ((_d->cancel_callback != NULL) && ((*_d->cancel_callback)(1))) {
      errno = ECANCELED;
      return -1;
    }
  } while (! eof);
  return 0;
}

struct CopyData {
  Buffer    buffer;
  Lock      read_lock;
  Lock      write_lock;
  Stream&   source;
  long long size;
  int       status;
  CopyData(Stream& s) : buffer(1 << 20), source(s), size(0), status(1) {}
};

static void* read_task(void* data) {
  CopyData& cd = *static_cast<CopyData*>(data);
  ssize_t length;
  do {
    if (! cd.buffer.isFull()) {
      // Fill up only up to half of capacity, for simultenous read and write
      length = cd.buffer.capacity() >> 1;
      if (static_cast<size_t>(length) > cd.buffer.writeable()) {
        length = cd.buffer.writeable();
      }
      length = cd.source.read(cd.buffer.writer(), length);
      if (length < 0) {
        cd.status = -1;
      }
      cd.buffer.written(length);
      // Allow write task to run
      cd.write_lock.release();
      cd.size += length;
      if (length == 0) {
        cd.status = 0;
      }
    } else {
      // Timed lock, as a deadlock is possible with this system
      cd.read_lock.lock(3);
    }
  } while (cd.status > 0);
  // Insert sleep and output here to check for not-handled join
  return NULL;
}

int Stream::copy(Stream& source) {
  if ((! isOpen()) || (! source.isOpen())) {
    errno = EBADF;
    return -1;
  }
  CopyData cd(source);
  pthread_t child;
  if (pthread_create(&child, NULL, read_task, &cd)) {
    return -1;
  }

  bool failed = false;
  do {
    if (! cd.buffer.isEmpty()) {
      // Write as much as possible
      ssize_t length = write(cd.buffer.reader(), cd.buffer.readable());
      if (length < 0) {
        failed = true;
        break;
      }
      cd.buffer.readn(length);
      // Allow read task to run
      cd.read_lock.release();
    } else if (cd.status > 0) {
      // Timed lock, as a deadlock is possible with this system
      cd.write_lock.lock(3);
    }
    if ((_d->cancel_callback != NULL) && ((*_d->cancel_callback)(2))) {
      errno = ECANCELED;
      failed = true;
      break;
    }
  } while ((cd.status > 0) || ! cd.buffer.isEmpty());
  // Make sure read can terminate
  if (failed) {
    cd.status = -1;
    cd.read_lock.release();
  }
  // Wait for read task termination, check status
  int rc = pthread_join(child, NULL);
  if (rc != 0) {
    errno = rc;
  }
  if ((rc != 0) || (cd.status == -1)) {
    return -1;
  }
  // Check that sizes match
  if (cd.size != source._d->size) {
    errno = EAGAIN;
    return -1;
  }
  return 0;
}

int Stream::compare(Stream& source, long long length) {
  if ((! isOpen()) || (! source.isOpen())) {
    errno = EBADF;
    return -1;
  }
  int             buf_size = 1024;
  unsigned char*  buffer1  = (unsigned char*) malloc(buf_size);
  unsigned char*  buffer2  = (unsigned char*) malloc(buf_size);
  int             rc = -2;

  while (rc == -2) {
    bool eof;
    // Make we stop when needed
    if ((length >= 0) && (length < buf_size)) {
      buf_size = length;
    }
    // Fill in buffer1
    ssize_t size1 = 0;
    eof = false;
    while (! eof && (size1 < buf_size)) {
      ssize_t size = read(&buffer1[size1], buf_size - size1);
      if (size < 0) {
        rc = -1;
        break;
      } else if (size == 0) {
        eof = true;
      }
      size1 += size;
    }
    // Fill in buffer2
    ssize_t size2 = 0;
    if (rc == -2) {
      eof = false;
      while (! eof && (size2 < buf_size)) {
        ssize_t size = source.read(&buffer2[size2], buf_size - size2);
        if (size < 0) {
          rc = -1;
          break;
        } else if (size == 0) {
          eof = true;
        }
        size2 += size;
      }
    }
    if (rc == -2) {
      // Compare size
      if (size1 != size2) {
        // Differ in size
        rc = 1;
      } else
      // Check end of files
      if (size1 == 0) {
        rc = 0;
      } else
      // Compare buffers
      if (memcmp(buffer1, buffer2, size1) != 0) {
        // Differ in contents
        rc = 1;
      } else
      // Check buffer fill
      if (size1 < buf_size) {
        rc = 0;
      } else
      // How much is left to compare?
      if (length >= 0) {
        length -= size1;
        if (length <= 0) {
          rc = 0;
        }
      }
    }
    if ((_d->cancel_callback != NULL) && ((*_d->cancel_callback)(3))) {
      errno = ECANCELED;
      return -1;
    }
  }
  free(buffer1);
  free(buffer2);
  return rc;
}

unsigned long Stream::originalSize() const {
  if (isOpen()) {
    errno = EBUSY;
    return -1;
  }
  int fd = std::open64(_path, O_RDONLY, 0666);
  if (fd < 0) {
    // errno set by open
    return -1;
  }
  union {
    char buffer[4];
    unsigned long size;
  } data;
  bool failed = false;
  if (std::lseek(fd, -4, SEEK_END) < 0) {
    // errno set by lseek
    failed = true;
  } else
  if (std::read(fd, data.buffer, 4) != 4) {
    // errno set by read
    failed = true;
  }
  std::close(fd);
  return failed ? -1 : data.size;
}

long long Stream::dataSize() const {
  return _d->size;
}

int Stream::getParams(
    vector<string>& params,
    unsigned char   flags,
    const char*     delims,
    const char*     quotes,
    const char*     comments) {
  Line line;
  bool eol;

  params.clear();
  int rc = getLine(line, &eol);
  if (rc < 0) {
    return -1;
  } else
  if (! eol) {
    if (rc == 0) {
      return 0;
    } else
    if ((flags & flags_need_lf) != 0) {
      return -1;
    }
  }
  if ((flags & flags_accept_cr_lf)
  && (line.size() > 0)
  && (line[line.size() - 1] == '\r')) {
    line.erase(line.size() - 1);
  }
  rc = extractParams(line, params, flags, 0, delims, quotes, comments);
  if (rc < 0) {
    return -1;
  }
  // Signal warning
  if (rc > 0) {
    return 2;
  }
  return 1;
}

// Public functions
int Stream::extractParams(
    const char*     line,
    vector<string>& params,
    unsigned char   flags,
    unsigned int    max_params,
    const char*     delims,
    const char*     quotes,
    const char*     comments) {
  const char* read   = line;
  char* param        = (char*) malloc(strlen(line) + 1);
  char* write        = NULL;
  bool no_increment  = true;  // Set to re-use character next time round
  bool skip_delims;           // Set to ignore any incoming blank
  bool check_comment;         // Set after a blank was found
  bool escaped       = false; // Set after a \ was found
  bool was_escaped   = false; // To deal with quoted paths ending in '\' (DOS!)
  bool decode_next   = false; // Set when data is expected to follow
  char quote         = -1;    // Last quote when decoding, -1 when not decoding
  bool value_end     = false; // Set to signal the end of a parameter
  bool ended_well    = true;  // Set when all quotes were matched

  if (flags & flags_empty_params) {
    skip_delims   = false;
    check_comment = true;
  } else {
    skip_delims   = true;
    check_comment = false;
  }

  do {
    if (no_increment) {
      no_increment = false;
    } else {
      read++;
      if (*read != 0) {
        // This is only used at end of line
        was_escaped = false;
      }
    }
    if (check_comment) {
      // Before deconfig, verify it's not a comment
      bool comment = false;
      for (const char* c = comments; *c != '\0'; c++) {
        if (*c == *read) {
          comment = true;
          break;
        }
      }
      if (comment) {
        // Nothing more to do
        break;
      } else if (decode_next && (*read == 0)) {
        // Were expecting one more parameter but met eof
        *param    = '\0';
        value_end = true;
      } else {
        check_comment = false;
        // Do not increment, as this a meaningful character
        no_increment = true;
      }
    } else if (quote > 0) {
      // Decoding quoted string
      if (*read == quote) {
        if (escaped) {
          *write++ = *read;
          escaped  = false;
          if (flags & flags_dos_catch) {
            was_escaped = true;
          }
        } else {
          // Found match, stop decoding
          value_end = true;
        }
      } else if (! (flags & flags_no_escape) && (*read == '\\')) {
        escaped = true;
      } else {
        if (escaped) {
          *write++ = '\\';
          escaped  = false;
        }
        // Do the DOS trick if eof met and escape found
        if (*read == 0) {
          if (was_escaped) {
            write--;
            *write++ = '\\';
          } else {
            ended_well = false;
          }
          value_end = true;
        } else {
          *write++ = *read;
        }
      }
    } else if (skip_delims || (quote == 0)) {
      bool delim = false;
      for (const char* c = delims; *c != '\0'; c++) {
        if (*c == *read) {
          delim = true;
          break;
        }
      }
      if (quote == 0) {
        // Decoding unquoted string
        if (delim) {
          // Found blank, stop decoding
          value_end = true;
          if ((flags & flags_empty_params) != 0) {
            // Delimiter found, data follows
            decode_next = true;
          }
        } else {
          if (*read == 0) {
            value_end = true;
          } else {
            // Take character into account for parameter
            *write++ = *read;
          }
        }
      } else if (! delim) {
        // No more blanks, check for comment
        skip_delims   = false;
        check_comment = true;
        // Do not increment, as this is the first non-delimiting character
        no_increment = true;
      }
    } else {
      // Start decoding new string
      write = param;
      quote = 0;
      for (const char* c = quotes; *c != '\0'; c++) {
        if (*c == *read) {
          quote = *c;
          break;
        }
      }
      if (quote == 0) {
        // Do not increment,as this is no quote and needs to be used
        no_increment = true;
      }
      decode_next = false;
    }
    if (value_end) {
      *write++ = '\0';
      params.push_back(string(param));
      if ((max_params > 0) && (params.size () >= max_params)) {
        // Reached required number of parameters
        break;
      }
      if (flags & flags_empty_params) {
        // Do not skip blanks but check for comment
        check_comment = true;
      } else {
        // Skip any blank on the way
        skip_delims = true;
      }
      quote       = -1;
      value_end   = false;
    }
  } while (*read != '\0');

  free(param);
  if (! ended_well) {
    return 1;
  }
  return 0;
}
