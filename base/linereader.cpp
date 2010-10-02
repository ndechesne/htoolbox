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
#include "string.h"
#include "errno.h"

#include "hreport.h"
#include "linereader.h"

using namespace htools;

enum {
  BUFFER_SIZE = 102400
};

struct LineReader::Private {
  IReaderWriter*  child;
  bool            delete_child;
  char            buffer[102400];
  const char*     buffer_end;
  const char*     reader;
  Private(IReaderWriter* c, bool d) : child(c), delete_child(d) {}
  void reset() {
    reader = buffer_end = buffer;
  }
  ssize_t refill() {
    ssize_t rc = child->read(buffer, sizeof(buffer));
    reader = buffer;
    buffer_end = buffer + rc;
    return rc;
  }
  static int grow(char** buffer, size_t* capacity_p, size_t target) {
    bool no_realloc = false;
    if ((*buffer == NULL) || (*capacity_p == 0)) {
      *capacity_p = 1;
      no_realloc = true;
    }
    while (*capacity_p < target) {
      *capacity_p <<= 1;
    }
    if (no_realloc) {
      *buffer = static_cast<char*>(malloc(*capacity_p));
    } else {
      *buffer = static_cast<char*>(realloc(*buffer, *capacity_p));
    }
    return *buffer == NULL ? -1 : 0;
  }
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
  _d->reset();
  return _d->child->open();
}

int LineReader::close() {
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

ssize_t LineReader::getDelim(char** buffer_p, size_t* capacity_p, int delim) {
  // Find end of line or end of file
  size_t count = 0;
  bool   found = false;
  // Initialise buffer, at least for the null character
  _d->grow(buffer_p, capacity_p, 128);
  // Look for delimiter or end of file
  do {
    // Fill up the buffer
    if (_d->reader == _d->buffer_end) {
      int rc = _d->refill();
      if (rc < 0) {
        return rc;
      }
      if (rc == 0) {
        break;
      }
    }
    // Look for delimiter or end of buffer
    const char* start_reader = _d->reader;
    const void* pos = memchr(_d->reader, delim, _d->buffer_end - _d->reader);
    if (pos == NULL) {
      _d->reader = _d->buffer_end;
    } else {
      _d->reader = static_cast<const char*>(pos);
      _d->reader++;
      found = true;
    }
    // Copy whatever we read
    size_t to_add = _d->reader - start_reader;
    if ((count + to_add >= *capacity_p) || (buffer_p == NULL)) {
      // Leave one space for the null character
      _d->grow(buffer_p, capacity_p, count + to_add + 1);
    }
    memcpy(&(*buffer_p)[count], start_reader, to_add);
    count += to_add;
  } while (! found);
  (*buffer_p)[count] = '\0';
  return count;
}
