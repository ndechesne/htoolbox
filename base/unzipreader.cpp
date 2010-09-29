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

#include <zlib.h>

#include <hreport.h>
#include "unzipreader.h"

using namespace htools;

enum {
  BUFFER_SIZE = 102400
};

struct UnzipReader::Private {
  IReaderWriter* child;
  bool           delete_child;
  z_stream       strm;
  unsigned char  buffer[BUFFER_SIZE];
  bool           buffer_empty;
  Private(IReaderWriter* c, bool d) : child(c), delete_child(d) {}
};

UnzipReader::UnzipReader(IReaderWriter* child, bool delete_child) :
  _d(new Private(child, delete_child)) {}

UnzipReader::~UnzipReader() {
  if (_d->delete_child) {
    delete _d->child;
  }
  delete _d;
}

int UnzipReader::open() {
  if (_d->child->open() < 0) {
    return -1;
  }
  _d->strm.zalloc   = Z_NULL;
  _d->strm.zfree    = Z_NULL;
  _d->strm.opaque   = Z_NULL;
  _d->strm.avail_in = 0;
  _d->strm.next_in  = Z_NULL;
  // De-compress
  if (inflateInit2(&_d->strm, 32 + 15) != Z_OK) {
    _d->child->close();
    hlog_alert("failed to initialise decompression");
    return -1;
  }
  _d->buffer_empty = true;
  return 0;
}

int UnzipReader::close() {
  int rc = 0;
  if (inflateEnd(&_d->strm) != Z_OK) {
    hlog_alert("failed to complete decompression");
    rc = -1;
  }
  if (_d->child->close() < 0) {
    rc = -1;
  }
  return rc;
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
        _d->strm.avail_in = static_cast<uInt>(count_in);
        _d->strm.next_in  = _d->buffer;
      }
    }
    _d->strm.avail_out = static_cast<uInt>(size - count);
    _d->strm.next_out  = &cbuffer[count];
    switch (inflate(&_d->strm, Z_NO_FLUSH)) {
      case Z_OK:
      case Z_STREAM_END:
        break;
      default:
        hlog_alert("failed to decompress");
        return -1;
    }
    count = size - _d->strm.avail_out;
    // Used up all input buffer data
    if (_d->strm.avail_out != 0) {
      _d->buffer_empty = true;
      if (count_in == 0) {
        break;
      }
    }
  }
  return count;
}

// Not implemented
ssize_t UnzipReader::write(const void*, size_t) {
  hlog_alert("cannot write from decompression module");
  return -1;
}
