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

struct Line::Private {
  unsigned int      capacity;
  unsigned int      size;
  char*             buffer;
  unsigned int      add_size;
};

Line::Line(unsigned int init_size, unsigned int add_size) : _d(new Private) {
  _d->capacity = init_size;
  _d->buffer   = NULL;
  _d->add_size = add_size;
  if (_d->capacity != 0) {
    _d->buffer = static_cast<char*>(malloc(_d->capacity));
  }
  _d->size = 0;
}

Line::Line(const char* line, unsigned int add_size) : _d(new Private) {
  _d->capacity = 0;
  _d->buffer   = NULL;
  _d->add_size = add_size;
  if (line != NULL) {
    operator=(line);
  } else {
    _d->size = 0;
  }
}

// What a shame to have to duplicate!
Line::Line(const Line& line) : _d(new Private) {
  _d->capacity = 0;
  _d->buffer   = NULL;
  _d->add_size = 0;
  if (line._d->buffer != NULL) {
    resize(line._d->capacity, line._d->add_size, true);
    operator=(line._d->buffer);
  } else {
    _d->size = 0;
  }
}

Line::~Line() {
  free(_d->buffer);
  delete _d;
}

const Line& Line::operator=(const Line& line) {
  return operator=(line._d->buffer);
}

const Line& Line::operator=(const char* line) {
  _d->size = strlen(line);
  if ((_d->size + 1) > _d->capacity) {
    resize(_d->size + 1, -1, true);
  }
  strcpy(_d->buffer, line);
  return *this;
}

const char& Line::operator[](int pos) const {
  return _d->buffer[pos];
}

Line::operator const char* () const {
  return _d->buffer;
}

int Line::resize(unsigned int new_size, int new_add_size, bool discard) {
  if (new_add_size >= 0) {
    _d->add_size = new_add_size;
  }
  if (new_size > _d->capacity) {
    if (_d->add_size == 0) {
      _d->capacity = new_size;
    } else {
      while (new_size > _d->capacity) {
        _d->capacity += _d->add_size;
      }
    }
    if (discard) {
      free(_d->buffer);
      _d->buffer = static_cast<char*>(malloc(_d->capacity));
    } else {
      _d->buffer = static_cast<char*>(realloc(_d->buffer, _d->capacity));
    }
  }
  return 0;
}

unsigned int Line::size() const {
  return _d->size;
}

const Line& Line::erase(int pos) {
  _d->size        = pos;
  _d->buffer[pos] = 0;
  return *this;
}

int Line::find(char c, int pos) const {
  const char* p = strchr(&_d->buffer[pos], c);
  if (p != NULL) {
    return p - _d->buffer;
  }
  return -1;
}

const Line& Line::append(const Line& line, int pos, int num) {
  return append(line._d->buffer, pos, num);
}

const Line& Line::append(const char* line, int pos, int num) {
  if (num < 0) {
    // Copy the whole line
    num = strlen(line);
  }
  if (pos < 0) {
    pos = strlen(_d->buffer);
  }
  if (num > 0) {
    _d->size = pos + num;
    if ((_d->size + 1) > _d->capacity) {
      resize(_d->size + 1);
    }
    strncpy(&_d->buffer[pos], line, num);
    _d->buffer[_d->size] = 0;
  }
  return *this;
}

char** Line::bufferPtr() {
  return &_d->buffer;
}

unsigned int* Line::capacityPtr() {
  return &_d->capacity;
}

unsigned int* Line::sizePtr() {
  return &_d->size;
}

unsigned int Line::capacity() const {
  return _d->capacity;
}

unsigned int Line::add_size() const {
  return _d->add_size;
}
