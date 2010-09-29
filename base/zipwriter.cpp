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

#include <zipwriter.h>

using namespace hbackup;

enum {
  BUFFER_SIZE = 102400
};

struct ZipWriter::Private {
  IReaderWriter* child;
  bool           delete_child;
  z_stream       strm;
  int            level;
  unsigned char  buffer[BUFFER_SIZE];
  bool           finished;
  Private(IReaderWriter* c, bool d, int l) :
    child(c), delete_child(d), level(l) {}
};

ZipWriter::ZipWriter(IReaderWriter* child, bool delete_child, int level) :
  _d(new Private(child, delete_child, level)) {}

ZipWriter::~ZipWriter() {
  if (_d->delete_child) {
    delete _d->child;
  }
  delete _d;
}

int ZipWriter::open() {
  if (_d->child->open() < 0) {
    return -1;
  }
  _d->strm.zalloc   = Z_NULL;
  _d->strm.zfree    = Z_NULL;
  _d->strm.opaque   = Z_NULL;
  _d->strm.avail_in = 0;
  _d->strm.next_in  = Z_NULL;
  // De-compress
  if (deflateInit2(&_d->strm, _d->level, Z_DEFLATED, 16 + 15, 9,
                   Z_DEFAULT_STRATEGY) != Z_OK) {
    _d->child->close();
    errno = ENOMEM;
    return -1;
  }
  _d->finished = false;
  return 0;
}

int ZipWriter::close() {
  if (! _d->finished) {
    if (write(NULL, 0) < 0) {
      return -1;
    }
  }
  int rc = 0;
  if (deflateEnd(&_d->strm) != Z_OK) {
    rc = -1;
  }
  if (_d->child->close() < 0) {
    rc = -1;
  }
  return rc;
}

// Not implemented
ssize_t ZipWriter::read(void* buffer, size_t size) {
  (void) buffer;
  (void) size;
  return -1;
}

ssize_t ZipWriter::write(const void* buffer, size_t size) {
  if ( _d->finished) {
    return 0;
  }
  if (size == 0) {
    _d->finished = true;
  } else {
    _d->strm.avail_in = static_cast<uInt>(size);
    // Casting away the constness here!!!
    _d->strm.next_in  = static_cast<Bytef*>(const_cast<void*>(buffer));
  }
  do {
    _d->strm.avail_out = static_cast<uInt>(BUFFER_SIZE);
    _d->strm.next_out  = _d->buffer;
    switch (deflate(&_d->strm, _d->finished ? Z_FINISH : Z_NO_FLUSH) != Z_OK) {
      case Z_OK:
      case Z_STREAM_END:
        break;
      default:
        errno = EPROTO;
        return -1;
    }
    ssize_t length = BUFFER_SIZE - _d->strm.avail_out;
    if (_d->child->write(_d->buffer, length) < 0) {
      return -1;
    }
  } while (_d->strm.avail_out == 0);
  return size;
}
