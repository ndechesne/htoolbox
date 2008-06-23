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

void Buffer::registerReader(BufferReader* reader) {
  _readers.push_back(reader);
  _readers_size = _readers.size();
}

int Buffer::unregisterReader(const BufferReader* reader) {
  bool found = false;
  for (std::list<BufferReader*>::iterator i = _readers.begin();
      i != _readers.end(); i++) {
    if (*i == reader) {
      _readers.erase(i);
      found = true;
      break;
    }
  }
  _readers_size = _readers.size();
  return found ? 0 : -1;
}

Buffer::Buffer(size_t size) {
  _readers_size = 0;
  if (size > 0) {
    create(size);
  } else {
    _buffer_start = NULL;
  }
}

Buffer::~Buffer() {
  if (exists()) {
    destroy();
  }
}

void Buffer::create(size_t size) {
  _buffer_start = static_cast<char*>(malloc(size));
  _buffer_end   = &_buffer_start[size];
  empty();
}

void Buffer::destroy() {
  free(_buffer_start);
  _buffer_start = NULL;
}

void Buffer::empty() {
  _writer_start = _buffer_start;
  _writer_end   = _buffer_start;
  _full         = false;
}

bool Buffer::exists() const {
  return _buffer_start != NULL;
}

size_t Buffer::usage() const {
  if (_writer_start > _writer_end) {
    return _writer_start - _writer_end;
  } else
  if ((_writer_start < _writer_end) || _full) {
    return _buffer_end - _buffer_start + _writer_start - _writer_end;
  } else
  {
    return 0;
  }
}

size_t Buffer::writeable() const {
  if (_full) {
    return 0;
  } else
  if (_writer_start < _writer_end) {
    return _writer_end - _writer_start;
  } else
  {
    return _buffer_end - _writer_start;
  }
}

void Buffer::written(size_t size) {
  if (size == 0) return;
  _writer_start += size;
  if (_writer_start >= _buffer_end) {
    _writer_start = _buffer_start;
  }
  if (_writer_start == _writer_end) {
    _full = true;
  }
}

ssize_t Buffer::write(const void* buffer, size_t size) {
  size_t really = 0;
  const char* next = static_cast<const char*>(buffer);
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

size_t Buffer::readable() const {
  // The readable/written part is [_writer_end _writer_start[
  if (_writer_end < _writer_start) {
    return _writer_start - _writer_end;
  } else
  // Wrapped
  if ((_writer_end > _writer_start) || _full) {
    return _buffer_end - _writer_end;
  } else
  // Empty
  {
    return 0;
  }
}

void Buffer::readn(size_t size) {
  if (size == 0) return;
  _writer_end += size;
  _full        = false;
  if (_writer_end >= _buffer_end) {
    _writer_end = _buffer_start;
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
