/*
     Copyright (C) 2008  Herve Fache

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

#include <stdlib.h>
#include <string.h>

#include "buffer.h"

using namespace hbackup;

struct Buffer::Private {
  char* buffer;
  char* writer;
  const char* reader;
  const char* end;
  bool empty;
  bool full;
};

Buffer::Buffer(size_t size) : _d(new Private) {
  if (size > 0) {
    create(size);
  } else {
    _d->buffer = NULL;
  }
}

Buffer::~Buffer() {
  if (exists()) {
    destroy();
  }
  delete _d;
}

void Buffer::create(size_t size) {
  _d->buffer = (char*) malloc(size);
  _d->end    = &_d->buffer[size];
  empty();
}

void Buffer::destroy() {
  free(_d->buffer);
  _d->buffer = NULL;
}

void Buffer::empty() {
  _d->writer = _d->buffer;
  _d->reader = _d->buffer;
  _d->empty  = true;
  _d->full   = false;
}

bool Buffer::exists() const {
  return _d->buffer != NULL;
}

size_t Buffer::capacity() const {
  return _d->end - _d->buffer;
}

bool Buffer::isEmpty() const {
  return _d->empty;
}

bool Buffer::isFull() const {
  return _d->full;
}

char* Buffer::writer() {
  return _d->writer;
}

size_t Buffer::writeable() const {
  if (_d->writer < _d->reader) {
    return _d->reader - _d->writer;
  } else
  if ((_d->writer > _d->reader) || ! _d->full) {
    return _d->end - _d->writer;
  } else
  {
    return 0;
  }
}

void Buffer::written(size_t size) {
  if (size == 0) return;
  _d->empty = false;
  _d->writer += size;
  if (_d->writer >= _d->end) {
    _d->writer = _d->buffer;
  }
  if (_d->writer == _d->reader) {
    _d->full = true;
  }
}

ssize_t Buffer::write(const void* buffer, size_t size) {
  size_t really = 0;
  const char* next = (const char*) buffer;
  while ((size > 0) && (writeable() > 0)) {
    size_t can;
    if (size > writeable()) {
      can = writeable();
    } else {
      can = size;
    }
    memcpy(writer(), next, can);
    next += can;
    written(can);
    really += can;
    size -= really;
  }
  return really;
}

const char* Buffer::reader() const {
  return _d->reader;
}

size_t Buffer::readable() const {
  if (_d->reader < _d->writer) {
    return _d->writer - _d->reader;
  } else
  if ((_d->reader > _d->writer) || _d->full) {
    return _d->end - _d->reader;
  } else
  {
    return 0;
  }
}

void Buffer::readn(size_t size) {
  if (size == 0) return;
  _d->full = false;
  _d->reader += size;
  if (_d->reader >= _d->end) {
    _d->reader = _d->buffer;
  }
  if (_d->writer == _d->reader) {
    // Do not call empty here in case of concurrent read/write access
    _d->empty = true;
  }
}

ssize_t Buffer::read(void* buffer, size_t size) {
  size_t really = 0;
  void* next = buffer;
  while ((size > 0) && (readable() > 0)) {
    size_t can;
    if (size > readable()) {
      can = readable();
    } else {
      can = size;
    }
    next = mempcpy(next, reader(), can);
    readn(can);
    really += can;
    size -= really;
  }
  return really;
}
