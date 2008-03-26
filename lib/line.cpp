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

using namespace std;

#include "line.h"

using namespace hbackup;

Line::Line(const char* line) : _capacity(0), _buffer(NULL) {
  if (line != NULL) {
    operator=(line);
  }
}

// What a shame to have to duplicate!
Line::Line(const Line& line) : _capacity(0), _buffer(NULL) {
  if (line._buffer != NULL) {
    operator=(line._buffer);
  }
}

Line::~Line() {
  free(_buffer);
}

const Line& Line::operator=(const Line& line) {
  return operator=(line._buffer);
}

const Line& Line::operator=(const char* line) {
  int size = strlen(line) + 1;
  if (size > _capacity) {
    free(_buffer);
    _capacity = size;
    _buffer   = strdup(line);
  } else {
    strcpy(_buffer, line);
  }
  return *this;
}

const Line& Line::erase(int pos) {
  _buffer[pos] = 0;
  return *this;
}

int Line::find(char c, int pos) const {
  const char* p = strchr(&_buffer[pos], c);
  if (p != NULL) {
    return p - _buffer;
  }
  return -1;
}

const Line& Line::append(const Line& line, int pos, int num) {
  return append(line._buffer, pos, num);
}

const Line& Line::append(const char* line, int pos, int num) {
  if (num < 0) {
    // Copy the whole line
    num = strlen(line);
  }
  if (pos < 0) {
    pos = strlen(_buffer);
  }
  if (num > 0) {
    int size = pos + num + 1;
    if (_capacity < size) {
      _capacity = size;
      _buffer   = (char*) realloc(_buffer, _capacity);
    }
    strncpy(&_buffer[pos], line, num);
    _buffer[size - 1] = 0;
  }
  return *this;
}
