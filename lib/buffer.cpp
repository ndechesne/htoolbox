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
    _buffer = NULL;
  }
}

Buffer::~Buffer() {
  if (exists()) {
    destroy();
  }
}

void Buffer::create(size_t size) {
  _buffer = static_cast<char*>(malloc(size));
  _end    = &_buffer[size];
  empty();
}

void Buffer::destroy() {
  free(_buffer);
  _buffer = NULL;
}

void Buffer::empty() {
  _writer = _buffer;
  _reader = _buffer;
  _empty  = true;
}

bool Buffer::exists() const {
  return _buffer != NULL;
}

size_t Buffer::usage() const {
  if (_empty) {
    return 0;
  } else
  if (_reader < _writer) {
    return _writer - _reader;
  } else
  {
    return _end - _buffer + _writer - _reader;
  }
}

size_t Buffer::writeable() const {
  if (_writer < _reader) {
    return _reader - _writer;
  } else
  if ((_writer > _reader) || _empty) {
    return _end - _writer;
  } else
  {
    return 0;
  }
}

void Buffer::written(size_t size) {
  if (size == 0) return;
  _empty = false;
  _writer += size;
  if (_writer >= _end) {
    _writer = _buffer;
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
  if (_reader < _writer) {
    return _writer - _reader;
  } else
  if ((_reader > _writer) || ! _empty) {
    return _end - _reader;
  } else
  {
    return 0;
  }
}

void Buffer::readn(size_t size) {
  if (size == 0) return;
  _reader += size;
  if (_reader >= _end) {
    _reader = _buffer;
  }
  if (_writer == _reader) {
    // Do not call empty here in case of concurrent read/write access
    _empty = true;
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
