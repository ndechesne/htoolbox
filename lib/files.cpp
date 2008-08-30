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
#include <semaphore.h>
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

#include "buffer.h"
#include "line.h"
#include "files.h"

using namespace hbackup;

const char* Path::operator=(const char* path) {
  if (_path != NULL) {
    free(_path);
  }
  _path = strdup(path);
  _length = strlen(_path);
  return _path;
}

const Path& Path::operator=(const Path& path) {
  if (_path != NULL) {
    free(_path);
  }
  _path = strdup(path._path);
  _length = path._length;
  return *this;
}

Path::Path(const char* dir, const char* name) {
  if (name[0] == '\0') {
    _path = strdup(dir);
  } else {
    asprintf(&_path, "%s/%s", dir, name);
  }
  _length = strlen(_path);
}

Path::Path(const Path& path, const char* name) {
  asprintf(&_path, "%s/%s", path._path, name);
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
  char* dir    = static_cast<char*>(malloc(length + 1));
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
    &&  (_path[0] >= 'a') && (_path[0] <= 'z')) {
      _path[0] = static_cast<char>(_path[0] - 0x20);
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

int Path::compare(const char* s1, const char* s2, ssize_t length) {
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

static int direntFilter(const struct dirent* a) {
  if (a->d_name[0] != '.') return 1;
  if (a->d_name[1] == '\0') return 0;
  if (a->d_name[1] != '.') return 1;
  if (a->d_name[2] == '\0') return 0;
  return 1;
}

static int direntCompare(const void* a, const void* b) {
  const struct dirent* l = *static_cast<const struct dirent* const *>(a);
  const struct dirent* r = *static_cast<const struct dirent* const *>(b);
  return Path::compare(l->d_name, r->d_name);
}

int Directory::createList() {
  if (_nodes != NULL) {
    errno = EBUSY;
    return -1;
  }
  _nodes = new list<Node*>;
  struct dirent** direntList;
  int size = scandir(_path, &direntList, direntFilter, direntCompare);
  if (size < 0) {
    deleteList();
    return -1;
  }
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
  int               fd;                 // file descriptor
  mode_t            mode;               // file open mode
  long long         size;               // uncompressed file data size (bytes)
  long long         progress;           // transfered size, for progress
  Buffer            buffer_comp;        // Buffer for [de]compression
  BufferReader      reader_comp;        // Reader for buffer above
  Buffer            buffer_data;        // Buffer for data
  BufferReader      reader_data;        // Reader for buffer above
  EVP_MD_CTX*       ctx;                // openssl resources
  z_stream*         strm;               // zlib resources
  long long         original_size;      // file size to store into gzip header
  bool              finish;             // tell compression to finish off
  progress_f        progress_callback;  // function to report progress
  cancel_f          cancel_callback;    // function to check for cancellation
  Private() : reader_comp(buffer_comp), reader_data(buffer_data) {}
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
    bool            checksum,
    long long       original_size) {
  if (isOpen()) {
    errno = EBUSY;
    return -1;
  }

  _d->mode          = O_NOATIME | O_LARGEFILE;
  _d->original_size = original_size;

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
  _d->finish   = true;
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
          Z_DEFAULT_STRATEGY) != Z_OK) {
        errno = ENOMEM;
        return -2;
      }
    } else {
      // De-compress
      if (inflateInit2(_d->strm, 32 + 15) != Z_OK) {
        errno = ENOMEM;
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
    _checksum = static_cast<char*>(malloc(2 * length + 1));
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
  const char* reader = static_cast<const char*>(buffer);
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
    void*           buffer,
    size_t          asked,
    size_t*         given) {
  ssize_t size = 0;
  bool    eof  = false;
  // Get header
  gz_header     header;
  unsigned char field[12];
  if (_d->original_size == -1) {
    header.text       = 0;
    header.time       = 0;
    header.os         = 3;
    header.extra      = field;
    header.extra_max  = 12;
    header.name       = Z_NULL;
    header.comment    = Z_NULL;
    header.hcrc       = 0;
    if (inflateGetHeader(_d->strm, &header) != Z_OK) {
      _d->original_size = -2;
    }
  }
  // Fill in buffer
  do {
    if (_d->reader_comp.isEmpty()) {
      size = std::read(_d->fd, _d->buffer_comp.writer(),
        _d->buffer_comp.writeable());
      if (size < 0) {
        return -1;
      }
      if (size == 0) {
        eof = true;
      }
      _d->buffer_comp.written(size);
      _d->strm->avail_in = _d->reader_comp.readable();
      _d->strm->next_in  = reinterpret_cast<unsigned char*>(
        const_cast<char*>(_d->reader_comp.reader()));
    }
    _d->strm->avail_out = asked;
    _d->strm->next_out  = static_cast<unsigned char*>(buffer);
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
  } while ((*given == 0) && ! eof);

  // Get header
  if (_d->original_size == -1) {
    GzipExtraFieldSize extra(field);
    _d->original_size = extra;
  }

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
  } else if (_d->reader_data.isEmpty()) {
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
      return _d->reader_data.readable();
    }
    given = _d->reader_data.read(buffer, asked);
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

  const char* reader = static_cast<const char*>(buffer);
  const char* end    = reader + count;
  ssize_t wlength;
  do {
    wlength = std::write(_d->fd, reader, end - reader);
    if (wlength < 0) {
      // errno set by write
      return -1;
    }
    reader += wlength;
  } while ((reader != end) && (errno == 0));  // errno maybe set with wlength 0
  return (reader != end) ? -1 : static_cast<ssize_t>(count);
}

ssize_t Stream::write_compress(
    const void*     buffer,
    size_t          count,
    bool            finish) {
  _d->strm->avail_in = count;
  _d->strm->next_in  = static_cast<Bytef*>(const_cast<void*>(buffer));

  // Set header
  if (_d->original_size >= 0) {
    GzipExtraFieldSize extra(_d->original_size);
    gz_header header;
    header.text       = 0;
    header.time       = 0;
    header.os         = 3;
    header.extra      = extra;
    header.extra_len  = extra.length();
    header.name       = Z_NULL;
    header.comment    = Z_NULL;
    header.hcrc       = 0;
    deflateSetHeader(_d->strm, &header);
  }

  // Flush result to file (no real cacheing)
  do {
    // Buffer is considered empty to start with. and is always flushed
    _d->strm->avail_out = _d->buffer_comp.writeable();
    // Casting away the constness here!!!
    _d->strm->next_out  =
      reinterpret_cast<unsigned char*>(_d->buffer_comp.writer());
    deflate(_d->strm, finish ? Z_FINISH : Z_NO_FLUSH);
    ssize_t length = _d->buffer_comp.writeable() - _d->strm->avail_out;
    if (length != write_all(_d->reader_comp.reader(), length)) {
      return -1;
    }
  } while (_d->strm->avail_out == 0);
  _d->original_size = -1;
  return count;
}

ssize_t Stream::write(
    const void*     buffer,
    size_t          given) {
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
    if (_d->finish) {
      // Finished last time
      return 0;
    }
    _d->finish = true;
  } else {
    _d->finish = false;
  }

  bool direct_write = false;
  ssize_t size = 0;
  if (_d->buffer_data.exists()) {
    // If told to finish or buffer is going to overflow, flush it to file
    if (_d->finish || (given > _d->buffer_data.writeable())) {
      // One or two writes (if end of buffer + beginning of it)
      while (_d->reader_data.readable() > 0) {
        ssize_t length = _d->reader_data.readable();
        if (_d->strm == NULL) {
          if (length != write_all(_d->reader_data.reader(), length)) {
            return -1;
          }
        } else {
          if (length != write_compress(_d->reader_data.reader(), length)) {
            return -1;
          }
        }
        _d->reader_data.readn(length);
        size += length;
      }
      // Buffer is now flushed, restore full writeable capacity
      _d->buffer_data.empty();
    }

    // If told to finish or more data than buffer can handle, just write
    if (_d->finish || (given > _d->buffer_data.writeable())) {
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
      if (length != write_compress(buffer, length, _d->finish)) {
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
    if (_d->reader_data.isEmpty()) {
      size = read(NULL, 0);
      if (size < 0) {
        return -1;
      }
    }
    // Make sure we have a buffer
    if (*buffer == NULL) {
      *buffer_capacity = 100;
      *buffer = static_cast<char*>(malloc(*buffer_capacity));
      writer = *buffer;
    }
    if (size == 0) {
      break;
    }
    const char* reader = _d->reader_data.reader();
    size_t      length = _d->reader_data.readable();
    while (length > 0) {
      length--;
      // Check for space
      if (count >= *buffer_capacity) {
        *buffer_capacity += 100;
        *buffer = static_cast<char*>(realloc(*buffer, *buffer_capacity));
        writer = &(*buffer)[count];
      }

      if (*reader == '\n') {
        found = true;
        break;
      }
      *writer++ = *reader++;
      count++;
    }
    _d->reader_data.readn(_d->reader_data.readable() - length);
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
  _d->size = 0;
  bool      eof  = false;

  do {
    ssize_t length = read(NULL, 0);
    _d->buffer_data.empty();
    if (length < 0) {
      return -1;
    }
    eof = (length == 0);
    _d->size += length;
    if ((_d->cancel_callback != NULL) && ((*_d->cancel_callback)(1))) {
      errno = ECANCELED;
      return -1;
    }
  } while (! eof);
  return 0;
}

struct CopyData {
  // Global
  Stream*       stream;
  Buffer&       buffer;
  sem_t&        read_sem;
  bool&         eof;
  bool&         failed;
  // Local
  BufferReader  reader;
  sem_t         write_sem;
  CopyData(Stream* d, Buffer& b, sem_t& s, bool& eof, bool& failed) :
      stream(d), buffer(b), read_sem(s), eof(eof), failed(failed),
      // No auto-unregistration: segentation fault may occur
      reader(buffer, false) {
    sem_init(&write_sem, 0, 0);
  }
  ~CopyData() {
    sem_destroy(&write_sem);
  }
};

struct ReadData {
  sem_t         read_sem;
  ReadData() {
    sem_init(&read_sem, 0, 0);
  }
  ~ReadData() {
    sem_destroy(&read_sem);
  }
};

static void* write_task(void* data) {
  CopyData& cd = *static_cast<CopyData*>(data);
  do {
    // Reset semaphore before checking for emptiness
    if (! cd.reader.isEmpty()) {
      // Write as much as possible
      ssize_t l = cd.stream->write(cd.reader.reader(), cd.reader.readable());
      if (l < 0) {
        cd.failed = true;
      } else {
        cd.reader.readn(l);
      }
      // Allow read task to run
      sem_post(&cd.read_sem);
    } else {
      // Now either a read occurred and we won't lock, or we shall wait
      if (sem_wait(&cd.write_sem)) {
        cd.failed = true;
      }
    }
  } while (! cd.failed && (! cd.eof || ! cd.reader.isEmpty()));
  return NULL;
}

int Stream::copy(Stream* dest1, Stream* dest2) {
  errno = 0;
  if (! isOpen() || ! dest1->isOpen()
  || ((dest2 != NULL) && ! dest2->isOpen())) {
    errno = EBADF;
    return -1;
  }
  Buffer    buffer(1 << 20);
  ReadData  rd;
  bool      eof    = false;
  bool      failed = false;
  CopyData  *cd1 = NULL;
  CopyData  *cd2 = NULL;
  long long size = 0;
  pthread_t child1;
  pthread_t child2;
  cd1 = new CopyData(dest1, buffer, rd.read_sem, eof, failed);
  int rc = pthread_create(&child1, NULL, write_task, cd1);
  if (rc != 0) {
    delete cd1;
    errno = rc;
    return -1;
  }
  if (dest2 != NULL) {
    cd2 = new CopyData(dest2, buffer, rd.read_sem, eof, failed);
    rc = pthread_create(&child2, NULL, write_task, cd2);
    if (rc != 0) {
      delete cd2;
      errno = rc;
      // Tell other task to die and wait for it
      failed = true;
      pthread_join(child1, NULL);
      delete cd1;
      return -1;
    }
  }

  // Read loop
  do {
    // Reset semaphore before checking for fullness
    if (! buffer.isFull()) {
      // Fill up only up to half of capacity, for simultenous read and write
      ssize_t length = buffer.capacity() >> 1;
      if (static_cast<size_t>(length) > buffer.writeable()) {
        length = buffer.writeable();
      }
      length = read(buffer.writer(), length);
      if (length < 0) {
        failed = true;
      } else {
        // Update writer before signaling eof
        buffer.written(length);
        // Allow write task to run
        if (length == 0) {
          eof = true;
        }
        size += length;
      }
      sem_post(&cd1->write_sem);
      if (dest2 != NULL) {
        sem_post(&cd2->write_sem);
      }
    } else {
      // Now either a write occurred and we won't lock, or we shall wait
      if (sem_wait(&rd.read_sem)) {
        failed = true;
      }
    }
    if ((_d->cancel_callback != NULL) && ((*_d->cancel_callback)(2))) {
      errno = ECANCELED;
      failed = true;
      sem_post(&cd1->write_sem);
      if (dest2 != NULL) {
        sem_post(&cd2->write_sem);
      }
      break;
    }
  } while (! failed && ! eof);

  // Wait for write tasks to terminate, check status
  rc = pthread_join(child1, NULL);
  delete cd1;
  if (rc != 0) {
    errno = rc;
  } else
  // Check that data sizes match
  if ((errno == 0) && (dataSize() != dest1->dataSize())) {
    errno = EAGAIN;
  }
  if (dest2 != NULL) {
    rc = pthread_join(child2, NULL);
    delete cd2;
    if (rc != 0) {
      errno = rc;
    } else
    // Check that data sizes match
    if ((errno == 0) && (dataSize() != dest2->dataSize())) {
      errno = EAGAIN;
    }
  }
  // Check that sizes match
  if (! failed && (errno == 0) && (size != dataSize())) {
    errno = EAGAIN;
  }
  return (errno != 0) ? -1 : 0;
}

int Stream::compare(Stream& source, long long length) {
  if ((! isOpen()) || (! source.isOpen())) {
    errno = EBADF;
    return -1;
  }
  int             buf_size = 1024;
  unsigned char*  buffer1  = static_cast<unsigned char*>(malloc(buf_size));
  unsigned char*  buffer2  = static_cast<unsigned char*>(malloc(buf_size));
  int             rc = -2;

  while (rc == -2) {
    bool eof;
    // Make we stop when needed
    if ((length >= 0) && (length < buf_size)) {
      buf_size = static_cast<int>(length);
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

long long Stream::originalSize() const {
  return _d->original_size;
}

long long Stream::getOriginalSize() const {
  if (isOpen()) {
    errno = EBUSY;
    return -1;
  }
  int fd = std::open64(_path, O_RDONLY, 0666);
  if (fd < 0) {
    // errno set by open
    return -1;
  }
  char buffer[14];
  _d->original_size = -1;
  // Check header
  if (std::read(fd, buffer, 10) != 10) {
    // errno set by read
  } else
  if (strncmp(buffer, "\x1f\x8b", 2) != 0) {
    errno = EILSEQ;
  } else
  // Check extra field flag
  if ((buffer[3] & 0x4) == 0) {
    errno = ENOSYS;
  } else
  // Check extra field
  if (std::read(fd, buffer, 14) != 14) {
    // errno set by read
  } else
  // Assume only OUR extra field is present
  {
    GzipExtraFieldSize extra(reinterpret_cast<unsigned char*>(&buffer[2]));
    _d->original_size = extra;
  }
  std::close(fd);
  return _d->original_size;
}

int Stream::setOriginalSize(long long size) const {
  if (isOpen()) {
    errno = EBUSY;
    return -1;
  }
  int fd = std::open64(_path, O_RDWR, 0666);
  if (fd < 0) {
    // errno set by open
    return -1;
  }
  char buffer[14];
  _d->original_size = -1;
  // Check header
  if (std::read(fd, buffer, 10) != 10) {
    // errno set by read
  } else
  if (strncmp(buffer, "\x1f\x8b", 2) != 0) {
    errno = EILSEQ;
  } else
  // Check extra field flag
  if ((buffer[3] & 0x4) == 0) {
    errno = ENOSYS;
  } else
  // Skip extra field size
  if (std::read(fd, buffer, 2) != 2) {
    // errno set by read
  } else
  // Assume OUR extra field is present HERE
  {
    GzipExtraFieldSize extra(size);
    if (std::write(fd, extra, 12) == 12) {
      _d->original_size = size;
    }
  }
  std::close(fd);
  return (_d->original_size < 0) ? -1 : 0;
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
  LineBuffer line;
  bool       eol;

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
  char* param        = static_cast<char*>(malloc(strlen(line) + 1));
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
