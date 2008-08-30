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

#ifndef LINE_H
#define LINE_H

#define HAVE_LINE_H 1

namespace hbackup {

class LineBuffer {
  static const unsigned int  _min_capacity = 128;
  char*             _line;
  unsigned int      _capacity;
  unsigned int      _size;
  LineBuffer(const LineBuffer&);
  int grow(unsigned int required);
public:
  // The no of instances and destruction are managed by this class' users
  unsigned int      _instances;
  LineBuffer() {
    _line      = NULL;
    _capacity  = 0;
    grow(1);
    _line[0]   = '\0';
    _instances = 0;
  }
  LineBuffer(const char* line) {
    _line      = NULL;
    _capacity  = 0;
    _size      = strlen(line);
    grow(_size + 1);
    memcpy(_line, line, _size + 1);
    _instances = 0;
  }
  LineBuffer(const LineBuffer& line, const LineBuffer& line2) {
    _line      = NULL;
    _capacity  = 0;
    _size      = line._size + line2._size;
    grow(_size + 1);
    memcpy(_line, line._line, line._size);
    memcpy(&_line[line._size], line2._line, line2._size + 1);
    _instances = 0;
  }
  LineBuffer(const LineBuffer& line, const char* line2) {
    _line      = NULL;
    _capacity  = 0;
    unsigned int size2 = strlen(line2);
    _size      = line._size + size2;
    grow(_size + 1);
    memcpy(_line, line._line, line._size);
    memcpy(&_line[line._size], line2, size2 + 1);
    _instances = 0;
  }
  ~LineBuffer() {
    free(_line);
  }
  operator const char* () {
    return _line;
  }
  LineBuffer& operator=(const LineBuffer& line) {
    _size = line._size;
    grow(_size + 1);
    memcpy(_line, line._line, line._size + 1);
    return *this;
  }
  LineBuffer& operator=(const char* line) {
    _size = strlen(line);
    grow(_size + 1);
    memcpy(_line, line, _size + 1);
    return *this;
  }
  LineBuffer& operator+=(const LineBuffer& line) {
    grow(_size + line._size + 1);
    memcpy(&_line[_size], line._line, line._size + 1);
    _size += line._size;
    return *this;
  }
  LineBuffer& operator+=(const char* line) {
    unsigned int size = strlen(line);
    grow(_size + size + 1);
    memcpy(&_line[_size], line, size + 1);
    _size += size;
    return *this;
  }
  const LineBuffer& erase(unsigned int pos = 0) {
    if (pos < _size) {
      _size = pos;
      _line[_size] = '\0';
    }
    return *this;
  }
  // For compatibility
  char**        bufferPtr()     { return &_line;     }
  unsigned int* sizePtr()       { return &_size;     }
  unsigned int* capacityPtr()   { return &_capacity; }
  // For test
  unsigned int size() const     { return _size;      }
  unsigned int capacity() const { return _capacity;  }
};

class Line {
  LineBuffer*       _line;
public:
  Line() {
    _line = new LineBuffer();
    _line->_instances++;
  }
  Line(const char* line) {
    _line = new LineBuffer(line);
    _line->_instances++;
  }
  Line(const Line& line) {
    _line = line._line;
    _line->_instances++;
  }
  ~Line() {
    if (--_line->_instances == 0) {
      delete _line;
    }
  }
  operator const char* () const {
    return *_line;
  }
  unsigned int size() const {
    return _line->size();
  }
  Line& operator=(const Line& line) {
    if (--_line->_instances == 0) {
      delete _line;
    }
    _line = line._line;
    _line->_instances++;
    return *this;
  }
  Line& operator=(const char* line) {
    if (_line->_instances > 1) {
      _line->_instances--;
      _line = new LineBuffer(line);
      _line->_instances++;
    } else {
      *_line = line;
    }
    return *this;
  }
  Line& operator+=(const Line& line) {
    if (_line->_instances > 1) {
      _line->_instances--;
      _line = new LineBuffer(*_line, *line._line);
      _line->_instances++;
    } else {
      *_line += *line._line;
    }
    return *this;
  }
  Line& operator+=(const char* line) {
    if (_line->_instances > 1) {
      _line->_instances--;
      _line = new LineBuffer(*_line, line);
      _line->_instances++;
    } else {
      *_line += line;
    }
    return *this;
  }
  const Line& erase(unsigned int pos = 0) {
    _line->erase(pos);
    return *this;
  }
  // For compatibility
  int find(char c, unsigned int pos = 0) const {
    const char* buffer = *_line;
    const char* r;
    for (r = &buffer[pos]; r < &buffer[_line->size()]; r++) {
      if (*r == c) {
        break;
      }
    }
    return r < &buffer[_line->size()] ? r - buffer : -1;
  }
  const Line& append(const char* line, int pos = -1) {
    if (pos >= 0) {
      erase(pos);
    }
    operator+=(line);
    return *this;
  }
  // For test
  operator const LineBuffer& () const {
    return *_line;
  }
  LineBuffer& instance() const {
    return *_line;
  }
};

}

#endif
