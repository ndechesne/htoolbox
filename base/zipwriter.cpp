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
  IWriter&      writer;
  z_stream*     strm;
  int           level;
  unsigned char buffer[BUFFER_SIZE];
  bool          finished;
  Private(IWriter& r, int l) : writer(r), strm(NULL), level(l) {}
};

ZipWriter::ZipWriter(IWriter& w, int level) : _d(new Private(w, level)) {}

ZipWriter::~ZipWriter() {
  if (_d->strm != NULL) {
    close();
  }
  delete _d;
}

int ZipWriter::open() {
  if (_d->writer.open() < 0) {
    return -1;
  }
  _d->strm           = new z_stream;
  _d->strm->zalloc   = Z_NULL;
  _d->strm->zfree    = Z_NULL;
  _d->strm->opaque   = Z_NULL;
  _d->strm->avail_in = 0;
  _d->strm->next_in  = Z_NULL;
  // De-compress
  if (deflateInit2(_d->strm, _d->level, Z_DEFLATED, 16 + 15, 9,
                   Z_DEFAULT_STRATEGY) != Z_OK) {
    close();
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
  deflateEnd(_d->strm);
  delete _d->strm;
  _d->strm = NULL;
  return _d->writer.close();
}

ssize_t ZipWriter::write(const void* buffer, size_t size) {
  if ( _d->finished) {
    return 0;
  }
  if (size == 0) {
    _d->finished = true;
  } else {
    _d->strm->avail_in = static_cast<uInt>(size);
    // Casting away the constness here!!!
    _d->strm->next_in  = static_cast<Bytef*>(const_cast<void*>(buffer));
  }
  do {
    _d->strm->avail_out = static_cast<uInt>(BUFFER_SIZE);
    _d->strm->next_out  = _d->buffer;
    deflate(_d->strm, _d->finished ? Z_FINISH : Z_NO_FLUSH);
    ssize_t length = BUFFER_SIZE - _d->strm->avail_out;
    if (_d->writer.write(_d->buffer, length) < 0) {
      return -1;
    }
  } while (_d->strm->avail_out == 0);
  return size;
}
