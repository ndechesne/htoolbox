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
#undef read
#undef write
namespace std {
  using ::open;
  using ::open64;
  using ::close;
  using ::read;
  using ::write;
}

using namespace std;

#include "files.h"

using namespace hbackup;

const char* Path::c_str() const {
  if (_length > 0) {
    return _const_path;
  } else {
    return "";
  }
}

string Path::str() const {
  return string(c_str());
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

ostream& hbackup::operator<<(ostream& os, Path const & path) {
  return os << path.c_str();
}

Path::Path(const char* dir, const char* name) {
  if (dir[0] == '\0') {
    _path = strdup(name);
  } else if (name[0] == '\0') {
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

char* Path::dirname(const char* path) {
  const char* end = strrchr(path, '/');
  char*       dir;
  if (end == NULL) {
    dir = strdup(".");
  } else {
    int length = end - path;
    dir = (char*) malloc(length + 1);
    strncpy(dir, path, length);
    dir[length] = '\0';
  }
  return dir;
}

char* Path::fromDos(const char* dos_path) {
  char* unix_path = strdup(dos_path);
  int   length    = strlen(dos_path);
  if (length > 0) {
    int         count  = length;
    bool        proven = false;
    const char* reader = dos_path;
    char*       writer = unix_path;
    while (count--) {
      if (*reader == '\\') {
        proven = true;
        *writer = '/';
      } else {
        *writer = *reader;
      }
      reader++;
      writer++;
    }
    // Upper case drive letter
    if (proven && (length >= 2)) {
      if ((dos_path[1] == ':') && (dos_path[2] == '\\')
        && (dos_path[0] >= 'a') && (dos_path[0] <= 'z')) {
        unix_path[0] -= 0x20;
      }
    }
  }
  return unix_path;
}

char* Path::noTrailingSlashes(char* dir_path) {
  char* end = &dir_path[strlen(dir_path)];
  while ((--end >= dir_path) && (*end == '/')) {
    *end = '\0';
  }
  return dir_path;
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

int Path::countBlocks(char c) const {
  const char* reader = _path;
  size_t      length = _length;
  int         blocks = 0;
  bool        just_seen_one = true;
  while (length-- > 0) {
    if (*reader++ == c) {
      just_seen_one = true;
    } else if (just_seen_one) {
      blocks++;
      just_seen_one = false;
    }
  }
  return blocks;
}

int Node::stat() {
  struct stat64 metadata;
  int rc = lstat64(_path.c_str(), &metadata);
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
  int rc = std::remove(_path.c_str());
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
    int readfile = open(_path.c_str(), O_WRONLY | O_CREAT, 0666);
    if (readfile < 0) {
      return -1;
    }
    close(readfile);
    stat();
  }
  return 0;
}

int Directory::createList() {
  DIR* directory = opendir(_path.c_str());
  if (directory == NULL) return -1;

  struct dirent *dir_entry;
  while (((dir_entry = readdir(directory)) != NULL)) {
    // Ignore . and ..
    if (!strcmp(dir_entry->d_name, ".") || !strcmp(dir_entry->d_name, "..")) {
      continue;
    }
    Node *g = new Node(Path(_path.c_str(), dir_entry->d_name));
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
    if (mkdir(_path.c_str(), 0777)) {
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
  unsigned char*    buffer;             // buffer for read/write operations
  unsigned char*    reader;             // buffer read pointer
  ssize_t           length;             // buffer length
  EVP_MD_CTX*       ctx;                // openssl resources
  z_stream*         strm;               // zlib resources
  progress_f        progress_callback;  // function to report progress
  cancel_f          cancel_callback;    // function to check for cancellation
};

void Stream::md5sum(char* out, const unsigned char* in, int bytes) {
  char* hex   = "0123456789abcdef";

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
  string base = path.c_str();
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

Stream::Stream(Path path) : File(path) {
  _d = new Private;
  _d->fd                = -1;
  _d->progress_callback = NULL;
  _d->cancel_callback   = NULL;
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

int Stream::open(const char* req_mode, int compression) {
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
  _d->length   = 0;
  _d->fd = std::open64(_path.c_str(), _d->mode, 0666);
  if (! isOpen()) {
    // errno set by open
    return -1;
  }

  // Create buffer if not told not to
  if (compression >= 0) {
    _d->buffer = (unsigned char*) malloc(chunk);
    _d->reader = _d->buffer;
  } else {
    _d->reader = NULL;
  }

  // Create openssl resources
  _d->ctx = new EVP_MD_CTX;
  if (_d->ctx != NULL) {
    EVP_DigestInit(_d->ctx, EVP_md5());
  }

  // Create zlib resources
  if ((compression > 0) && (_d->reader != NULL)) {
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

  // Write last few bytes in case of compressed write
  if (isWriteable()) {
    write(NULL, 0);
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
  }

  int rc = std::close(_d->fd);
  _d->fd = -1;

  // Destroy buffer if any
  if (_d->reader != NULL) {
    free(_d->buffer);
    _d->buffer = NULL;
  }

  // Update metadata
  stat();
  return rc;
}

long long Stream::progress() const {
  return _d->progress;
}

void Stream::setProgressCallback(progress_f progress) {
  _d->progress_callback = progress;
}

ssize_t Stream::read(void* buffer, size_t count) {
  if (! isOpen()) {
    errno = EBADF;
    return -1;
  }

  if (isWriteable()) {
    errno = EPERM;
    return -1;
  }

  if (count > chunk) count = chunk;
  if (count == 0) return 0;

  // Read new data
  if (_d->length == 0) {
    if (_d->reader == NULL) {
      // No buffer: just read
      _d->buffer = (unsigned char*) buffer;
      _d->length = std::read(_d->fd, _d->buffer, count);
    } else {
      // Fill in buffer
      _d->length = std::read(_d->fd, _d->buffer, chunk);
      _d->reader = _d->buffer;
    }

    // Check result
    if (_d->length < 0) {
      // errno set by read
      return -1;
    }

    // Update progress indicator (size read)
    if (_d->length > 0) {
      long long previous = _d->progress;
      _d->progress += _d->length;
      if (_d->progress_callback != NULL) {
        (*_d->progress_callback)(previous, _d->progress, _size);
      }
    }

    // Fill decompression input buffer with chunk or just return chunk
    if (_d->strm != NULL) {
      _d->strm->avail_in = _d->length;
      _d->strm->next_in  = _d->buffer;
    }
  }
  if (_d->strm != NULL) {
    // Continue decompression of previous data
    _d->strm->avail_out = count;
    _d->strm->next_out  = (unsigned char*) buffer;
    switch (inflate(_d->strm, Z_NO_FLUSH)) {
      case Z_NEED_DICT:
      case Z_DATA_ERROR:
      case Z_MEM_ERROR:
        return -2;
    }
    _d->length = (_d->strm->avail_out == 0);
    count -= _d->strm->avail_out;
  } else {
    if ((unsigned) _d->length < count) {
      count = _d->length;
    }
    if (_d->reader == NULL) {
      _d->length = 0;
    } else {
      memcpy(buffer, _d->reader, count);
      _d->length -= count;
      _d->reader += count;
    }
  }

  // Update checksum
  if (_d->ctx != NULL) {
    EVP_DigestUpdate(_d->ctx, buffer, count);
  }

  _d->size += count;
  return count;
}

ssize_t Stream::write(const void* buffer, size_t count) {
  static bool finish = true;

  if (! isOpen()) {
    errno = EBADF;
    return -1;
  }

  if (! isWriteable()) {
    errno = EPERM;
    return -1;
  }

  if (count > chunk) count = chunk;

  if ((count == 0) && (buffer == NULL)) {
    if (finish) {
      // Finished last time
      return 0;
    }
    finish = true;
  } else {
    finish = false;
  }

  _d->size += count;

  if (_d->strm == NULL) {
    // Checksum computation
    if (_d->ctx != NULL) {
      EVP_DigestUpdate(_d->ctx, buffer, count);
    }

    size_t blength  = count;
    bool   write_it = finish;

    if (_d->reader == NULL) {
      // No buffer: just write
      write_it = true;
      _d->length = count;
      _d->buffer = (unsigned char*) buffer;
    } else {
      // Fill in buffer
      write_it = finish;
      if (count > chunk - _d->length) {
        blength = chunk - _d->length;
        write_it = true;
      }
      memcpy(_d->reader, buffer, blength);
      _d->length += blength;
      _d->reader += blength;
    }

    // Write data
    if (write_it) {
      ssize_t length = _d->length;
      ssize_t wlength;
      do {
        wlength = std::write(_d->fd, _d->buffer, length);
        if (wlength < 0) {
          // errno set by write
          return -1;
        }

        // Update progress indicator (size written)
        if (wlength > 0) {
          long long previous = _d->progress;
          _d->progress += wlength;
          if (_d->progress_callback != NULL) {
            (*_d->progress_callback)(previous, _d->progress, _size);
          }
        }

        length -= wlength;
      } while ((length != 0) && (wlength != 0));
    }

    // Refill buffer
    if (blength < count) {
      char* cbuffer = (char*) buffer;
      memcpy(_d->buffer, &cbuffer[blength], count - blength);
      _d->length = count - blength;
      _d->reader = _d->buffer + _d->length;
    }
  } else {
    // Compress data
    _d->strm->avail_in = count;
    _d->strm->next_in  = (unsigned char*) buffer;
    count = 0;

    ssize_t length;
    do {
      _d->strm->avail_out = chunk;
      _d->strm->next_out  = _d->buffer;
      deflate(_d->strm, finish ? Z_FINISH : Z_NO_FLUSH);
      length = chunk - _d->strm->avail_out;
      count += length;

      // Checksum computation (on compressed data)
      if (_d->ctx != NULL) {
        EVP_DigestUpdate(_d->ctx, _d->buffer, length);
      }

      ssize_t wlength;
      do {
        wlength = std::write(_d->fd, _d->buffer, length);
        if (wlength < 0) {
          // errno set by write
          return -1;
        }
        length -= wlength;
      } while ((length != 0) && (wlength != 0));
    } while (_d->strm->avail_out == 0);
  }

  _size += count;
  return count;
}

ssize_t Stream::getLine(
    string&         buffer,
    bool*           end_of_line_found) {
  char reader[2];

  // Initialize return values
  buffer = "";
  if (end_of_line_found != NULL) {
    *end_of_line_found = false;
  }

  // Find end of line or end of file
  reader[1] = '\0';
  do {
    // Read one character at a time
    ssize_t size = read(reader, 1);
    // Error
    if (size < 0) {
      // errno set by read
      return -1;
    }
    // End of file
    if (size == 0) {
      break;
    }
    // End of line
    if (*reader == '\n') {
      if (end_of_line_found != NULL) {
        *end_of_line_found = true;
      }
      break;
    }
    // Add to buffer
    buffer += reader;
  } while (true);

  return buffer.length();
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
  unsigned char buffer[Stream::chunk];
  long long     read_size = 0;
  ssize_t       size;
  do {
    size = read(buffer, Stream::chunk);
    if (size < 0) {
      break;
    }
    read_size += size;
    if ((_d->cancel_callback != NULL) && ((*_d->cancel_callback)(1))) {
      errno = ECANCELED;
      return -1;
    }
  } while (size != 0);
  if (read_size != _d->size) {
    errno = EAGAIN;
    return -1;
  }
  return 0;
}

int Stream::copy(Stream& source) {
  if ((! isOpen()) || (! source.isOpen())) {
    errno = EBADF;
    return -1;
  }
  unsigned char buffer[Stream::chunk];
  long long read_size  = 0;
  long long write_size = 0;
  bool      eof        = false;
  ssize_t   size;

  do {
    size = source.read(buffer, Stream::chunk);
    if (size < 0) {
      break;
    }
    eof = (size == 0);
    read_size += size;
    size = write(buffer, size);
    if (size < 0) {
      break;
    }
    write_size += size;
    if ((_d->cancel_callback != NULL) && ((*_d->cancel_callback)(2))) {
      errno = ECANCELED;
      return -1;
    }
  } while (! eof);
  if (size < 0) {
    // errno is to be used
    return -1;
  }
  if (read_size != source._d->size) {
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

int Stream::getParams(
    vector<string>& params,
    char            flags,
    const char*     delims,
    const char*     quotes,
    const char*     comments) {
  string buffer;
  bool   eol;

  params.clear();
  int rc = getLine(buffer, &eol);
  if (rc < 0) {
    return -1;
  }
  if (! eol) {
    if (rc == 0) {
      return 0;
    }
    if ((flags & flags_need_lf) != 0) {
      return -1;
    }
  }
  if ((flags & flags_accept_cr_lf) && (buffer[buffer.size() - 1] == '\r')) {
    buffer.erase(buffer.size() - 1);
  }
  rc = extractParams(buffer, params, flags, 0, delims, quotes, comments);
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
    const string&   line,
    vector<string>& params,
    char            flags,
    unsigned int    max_params,
    const char*     delims,
    const char*     quotes,
    const char*     comments) {
  const char* read   = line.c_str();
  char* param        = (char*) malloc(line.size() + 1);
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
