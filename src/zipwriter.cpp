/*
    Copyright (C) 2010 - 2011  Hervé Fache

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

#include <errno.h>
#include <zlib.h>

#include <report.h>
#include "zipwriter.h"

using namespace htoolbox;

enum {
  BUFFER_SIZE = 102400
};

struct ZipWriter::Private {
  z_stream       strm;
  int            level;
  unsigned char  buffer[BUFFER_SIZE];
  bool           finished;
  Private(int l) : level(l) {}
};

ZipWriter::ZipWriter(IReaderWriter* child, bool delete_child, int level) :
  IReaderWriter(child, delete_child), _d(new Private(level)) {}

ZipWriter::~ZipWriter() {
  delete _d;
}

int ZipWriter::open() {
  if (_child->open() < 0) {
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
    _child->close();
    hlog_alert("failed to initialise compression (level = %d)", _d->level);
    errno = EUNATCH;
    return -1;
  }
  _d->finished = false;
  return 0;
}

int ZipWriter::close() {
  if (! _d->finished) {
    if (put(NULL, 0) < 0) {
      return -1;
    }
  }
  int rc = 0;
  if (deflateEnd(&_d->strm) != Z_OK) {
    hlog_alert("failed to complete compression");
    errno = EUNATCH;
    rc = -1;
  }
  if (_child->close() < 0) {
    rc = -1;
  }
  return rc;
}

// Not implemented
ssize_t ZipWriter::get(void*, size_t) {
  hlog_alert("cannot read from compression module");
  errno = EPROTO;
  return -1;
}

ssize_t ZipWriter::put(const void* buffer, size_t size) {
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


    int zlib_rc = deflate(&_d->strm, _d->finished ? Z_FINISH : Z_NO_FLUSH);
    if (zlib_rc < 0) {
      hlog_alert("failed to compress, zlib error code is %d", zlib_rc);
      errno = EUNATCH;
      return -1;
    }
    ssize_t length = BUFFER_SIZE - _d->strm.avail_out;
    if (_child->put(_d->buffer, length) < 0) {
      return -1;
    }
  } while (_d->strm.avail_out == 0);
  return size;
}
