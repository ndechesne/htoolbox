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

#include "strings.h"
#include "files.h"
#include "hbackup.h"

using namespace hbackup;

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
  int rc = std::remove(_path);
  if (! rc) {
    _type = '?';
  }
  return rc;
}

int File::create() {
  errno = 0;
  if (_type == '?') {
    stat();
  }
  if (! isValid()) {
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
    Node *g = new Node(_path, dir_entry->d_name);
    g->stat();
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
  errno = 0;
  if (_type == '?') {
    stat();
  }
  if (! isValid()) {
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
  int               fd;         // file descriptor
  mode_t            mode;       // file open mode
  long long         size;       // uncompressed file data size, in bytes
  unsigned char*    buffer;     // buffer for read/write operations
  unsigned char*    reader;     // buffer read pointer
  ssize_t           length;     // buffer length
  EVP_MD_CTX*       ctx;        // openssl resources
  z_stream*         strm;       // zlib resources
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

Stream::Stream(const char *dir_path, const char* name) : File(dir_path, name) {
  _d = new Private;
  _d->fd = -1;
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
    errno = EACCES;
    return -1;
  }

  _d->size   = 0;
  _d->length = 0;
  _d->fd = std::open64(_path, _d->mode, 0666);
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
        cerr << "stream: deflate init failed" << endl;
        compression = 0;
      }
    } else {
      // De-compress
      if (inflateInit2(_d->strm, 32 + 15)) {
        cerr << "stream: inflate init failed" << endl;
        compression = 0;
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

ssize_t Stream::read(void* buffer, size_t count) {
  if (! isOpen()) {
    errno = EBADF;
    return -1;
  }

  if (isWriteable()) {
    errno = EINVAL;
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

    // Update checksum with chunk
    if (_d->ctx != NULL) {
      EVP_DigestUpdate(_d->ctx, _d->buffer, _d->length);
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
        cerr << "File::read: inflate failed" << endl;
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
  _d->size   += count;
  return count;
}

ssize_t Stream::write(const void* buffer, size_t count) {
  static bool finish = true;

  if (! isOpen()) {
    errno = EBADF;
    return -1;
  }

  if (! isWriteable()) {
    errno = EINVAL;
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

ssize_t Stream::getLine(string& buffer) {
  buffer = "";
  char    reader[2];
  size_t  read_size         = 0;

  // Find end of line or end of file
  reader[1] = '\0';
  do {
    // Read one character at a time
    ssize_t size = read(reader, 1);
    if (size < 0) {
      // errno set by read
      read_size = -1;
      return -1;
    }
    if (size == 0) {
      break;
    }
    buffer += reader;
    // End of line found?
    if (*reader == '\n') {
      break;
    }
  } while (true);

  return buffer.length();
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
  } while (size != 0);
  if (read_size != _size) {
    errno = EAGAIN;
    return -1;
  }
  return 0;
}

int Stream::copy(Stream& source, cancel_f cancel) {
  if ((! isOpen()) || (! source.isOpen())) {
    errno = EBADF;
    return -1;
  }
  unsigned char buffer[Stream::chunk];
  long long read_size  = 0;
  long long write_size = 0;
  bool      eof        = false;
  do {
    ssize_t size = source.read(buffer, Stream::chunk);
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
    if ((cancel != NULL) && ((*cancel)())) {
      errno = ECANCELED;
      return -1;
    }
  } while (! eof);
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
  }
  free(buffer1);
  free(buffer2);
  return rc;
}

// Public functions
int Stream::readConfigLine(const string& line, list<string>& params) {
  const char* read  = line.c_str();
  const char* end   = &read[line.size()];
  char* value       = NULL;
  char* write       = NULL;
  bool increment;
  bool skipblanks   = true;
  bool checkcomment = false;
  bool escaped      = false;
  bool unquoted     = false;
  bool singlequoted = false;
  bool doublequoted = false;
  bool valueend     = false;
  bool endedwell    = true;

  while (read <= end) {
    increment = true;
    // End of line
    if (read == end) {
      if (unquoted || singlequoted || doublequoted) {
        // Stop decoding
        valueend = true;
        // Missing closing single- or double-quote
        if (singlequoted || doublequoted) {
          endedwell = false;
        }
      }
    } else
    // Skip blanks until no more, then change mode
    if (skipblanks) {
      if ((*read != ' ') && (*read != '\t')) {
        skipblanks = false;
        checkcomment = true;
        // Do not increment!
        increment = false;
      }
    } else if (checkcomment) {
      if (*read == '#') {
        // Nothing more to do
        break;
      } else {
        checkcomment = false;
        // Do not increment!
        increment = false;
      }
    } else { // neither a blank nor a comment
      if (singlequoted || doublequoted) {
        // Decoding quoted string
        if ((singlequoted && (*read == '\''))
         || (doublequoted && (*read == '"'))) {
          if (escaped) {
            *write++ = *read;
            escaped  = false;
          } else {
            // Found match, stop decoding
            singlequoted = doublequoted = false;
            valueend = true;
          }
        } else if (*read == '\\') {
          escaped = true;
        } else {
          if (escaped) {
            *write++ = '\\';
            escaped  = false;
          }
          *write++ = *read;
        }
      } else if (unquoted) {
        // Decoding unquoted string
        if ((*read == ' ') || (*read == '\t')) {
          // Found blank, stop decoding
          unquoted = false;
          valueend = true;
          // Do not increment!
          increment = false;
        } else {
          *write++ = *read;
        }
      } else {
        // Start decoding new string
        write = value = (char*) malloc(end - read + 1);
        if (*read == '\'') {
          singlequoted = true;
        } else
        if (*read == '"') {
          doublequoted = true;
        } else {
          unquoted = true;
          // Do not increment!
          increment = false;
        }
      }
    }
    if (valueend) {
      *write++ = '\0';
      params.push_back(string(value));
      free(value);
      skipblanks = true;
      valueend = false;
    }
    if (increment) {
      read++;
    }
  }

  if (! endedwell) {
    return 1;
  }
  return 0;
}
