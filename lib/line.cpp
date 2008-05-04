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

Line::Line(unsigned int init_size, unsigned int add_size) :
    _capacity(init_size), _buffer(NULL), _add_size(add_size) {
  if (_capacity != 0) {
    _buffer = (char*) malloc(_capacity);
  }
  _size = 0;
}

Line::Line(const char* line, unsigned int add_size) :
    _capacity(0), _buffer(NULL), _add_size(add_size) {
  if (line != NULL) {
    operator=(line);
  } else {
    _size = 0;
  }
}

// What a shame to have to duplicate!
Line::Line(const Line& line) : _capacity(0), _buffer(NULL), _add_size(0) {
  if (line._buffer != NULL) {
    resize(line._capacity, line._add_size, true);
    operator=(line._buffer);
  } else {
    _size = 0;
  }
}

Line::~Line() {
  free(_buffer);
}

const Line& Line::operator=(const Line& line) {
  return operator=(line._buffer);
}

const Line& Line::operator=(const char* line) {
  _size = strlen(line);
  if ((_size + 1) > _capacity) {
    resize(_size + 1, -1, true);
  }
  strcpy(_buffer, line);
  return *this;
}

int Line::resize(unsigned int new_size, int new_add_size, bool discard) {
  if (new_add_size >= 0) {
    _add_size = new_add_size;
  }
  if (new_size > _capacity) {
    if (_add_size == 0) {
      _capacity = new_size;
    } else {
      while (new_size > _capacity) {
        _capacity += _add_size;
      }
    }
    if (discard) {
      free(_buffer);
      _buffer = (char*) malloc(_capacity);
    } else {
      _buffer = (char*) realloc(_buffer, _capacity);
    }
  }
  return 0;
}

const Line& Line::erase(int pos) {
  _size = pos;
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
    _size = pos + num;
    if ((_size + 1) > _capacity) {
      resize(_size + 1);
    }
    strncpy(&_buffer[pos], line, num);
    _buffer[_size] = 0;
  }
  return *this;
}
