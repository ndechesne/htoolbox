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
  _write_count = 0;
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
  _capacity     = size;
  empty();
}

void Buffer::destroy() {
  free(_buffer_start);
  _buffer_start = NULL;
}

void Buffer::empty() {
  _write_start    = _buffer_start;
  _write_count    = 0;
  _read_count     = 0;
  _readers_update = false;
  for (std::list<BufferReader*>::iterator i = _readers.begin();
      i != _readers.end(); i++) {
    (*i)->empty();
  }
}

void Buffer::update() {
  int max_used = 0;
  for (std::list<BufferReader*>::iterator i = _readers.begin();
      i != _readers.end(); i++) {
    // Unread space for this reader
    int this_used = _write_count - (*i)->_read_count;;
    if (this_used > max_used) {
      max_used = this_used;
    }
  }
  _read_count = _write_count - max_used;
}

bool Buffer::exists() const {
  return _buffer_start != NULL;
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
