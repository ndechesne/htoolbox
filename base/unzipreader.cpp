/*
     Copyright (C) 2010  Herve Fache

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

#include <errno.h>

#include <zlib.h>

#include <unzipreader.h>

using namespace hbackup;

enum {
  BUFFER_SIZE = 102400
};

struct UnzipReader::Private {
  IReaderWriter* child;
  bool           delete_child;
  z_stream*      strm;
  unsigned char  buffer[BUFFER_SIZE];
  bool           buffer_empty;
  Private(IReaderWriter* c, bool d) : child(c), delete_child(d), strm(NULL) {}
};

UnzipReader::UnzipReader(IReaderWriter* child, bool delete_child) :
  _d(new Private(child, delete_child)) {}

UnzipReader::~UnzipReader() {
  if (_d->delete_child) {
    delete _d->child;
  }
  if (_d->strm != NULL) {
    close();
  }
  delete _d;
}

int UnzipReader::open() {
  if (_d->child->open() < 0) {
    return -1;
  }
  _d->strm           = new z_stream;
  _d->strm->zalloc   = Z_NULL;
  _d->strm->zfree    = Z_NULL;
  _d->strm->opaque   = Z_NULL;
  _d->strm->avail_in = 0;
  _d->strm->next_in  = Z_NULL;
  // De-compress
  if (inflateInit2(_d->strm, 32 + 15) != Z_OK) {
    close();
    errno = ENOMEM;
    return -1;
  }
  _d->buffer_empty = true;
  return 0;
}

int UnzipReader::close() {
  inflateEnd(_d->strm);
  delete _d->strm;
  _d->strm = NULL;
  return _d->child->close();
}

ssize_t UnzipReader::read(void* buffer, size_t size) {
  unsigned char* cbuffer = static_cast<unsigned char*>(buffer);
  size_t count = 0;
  while (count < size) {
      ssize_t count_in;
    if (_d->buffer_empty) {
      count_in = _d->child->read(_d->buffer, BUFFER_SIZE);
      if (count_in < 0) {
        return -1;
      }
      if (count_in != 0) {
        _d->strm->avail_in = static_cast<uInt>(count_in);
        _d->strm->next_in  = _d->buffer;
      }
    }
    _d->strm->avail_out = static_cast<uInt>(size - count);
    _d->strm->next_out  = &cbuffer[count];
    switch (inflate(_d->strm, Z_NO_FLUSH)) {
      case Z_NEED_DICT:
      case Z_DATA_ERROR:
      case Z_MEM_ERROR:
        errno = EPROTO;
          return -1;
    }
    count = size - _d->strm->avail_out;
    // Used up all input buffer data
    if (_d->strm->avail_out != 0) {
      _d->buffer_empty = true;
      if (count_in == 0) {
        break;
      }
    }
  }
  return count;
}

// Not implemented
ssize_t UnzipReader::write(const void* buffer, size_t size) {
  (void) buffer;
  (void) size;
  return -1;
}

long long UnzipReader::offset() const {
  return _d->child->offset();
}
