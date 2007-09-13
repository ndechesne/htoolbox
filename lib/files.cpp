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

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

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

void Node::metadata(const char* path) {
  struct stat64 metadata;
  if (lstat64(path, &metadata)) {
    // errno set by lstat
    _type = '?';
  } else {
    if (S_ISREG(metadata.st_mode))       _type =  'f';
    else if (S_ISDIR(metadata.st_mode))  _type =  'd';
    else if (S_ISCHR(metadata.st_mode))  _type =  'c';
    else if (S_ISBLK(metadata.st_mode))  _type =  'b';
    else if (S_ISFIFO(metadata.st_mode)) _type =  'p';
    else if (S_ISLNK(metadata.st_mode))  _type =  'l';
    else if (S_ISSOCK(metadata.st_mode)) _type =  's';
    else                                 _type =  '?';
    // Fill in file information
    _size  = metadata.st_size;
    _mtime = metadata.st_mtime;
    _uid   = metadata.st_uid;
    _gid   = metadata.st_gid;
    _mode  = metadata.st_mode & ~S_IFMT;
  }
}

Node::Node(const char* dir_path, const char* name) {
  _parsed = false;
  char* full_path = path(dir_path, name);
  asprintf(&_name, "%s", basename(full_path));
  if (_name[0] == '\0') {
    _type = '?';
  } else {
    errno = 0;
    metadata(full_path);
  }
  free(full_path);
}

bool Node::operator!=(const Node& right) const {
  return (_type != right._type)   || (_mtime != right._mtime)
      || (_size != right._size)   || (_uid != right._uid)
      || (_gid != right._gid)     || (_mode != right._mode)
      || (strcmp(_name, right._name) != 0);
}

int Node::pathCompare(const char* s1, const char* s2, size_t length) {
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

int File::create(const char* dir_path) {
  errno = 0;
  char* full_path = path(dir_path, _name);
  if (_name[0] == '\0') {
    metadata(full_path);
  }
  if (! isValid()) {
    int readfile = open(full_path, O_WRONLY | O_CREAT, 0666);
    if (readfile < 0) {
      return -1;
    }
    close(readfile);
    metadata(full_path);
  }
  free(full_path);
  return 0;
}

int Directory::createList(const char* dir_path, bool is_path) {
  char* full_path;
  if (is_path) {
    full_path = path(dir_path, "");
  } else {
    full_path = path(dir_path, _name);
  }
  DIR* directory = opendir(full_path);
  if (directory == NULL) return -1;

  struct dirent *dir_entry;
  while (((dir_entry = readdir(directory)) != NULL) && ! terminating()) {
    // Ignore . and ..
    if (!strcmp(dir_entry->d_name, ".") || !strcmp(dir_entry->d_name, "..")) {
      continue;
    }
    Node *g = new Node(full_path, dir_entry->d_name);
    list<Node*>::iterator i = _nodes.begin();
    while ((i != _nodes.end()) && (*(*i) < *g)) {
      i++;
    }
    _nodes.insert(i, g);
  }
  free(full_path);

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

int Directory::create(const char* dir_path) {
  errno = 0;
  char* full_path = path(dir_path, _name);
  if (_name[0] == '\0') {
    metadata(full_path);
  }
  if (! isValid()) {
    if (mkdir(full_path, 0777)) {
      return -1;
    }
    metadata(full_path);
  }
  free(full_path);
  return 0;
}

bool Link::operator!=(const Link& right) const {
  if (*((Node*)this) != (Node)right) {
    return true;
  }
  return strcmp(_link, right._link) != 0;
}

void Stream::md5sum(char* out, const unsigned char* in, int bytes) {
  char* hex   = "0123456789abcdef";

  while (bytes-- != 0) {
    *out++ = hex[*in >> 4];
    *out++ = hex[*in & 0xf];
    in++;
  }
  *out = '\0';
}

Stream::~Stream() {
  if (isOpen()) {
    close();
  }
  free(_path);
  _path = NULL;
}

int Stream::open(const char* req_mode, int compression) {
  _fmode = O_NOATIME | O_LARGEFILE;

  switch (req_mode[0]) {
  case 'w':
    _fmode = O_WRONLY | O_CREAT | O_TRUNC;
    _size = 0;
    break;
  case 'r':
    _fmode = O_RDONLY;
    break;
  default:
    errno = EACCES;
    return -1;
  }

  _dsize   = 0;
  _flength = 0;
  _fd = std::open64(_path, _fmode, 0666);
  if (! isOpen()) {
    // errno set by open
    return -1;
  }

  // Create buffer if not told not to
  if (compression >= 0) {
    _fbuffer = (unsigned char*) malloc(chunk);
    _freader = _fbuffer;
  } else {
    _freader = NULL;
  }

  // Create openssl resources
  _ctx = new EVP_MD_CTX;
  if (_ctx != NULL) {
    EVP_DigestInit(_ctx, EVP_md5());
  }

  // Create zlib resources
  if ((compression > 0) && (_freader != NULL)) {
    _strm           = new z_stream;
    _strm->zalloc   = Z_NULL;
    _strm->zfree    = Z_NULL;
    _strm->opaque   = Z_NULL;
    _strm->avail_in = 0;
    _strm->next_in  = Z_NULL;
    if (isWriteable()) {
      // Compress
      if (deflateInit2(_strm, compression, Z_DEFLATED, 16 + 15, 9,
          Z_DEFAULT_STRATEGY)) {
        cerr << "stream: deflate init failed" << endl;
        compression = 0;
      }
    } else {
      // De-compress
      if (inflateInit2(_strm, 32 + 15)) {
        cerr << "stream: inflate init failed" << endl;
        compression = 0;
      }
    }
  } else {
    _strm = NULL;
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
  if (_ctx != NULL) {
    unsigned char checksum[36];
    size_t        length;

    EVP_DigestFinal(_ctx, checksum, &length);
    _checksum = (char*) malloc(2 * length + 1);
    md5sum(_checksum, checksum, length);
    delete _ctx;
    _ctx = NULL;
  }

  // Destroy zlib resources
  if (_strm != NULL) {
    if (isWriteable()) {
      deflateEnd(_strm);
    } else {
      inflateEnd(_strm);
    }
    delete _strm;
    _strm = NULL;
  }

  int rc = std::close(_fd);
  _fd = -1;

  // Destroy buffer if any
  if (_freader != NULL) {
    free(_fbuffer);
    _fbuffer = NULL;
  }

  // Update metadata
  metadata(_path);
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
  if (_flength == 0) {
    if (_freader == NULL) {
      // No buffer: just read
      _fbuffer = (unsigned char*) buffer;
      _flength = std::read(_fd, _fbuffer, count);
    } else {
      // Fill in buffer
      _flength = std::read(_fd, _fbuffer, chunk);
      _freader = _fbuffer;
    }

    // Check result
    if (_flength < 0) {
      // errno set by read
      return -1;
    }

    // Update checksum with chunk
    if (_ctx != NULL) {
      EVP_DigestUpdate(_ctx, _fbuffer, _flength);
    }

    // Fill decompression input buffer with chunk or just return chunk
    if (_strm != NULL) {
      _strm->avail_in = _flength;
      _strm->next_in  = _fbuffer;
    }
  }
  if (_strm != NULL) {
    // Continue decompression of previous data
    _strm->avail_out = count;
    _strm->next_out  = (unsigned char*) buffer;
    switch (inflate(_strm, Z_NO_FLUSH)) {
      case Z_NEED_DICT:
      case Z_DATA_ERROR:
      case Z_MEM_ERROR:
        fprintf(stderr, "File::read: inflate failed\n");
    }
    _flength = (_strm->avail_out == 0);
    count -= _strm->avail_out;
  } else {
    if ((unsigned) _flength < count) {
      count = _flength;
    }
    if (_freader == NULL) {
      _flength = 0;
    } else {
      memcpy(buffer, _freader, count);
      _flength -= count;
      _freader += count;
    }
  }
  _dsize   += count;
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

  if (count == 0) {
    if (finish) {
      // Finished last time
      return 0;
    }
    finish = true;
  } else {
    finish = false;
  }

  _dsize += count;

  if (_strm == NULL) {
    // Checksum computation
    if (_ctx != NULL) {
      EVP_DigestUpdate(_ctx, buffer, count);
    }

    size_t blength  = count;
    bool   write_it = finish;

    if (_freader == NULL) {
      // No buffer: just write
      write_it = true;
      _flength = count;
      _fbuffer = (unsigned char*) buffer;
    } else {
      // Fill in buffer
      write_it = finish;
      if (count > chunk - _flength) {
        blength = chunk - _flength;
        write_it = true;
      }
      memcpy(_freader, buffer, blength);
      _flength += blength;
      _freader += blength;
    }

    // Write data
    if (write_it) {
      ssize_t length = _flength;
      ssize_t wlength;
      do {
        wlength = std::write(_fd, _fbuffer, length);
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
      memcpy(_fbuffer, &cbuffer[blength], count - blength);
      _flength = count - blength;
      _freader = _fbuffer + _flength;
    }
  } else {
    // Compress data
    _strm->avail_in = count;
    _strm->next_in  = (unsigned char*) buffer;
    count = 0;

    ssize_t length;
    do {
      _strm->avail_out = chunk;
      _strm->next_out  = _fbuffer;
      deflate(_strm, finish ? Z_FINISH : Z_NO_FLUSH);
      length = chunk - _strm->avail_out;
      count += length;

      // Checksum computation (on compressed data)
      if (_ctx != NULL) {
        EVP_DigestUpdate(_ctx, _fbuffer, length);
      }

      ssize_t wlength;
      do {
        wlength = std::write(_fd, _fbuffer, length);
        if (wlength < 0) {
          // errno set by write
          return -1;
        }
        length -= wlength;
      } while ((length != 0) && (wlength != 0));
    } while (_strm->avail_out == 0);
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
  if (open("r", 0)) {
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
  if (close()) {
    return -1;
  }
  if (read_size != _size) {
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
  } while (! eof);
  if (read_size != source.dsize()) {
    errno = EAGAIN;
    return -1;
  }
  return 0;
}

// Public functions
int Stream::decodeLine(const string& line, list<string>& params) {
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
        write = value = new char[end - read + 1];
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
      delete value;
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
