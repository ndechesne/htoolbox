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

#ifdef _DEBUG
#include <iostream>
#endif

#include <cstdlib>
#include <cstring>

namespace hbackup {

#ifdef _DEBUG
extern bool         __Line_debug;
#endif
extern unsigned int __Line_address;

class LineBuffer {
  static const unsigned int  _min_capacity = 128;
  char*             _line;
  unsigned int      _capacity;
  unsigned int      _size;
  int grow(unsigned int required, bool discard = false);
  LineBuffer(const LineBuffer&);
  LineBuffer& operator=(const LineBuffer&);
public:
  // The no of instances and destruction are managed by this class' users
  unsigned int      _instances;
  // For debug purposes only
  unsigned int      _address;
  LineBuffer() {
    _line      = NULL;
    _capacity  = 0;
    _size      = 0;
    _instances = 0;
    _address   = ++__Line_address;
  }
  ~LineBuffer() {
    free(_line);
  }
  operator const char* () const {
    return _line;
  }
  const char* c_str() const {
    return _line;
  }
  char* str() {
    return _line;
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
  LineBuffer& set(const LineBuffer& line, const LineBuffer& line2) {
    _size = line._size + line2._size;
    grow(_size + 1, true);
    memcpy(_line, line._line, line._size);
    memcpy(&_line[line._size], line2._line, line2._size + 1);
    return *this;
  }
  LineBuffer& set(const LineBuffer& line, const char* line2) {
    unsigned int size2 = strlen(line2);
    _size = line._size + size2;
    grow(_size + 1, true);
    memcpy(_line, line._line, line._size);
    memcpy(&_line[line._size], line2, size2 + 1);
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
protected:
  LineBuffer*       _line;
private:
  LineBuffer*       _deleted_line;
  int getBuffer() {
    if (_deleted_line != NULL) {
      _line         = _deleted_line;
      _deleted_line = NULL;
      return 1;
    } else {
      _line  = new LineBuffer;
      return 0;
    }
  }
  void disposeBuffer() {
    --_line->_instances;
    if (_line->_instances == 0) {
      if (_deleted_line == NULL) {
        _deleted_line = _line;
#ifdef _DEBUG
        if (__Line_debug) {
          std::cout << "disp: saved  ";
          show(_deleted_line);
        }
#endif
      } else
      if (_deleted_line->capacity() >= _line->capacity()) {
#ifdef _DEBUG
        if (__Line_debug) {
          std::cout << "disp: rep buf";
          show(_line);
        }
#endif
        delete _line;
      } else {
#ifdef _DEBUG
        if (__Line_debug) {
          std::cout << "disp: rep ext";
          show(_deleted_line);
        }
#endif
        delete _deleted_line;
        _deleted_line = _line;
#ifdef _DEBUG
        if (__Line_debug) {
          std::cout << " ->";
          show(_deleted_line);
        }
#endif
      }
    }
  }
  LineBuffer& instance() {
    if (_line->_instances > 1) {
      _line->_instances--;
      LineBuffer* temp = _line;
#ifdef _DEBUG
      int re =
#endif
      getBuffer();
      _line->set(*temp);
      _line->_instances++;
#ifdef _DEBUG
      if (__Line_debug) {
        if (re) {
          std::cout << "inst: re-used";
        } else {
          std::cout << "inst: created";
        }
        show(_line);
      }
#endif
    }
    return *_line;
  }
public:
  Line(const char* line = "") {
    _deleted_line = NULL;
    getBuffer();
    _line->set(line);
    _line->_instances++;
#ifdef _DEBUG
    if (__Line_debug) {
      std::cout << "ctor: created";
      show(_line);
    }
#endif
  }
  Line(const Line& line) {
    _deleted_line = NULL;
    _line         = line._line;
    _line->_instances++;
#ifdef _DEBUG
    if (__Line_debug) {
      std::cout << "ctor: copied ";
      show(_line);
    }
#endif
  }
  ~Line() {
    if (--_line->_instances == 0) {
#ifdef _DEBUG
      if (__Line_debug) {
        std::cout << "dtor: del buf";
        show(_line);
      }
#endif
      delete _line;
    }
    if ((_deleted_line != NULL) && (_deleted_line->_instances == 0)) {
#ifdef _DEBUG
      if (__Line_debug) {
        std::cout << "dtor: del ext";
        show(_deleted_line);
      }
#endif
      delete _deleted_line;
    }
  }
  operator const char* () const {
    return *_line;
  }
  const char* c_str() const {
    return _line->c_str();
  }
  char* buffer() {
    return instance().str();
  }
  operator LineBuffer& () {
    return instance();
  }
  unsigned int size() const {
    return _line->size();
  }
  Line& operator=(const Line& line) {
    disposeBuffer();
    _line = line._line;
    _line->_instances++;
#ifdef _DEBUG
    if (__Line_debug) {
      std::cout << "ln= : copied ";
      show(_line);
    }
#endif
    return *this;
  }
  Line& operator=(const char* line) {
    if (_line->_instances > 1) {
      _line->_instances--;
#ifdef _DEBUG
      int re =
#endif
      getBuffer();
      _line->set(line);
      _line->_instances++;
#ifdef _DEBUG
      if (__Line_debug) {
        if (re) {
          std::cout << "cs= : re-used";
        } else {
          std::cout << "cs= : created";
        }
        show(_line);
      }
#endif
    } else {
      _line->set(line);
#ifdef _DEBUG
      if (__Line_debug) {
        std::cout << "cs= : updated";
        show(_line);
      }
#endif
    }
    return *this;
  }
  Line& operator+=(const Line& line) {
    if (_line->_instances > 1) {
      _line->_instances--;
      LineBuffer* temp = _line;
#ifdef _DEBUG
      int re =
#endif
      getBuffer();
      _line->set(*temp, *line._line);
      _line->_instances++;
#ifdef _DEBUG
      if (__Line_debug) {
        if (re) {
          std::cout << "ln+=: re-used";
        } else {
          std::cout << "ln+=: created";
        }
        show(_line);
      }
#endif
    } else {
      *_line += *line._line;
#ifdef _DEBUG
      if (__Line_debug) {
        std::cout << "ln+=: updated";
        show(_line);
      }
#endif
    }
    return *this;
  }
  Line& operator+=(const char* line) {
    if (_line->_instances > 1) {
      _line->_instances--;
      LineBuffer* temp = _line;
#ifdef _DEBUG
      int re =
#endif
      getBuffer();
      _line->set(*temp, line);
      _line->_instances++;
#ifdef _DEBUG
      if (__Line_debug) {
        if (re) {
          std::cout << "cs+=: re-used";
        } else {
          std::cout << "cs+=: created";
        }
        show(_line);
      }
#endif
    } else {
      *_line += line;
#ifdef _DEBUG
      if (__Line_debug) {
        std::cout << "cs+=: updated";
        show(_line);
      }
#endif
    }
    return *this;
  }
  void swap(Line& other) {
    LineBuffer* temp = _line;
    _line = other._line;
    other._line = temp;
  }
  const Line& erase(unsigned int pos = 0) {
    if (_line->_instances > 1) {
      _line->_instances--;
      LineBuffer* temp = _line;
#ifdef _DEBUG
      int re =
#endif
      getBuffer();
      _line->set(*temp, pos);
      _line->_instances++;
#ifdef _DEBUG
      if (__Line_debug) {
        if (re) {
          std::cout << "eras: re-used";
        } else {
          std::cout << "eras: created";
        }
        show(_line);
      }
#endif
    } else {
      _line->erase(pos);
#ifdef _DEBUG
      if (__Line_debug) {
        std::cout << "eras: updated";
        show(_line);
      }
#endif
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
  unsigned int instances() {
    return _line->_instances;
  }
  void show(const LineBuffer* line = NULL) {
#ifdef _DEBUG
    if (line == NULL) {
      line = _line;
    }
    std::cout
      << " bf " << line->_address
      << " in " << line->_instances
      << " cp " << line->capacity()
      << " sz " << line->size()
      << " '"   << line->c_str() << "'"
      << std::endl;
#else
    (void)line;
#endif
  }
};

}

#endif
