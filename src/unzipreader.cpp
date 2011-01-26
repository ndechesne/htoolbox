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
#include "unzipreader.h"

using namespace htoolbox;

enum {
  BUFFER_SIZE = 102400
};

struct UnzipReader::Private {
  z_stream       strm;
  unsigned char  buffer[BUFFER_SIZE];
};

UnzipReader::UnzipReader(IReaderWriter* child, bool delete_child) :
  IReaderWriter(child, delete_child), _d(new Private) {}

UnzipReader::~UnzipReader() {
  delete _d;
}

int UnzipReader::open() {
  if (_child->open() < 0) {
    return -1;
  }
  _d->strm.zalloc   = Z_NULL;
  _d->strm.zfree    = Z_NULL;
  _d->strm.opaque   = Z_NULL;
  _d->strm.avail_in = 0;
  _d->strm.next_in  = Z_NULL;
  // De-compress
  if (inflateInit2(&_d->strm, 32 + 15) != Z_OK) {
    _child->close();
    hlog_alert("failed to initialise decompression");
    errno = EUNATCH;
    return -1;
  }
  return 0;
}

int UnzipReader::close() {
  int rc = 0;
  if (inflateEnd(&_d->strm) != Z_OK) {
    hlog_alert("failed to complete decompression");
    errno = EUNATCH;
    rc = -1;
  }
  if (_child->close() < 0) {
    rc = -1;
  }
  return rc;
}

ssize_t UnzipReader::read(void* buffer, size_t size) {
  unsigned char* cbuffer = static_cast<unsigned char*>(buffer);
  size_t count = 0;
  while (count < size) {
    ssize_t count_in;
    if (_d->strm.avail_in == 0) {
      count_in = _child->read(_d->buffer, BUFFER_SIZE);
      if (count_in < 0) {
        return -1;
      }
      if (count_in != 0) {
        _d->strm.avail_in = static_cast<uInt>(count_in);
        _d->strm.next_in  = _d->buffer;
      }
    }
    _d->strm.avail_out = static_cast<uInt>(size - count);
    _d->strm.next_out  = &cbuffer[count];
    int zlib_rc = inflate(&_d->strm, Z_NO_FLUSH);
    if (zlib_rc < 0) {
      hlog_alert("failed to decompress, zlib error code is %d", zlib_rc);
      errno = EUCLEAN;
      return -1;
    }
    count = size - _d->strm.avail_out;
    if (zlib_rc == Z_STREAM_END) {
      break;
    }
  }
  return count;
}

// Not implemented
ssize_t UnzipReader::write(const void*, size_t) {
  hlog_alert("cannot write from decompression module");
  errno = EPROTO;
  return -1;
}
