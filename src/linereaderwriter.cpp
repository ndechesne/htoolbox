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

#include "stdlib.h"
#include "string.h"
#include "errno.h"

#include "report.h"
#include "linereaderwriter.h"

using namespace htoolbox;

enum {
  BUFFER_SIZE = 102400
};

struct LineReaderWriter::Private {
  IReaderWriter*  child;
  char            buffer[102400];
  const char*     buffer_end;
  const char*     reader;
  bool            found;
  Private(IReaderWriter* c) : child(c) {}
};

LineReaderWriter::LineReaderWriter(IReaderWriter* child, bool delete_child) :
  IReaderWriter(child, delete_child), _d(new Private(child)) {}

LineReaderWriter::~LineReaderWriter() {
  delete _d;
}

int LineReaderWriter::open() {
  _d->reader = _d->buffer_end = _d->buffer;
  _offset = 0;
  return _d->child->open();
}

int LineReaderWriter::close() {
  return _d->child->close();
}

ssize_t LineReaderWriter::read(void* buffer, size_t size) {
  // Check for buffered data first
  size_t copy_size = _d->buffer_end - _d->reader;
  if (copy_size != 0) {
    if (size <= copy_size) {
      copy_size = size;
    }
    memcpy(buffer, _d->reader, copy_size);
    _d->reader += copy_size;
  } else {
    copy_size = _d->child->read(buffer, size);
  }
  _offset += copy_size;
  return copy_size;
}

ssize_t LineReaderWriter::get(void* buffer, size_t size) {
  // Check for buffered data first
  size_t copy_size = _d->buffer_end - _d->reader;
  if (copy_size != 0) {
    if (size <= copy_size) {
      // Get buffered data and exit
      copy_size = size;
    }
    memcpy(buffer, _d->reader, copy_size);
    _d->reader += copy_size;
    if (copy_size == size) {
      _offset += copy_size;
      return copy_size;
    }
  }
  // If we are here, then we need more data
  char* cbuffer = static_cast<char*>(buffer);
  ssize_t rc = _d->child->get(&cbuffer[copy_size], size - copy_size);
  if (rc < 0) {
    return rc;
  }
  copy_size += rc;
  _offset += copy_size;
  return copy_size;
}

ssize_t LineReaderWriter::put(const void* buffer, size_t size) {
  ssize_t rc = _d->child->put(buffer, size);
  if (rc > 0) {
    _offset += rc;
  }
  return rc;
}

int64_t LineReaderWriter::childOffset() const {
  return _child->offset();
}

ssize_t LineReaderWriter::getLine(
    char**          buffer_p,
    size_t*         capacity_p,
    int             delim,
    int             delim2) {
  // Initialise buffer, at least for the null character
  if ((*buffer_p == NULL) || (*capacity_p == 0)) {
    *capacity_p = 1024;
    *buffer_p = static_cast<char*>(malloc(*capacity_p));
  }
  // Find end of line or end of file
  size_t count = 0;
  bool   found_first = false;
  _d->found = false;
  // Look for delimiter or end of file
  do {
    // Fill up the buffer
    if (_d->reader == _d->buffer_end) {
      ssize_t rc = _d->child->get(_d->buffer, sizeof(_d->buffer));
      if (rc < 0) {
        return rc;
      }
      if (rc == 0) {
        break;
      }
      _d->reader = _d->buffer;
      _d->buffer_end = _d->buffer + rc;
    }
    const char* start_reader = _d->reader;
    if (found_first) {
      if (*_d->reader++ == delim2) {
        _d->found = true;
      } else {
        found_first = false;
      }
    }
    if (! _d->found) {
      // Look for delimiter or end of buffer
      const void* pos = memchr(_d->reader, delim, _d->buffer_end - _d->reader);
      if (pos == NULL) {
        _d->reader = _d->buffer_end;
      } else {
        _d->reader = static_cast<const char*>(pos);
        _d->reader++;
        if (delim2 < 0) {
          _d->found = true;
        } else
        if ((_d->reader < _d->buffer_end) && (*_d->reader++ == delim2)) {
          _d->found = true;
        } else {
          found_first = true;
        }
      }
    }
    // Copy whatever we read
    size_t to_add = _d->reader - start_reader;
    if (count + to_add >= *capacity_p) {
      // Leave one space for the null character
      while (*capacity_p < (count + to_add + 1)) {
        *capacity_p <<= 1;
      }
      *buffer_p = static_cast<char*>(realloc(*buffer_p, *capacity_p));
    }
    memcpy(&(*buffer_p)[count], start_reader, to_add);
    count += to_add;
  } while (! _d->found);
  (*buffer_p)[count] = '\0';
  _offset += count;
  return count;
}

bool LineReaderWriter::delimsWereFound() const {
  return _d->found;
}

ssize_t LineReaderWriter::putLine(
    const void*     buffer,
    size_t          size,
    int             delim,
    int             delim2) {
  ssize_t rc = _d->child->put(buffer, size);
  if ((rc < 0) || (_d->child->put(&delim, 1) < 0)) {
    return -1;
  }
  _offset += rc + 1;
  if (delim2 >= 0) {
    if (_d->child->put(&delim2, 1) < 0) {
      return -1;
    }
    ++_offset;
  }
  return rc;
}
