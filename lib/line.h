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

#ifdef _DEBUG
#include <iostream>
#endif

#include <cstdlib>
#include <cstring>

namespace hbackup {

#ifdef _DEBUG
extern bool         _Line_debug;
extern unsigned int _Line_address;
#endif

class LineBuffer {
  static const unsigned int  _min_capacity = 128;
  char*             _line;
  unsigned int      _capacity;
  unsigned int      _size;
#ifdef _DEBUG
  unsigned int      _address;
#endif
  int grow(unsigned int required, bool discard = false);
  LineBuffer(const LineBuffer&);
  LineBuffer& operator=(const LineBuffer&);
public:
  // The no of instances and destruction are managed by this class' users
  unsigned int      _instances;
  LineBuffer() {
    _line      = NULL;
    _capacity  = 0;
    _size      = 0;
    _instances = 0;
#ifdef _DEBUG
    _address   = ++_Line_address;
#endif
  }
  LineBuffer(const LineBuffer& line, const LineBuffer& line2) {
    _line      = NULL;
    _capacity  = 0;
    _size      = line._size + line2._size;
    grow(_size + 1);
    memcpy(_line, line._line, line._size);
    memcpy(&_line[line._size], line2._line, line2._size + 1);
    _instances = 0;
#ifdef _DEBUG
    _address   = ++_Line_address;
#endif
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
#ifdef _DEBUG
    _address   = ++_Line_address;
#endif
  }
  ~LineBuffer() {
    free(_line);
  }
  operator const char* () {
    return _line;
  }
  char& operator[](unsigned int pos) {
    return _line[pos];
  }
  LineBuffer& set(const LineBuffer& line, ssize_t size = -1) {
    if ((size >= 0) && (static_cast<size_t>(size) < line._size)) {
      _size = size;
    } else {
      _size = line._size;
    }
    grow(_size + 1, true);
    memcpy(_line, line._line, _size);
    _line[_size] = '\0';
    return *this;
  }
  LineBuffer& set(const char* line, ssize_t size = -1) {
    ssize_t length = strlen(line);
    if ((size >= 0) && (size < length)) {
      _size = size;
    } else {
      _size = length;
    }
    grow(_size + 1, true);
    memcpy(_line, line, _size);
    _line[_size] = '\0';
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
#ifdef _DEBUG
  unsigned int address() const  { return _address;   }
#endif
};

class Line {
  LineBuffer*       _line;
  LineBuffer*       _deleted_line;
  void disposeBuffer();
public:
  Line() {
    _line = new LineBuffer();
    _line->set("");
    _deleted_line = NULL;
    _line->_instances++;
#ifdef _DEBUG
    if (_Line_debug) {
      std::cout << "dflt ctor: created empty buffer #" << _line->address()
        << std::endl;
    }
#endif
  }
  Line(const char* line) {
    _line = new LineBuffer();
    _line->set(line);
    _deleted_line = NULL;
    _line->_instances++;
#ifdef _DEBUG
    if (_Line_debug) {
      std::cout << "cstr ctor: created buffer #" << _line->address()
        << std::endl;
    }
#endif
  }
  Line(const Line& line) {
    _line         = line._line;
    _deleted_line = NULL;
    _line->_instances++;
#ifdef _DEBUG
    if (_Line_debug) {
      std::cout << "copy ctor: copied buffer #" << _line->address()
        << " (" << _line->_instances << ")" << std::endl;
    }
#endif
  }
  ~Line() {
    if (--_line->_instances == 0) {
#ifdef _DEBUG
      if (_Line_debug) {
        std::cout << "dtor: deleted buffer #" << _line->address() << std::endl;
      }
#endif
      delete _line;
    }
    if ((_deleted_line != NULL) && (_deleted_line->_instances == 0)) {
#ifdef _DEBUG
      if (_Line_debug) {
        std::cout << "dtor: deleted extra buffer #" << _deleted_line->address()
          << std::endl;
      }
#endif
      delete _deleted_line;
    }
  }
  operator const char* () const {
    return *_line;
  }
  unsigned int size() const {
    return _line->size();
  }
  Line& operator=(const Line& line) {
    disposeBuffer();
    _line = line._line;
    _line->_instances++;
#ifdef _DEBUG
    if (_Line_debug) {
      std::cout << "ln= : copied buffer #" << _line->address()
        << " (" << _line->_instances << ")" << std::endl;
    }
#endif
    return *this;
  }
  Line& operator=(const char* line) {
    if (_line->_instances > 1) {
      _line->_instances--;
      _line = new LineBuffer();
      _line->set(line);
      _line->_instances++;
#ifdef _DEBUG
      if (_Line_debug) {
        std::cout << "cs= : created buffer #" << _line->address() << std::endl;
      }
#endif
    } else {
      _line->set(line);
    }
    return *this;
  }
  Line& operator+=(const Line& line) {
    if (_line->_instances > 1) {
      _line->_instances--;
      _line = new LineBuffer(*_line, *line._line);
      _line->_instances++;
#ifdef _DEBUG
      if (_Line_debug) {
        std::cout << "ln+=: created concat buffer #" << _line->address()
          << std::endl;
      }
#endif
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
#ifdef _DEBUG
      if (_Line_debug) {
        std::cout << "cs+=: created concat buffer #" << _line->address()
          << std::endl;
      }
#endif
    } else {
      *_line += line;
    }
    return *this;
  }
  const Line& erase(unsigned int pos = 0) {
    if (_line->_instances > 1) {
      _line->_instances--;
      LineBuffer* line = _line;
      if (_deleted_line != NULL) {
        _line         = _deleted_line;
        _deleted_line = NULL;
#ifdef _DEBUG
        if (_Line_debug) {
          std::cout << "eras: re-used buffer #" << _line->address()
            << std::endl;
        }
#endif
        _line->set(*line, pos);
      } else {
        _line  = new LineBuffer();
        _line->set(*line, pos);
#ifdef _DEBUG
        if (_Line_debug) {
          std::cout << "eras: created buffer #" << _line->address()
            << std::endl;
        }
#endif
      }
      _line->_instances++;
    } else {
      _line->erase(pos);
    }
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
  int rfind(char c, unsigned int pos = 0) const {
    const char* buffer = *_line;
    const char* r = &buffer[_line->size()];
    while (--r >= &buffer[pos]) {
      if (*r == c) {
        break;
      }
    }
    return r >= &buffer[pos] ? r - buffer : -1;
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
  LineBuffer& instance() {
    if (_line->_instances > 1) {
      _line->_instances--;
      LineBuffer* line = _line;
      _line  = new LineBuffer();
      _line->set(*line);
      _line->_instances++;
#ifdef _DEBUG
      if (_Line_debug) {
        std::cout << "inst: created buffer #" << _line->address() << std::endl;
      }
#endif
    }
    return *_line;
  }
  unsigned int instances() {
    return _line->_instances;
  }
};

}

#endif
