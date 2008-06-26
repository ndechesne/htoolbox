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
  _write_id = 0;
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
  _write_start = _buffer_start;
  _write_end   = _buffer_start;
  _full        = false;
  for (std::list<BufferReader*>::iterator i = _readers.begin();
      i != _readers.end(); i++) {
    (*i)->empty();
  }
}

void Buffer::update() {
  int min_free = _buffer_end - _buffer_start;
  for (std::list<BufferReader*>::iterator i = _readers.begin();
      i != _readers.end(); i++) {
    // Buffer free space for this reader
    int this_free = (*i)->reader() - _write_start;
    // Wrapped
    if (this_free < 0) {
      this_free += _buffer_end - _buffer_start;
    }
    // Better or empty
    if ((this_free < min_free) && ((this_free != 0) || ! (*i)->isEmpty())) {
      min_free = this_free;
    }
  }
  // Compute new end
  _write_end = _write_start + min_free;
  if (_write_end >= _buffer_end) {
    _write_end -= _buffer_end - _buffer_start;
  }
  _full = (min_free == 0);
}

bool Buffer::exists() const {
  return _buffer_start != NULL;
}

size_t Buffer::usage() const {
  if (_write_start > _write_end) {
    return _write_start - _write_end;
  } else
  if ((_write_start < _write_end) || _full) {
    return _buffer_end - _buffer_start + _write_start - _write_end;
  } else
  {
    return 0;
  }
}

size_t Buffer::writeable() const {
  if (_full) {
    return 0;
  } else
  if (_write_start < _write_end) {
    return _write_end - _write_start;
  } else
  {
    return _buffer_end - _write_start;
  }
}

void Buffer::written(size_t size) {
  if (size == 0) return;
  _write_start += size;
  if (_write_start >= _buffer_end) {
    _write_start = _buffer_start;
  }
  if (_write_start == _write_end) {
    _full = true;
  }
  _write_id++;
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

void BufferReader::empty() {
  _read_start = _buffer._write_end;
  _empty_id   = _buffer._write_id;
}

size_t BufferReader::readable() const {
  // The readable/written part is [_write_end _write_start[
  if (_read_start < _buffer._write_start) {
    return _buffer._write_start - _read_start;
  } else
  // Wrapped
  if ((_read_start > _buffer._write_start) || _buffer._full) {
    return _buffer._buffer_end - _read_start;
  } else
  // Empty
  {
    return 0;
  }
}

void BufferReader::readn(size_t size) {
  if (size == 0) return;
  _read_start += size;
  if (_read_start >= _buffer._buffer_end) {
    _read_start = _buffer._buffer_start;
  }
  // Read it all?
  if (_read_start == _buffer._write_start) {
    _empty_id = _buffer._write_id;
  }
  if (_buffer._readers_size == 1) {
    // Just update
    _buffer._write_end = _read_start;
    _buffer._full      = false;
  } else {
    // Tell write to find new end
    _buffer.update();
  }
}

ssize_t BufferReader::read(void* buffer, size_t size) {
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
