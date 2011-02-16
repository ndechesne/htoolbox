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
#include "zipper.h"

using namespace htoolbox;

enum {
  BUFFER_SIZE = 102400
};

struct Zipper::Private {
  bool           zip;
  z_stream       strm;
  unsigned char  buffer[BUFFER_SIZE];
  bool           finished;
  int            level;
  Private(int l) : zip(l >= 0), level(l) {}
  int open() {
    strm.zalloc   = Z_NULL;
    strm.zfree    = Z_NULL;
    strm.opaque   = Z_NULL;
    strm.avail_in = 0;
    strm.next_in  = Z_NULL;
    int rc;
    if (zip) {
      rc = deflateInit2(&strm, level, Z_DEFLATED, 16+15, 9, Z_DEFAULT_STRATEGY);
    } else {
      rc = inflateInit2(&strm, 32 + 15);
    }
    if (rc != Z_OK) {
      hlog_alert("failed to initialise compression (level = %d)", level);
      errno = EUNATCH;
      return -1;
    }
    finished = false;
    return 0;
  }
  int close() {
    int rc;
    if (zip) {
      rc = deflateEnd(&strm);
    } else {
      rc = inflateEnd(&strm);
    }
    if (rc != Z_OK) {
      hlog_alert("failed to complete compression");
      errno = EUNATCH;
      return -1;
    }
    return 0;
  }
  bool canSubmit() const {
    return strm.avail_in == 0;
  }
  void submit(const void* buffer, size_t size) {
    if (finished) {
      return;
    }
    if (size != 0) {
      strm.avail_in = static_cast<uInt>(size);
      // Casting away the constness here!!!
      strm.next_in  = static_cast<Bytef*>(const_cast<void*>(buffer));
    } else
    if (zip) {
      finished = true;
    }
  }
  bool canUpdate() const {
    return strm.avail_out == 0;
  }
  ssize_t update(void* buffer, size_t size) {
    strm.avail_out = static_cast<uInt>(size);
    strm.next_out  = static_cast<Bytef*>(buffer);
    int rc;
    const char* mode;
    if (zip) {
      mode = "compress";
      rc = deflate(&strm, finished ? Z_FINISH : Z_NO_FLUSH);
    } else {
      mode = "decompress";
      rc = inflate(&strm, Z_NO_FLUSH);
    }
    if (rc < 0) {
      hlog_alert("failed to %s, zlib error code is %d", mode, rc);
      errno = EUCLEAN;
      return -1;
    }
    if (! zip && (rc == Z_STREAM_END)) {
      finished = true;
    }
    return size - strm.avail_out;
  }
};

Zipper::Zipper(IReaderWriter* child, bool delete_child, int level) :
  IReaderWriter(child, delete_child), _d(new Private(level)) {}

Zipper::~Zipper() {
  delete _d;
}

int Zipper::open() {
  if (_child->open() < 0) {
    return -1;
  }
  if (_d->open() < 0) {
    _child->close();
    return -1;
  }
  return 0;
}

int Zipper::close() {
  if (_d->zip && ! _d->finished) {
    if (put(NULL, 0) < 0) {
      return -1;
    }
  }
  int rc = _d->close();
  if (_child->close() < 0) {
    rc = -1;
  }
  return rc;
}

ssize_t Zipper::read(void* buffer, size_t size) {
  if (_d->canSubmit()) {
    ssize_t length = _child->read(_d->buffer, sizeof(_d->buffer));
    if (length < 0) {
      return -1;
    }
    _d->submit(_d->buffer, length);
  }
  return _d->update(buffer, size);
}

ssize_t Zipper::get(void* buffer, size_t size) {
  char* cbuffer = static_cast<char*>(buffer);
  size_t count = 0;
  while (count < size) {
    if (_d->canSubmit()) {
      ssize_t length = _child->get(_d->buffer, sizeof(_d->buffer));
      if (length < 0) {
        return -1;
      }
      _d->submit(_d->buffer, length);
    }
    ssize_t length = _d->update(&cbuffer[count], size - count);
    if (length < 0) {
      return -1;
    }
    count += length;
    if (_d->finished) {
      break;
    }
  }
  return count;
}

ssize_t Zipper::put(const void* buffer, size_t size) {
  _d->submit(buffer, size);
  do {
    ssize_t length = _d->update(_d->buffer, sizeof(_d->buffer));
    if (_child->put(_d->buffer, length) < 0) {
      return -1;
    }
  } while (_d->canUpdate());
  return size;
}
