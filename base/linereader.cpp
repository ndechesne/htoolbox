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

#include "stdlib.h"
#include "errno.h"

#include "hreport.h"
#include "buffer.h"
#include "linereader.h"

using namespace htools;

enum {
  BUFFER_SIZE = 102400
};

struct LineReader::Private {
  IReaderWriter*      child;
  bool                delete_child;
  Buffer<char>        getline_buffer;     // Buffer for data
  BufferReader<char>  getline_reader;        // Reader for buffer above
  Private(IReaderWriter* c, bool d) : child(c), delete_child(d),
    getline_reader(getline_buffer) {}
};

LineReader::LineReader(IReaderWriter* child, bool delete_child) :
  _d(new Private(child, delete_child)) {}

LineReader::~LineReader() {
  if (_d->delete_child) {
    delete _d->child;
  }
  delete _d;
}

int LineReader::open() {
  _d->getline_buffer.create();
  return _d->child->open();
}

int LineReader::close() {
  _d->getline_buffer.destroy();
  return _d->child->close();
}

ssize_t LineReader::read(void*, size_t) {
  hlog_alert("cannot read from line reader module");
  errno = EPROTO;
  return -1;
}

// Not implemented
ssize_t LineReader::write(const void*, size_t) {
  hlog_alert("cannot write to line reader module");
  errno = EPROTO;
  return -1;
}

ssize_t LineReader::getLine(char** buffer, size_t* buffer_capacity) {
  // Make sure we have a buffer
  if (*buffer == NULL) {
    *buffer_capacity = 1024;
    *buffer = static_cast<char*>(malloc(*buffer_capacity));
  }
  // Find end of line or end of file
  size_t count  = 0;
  char*  writer = *buffer;
  bool   found  = false;
  do {
    // Fill up the buffer
    ssize_t size = _d->getline_reader.readable();
    if (size == 0) {
      size = _d->child->read(_d->getline_buffer.writer(),
                             _d->getline_buffer.writeable());
      if (size < 0) {
        return size;
      }
      if (size == 0) {
        break;
      }
      _d->getline_buffer.written(size);
    }
    const char* reader = _d->getline_reader.reader();
    size_t      length = size;
    while (length > 0) {
      length--;
      // Check for space
      if (count >= *buffer_capacity) {
        *buffer_capacity <<= 1;
        *buffer = static_cast<char*>(realloc(*buffer, *buffer_capacity));
        writer = &(*buffer)[count];
      }
      *writer++ = *reader;
      count++;
      if (*reader++ == '\n') {
        found = true;
        break;
      }
    }
    _d->getline_reader.readn(size - length);
  } while (! found);
  *writer = '\0';
  return count;
}
