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

#ifndef _NULLWRITER_H
#define _NULLWRITER_H

#include <ireaderwriter.h>

namespace htoolbox {

//! \brief Null writer
/*!
 * Opening, closing and writing always succeeds but does nothing.
 *
 * Offset is valid.
 */
class NullWriter : public IReaderWriter {
  int64_t _offset;
public:
  NullWriter() {}
  ~NullWriter() {}
  int open() {
    _offset = 0;
    return 0;
  }
  int close() {
    return 0;
  }
  //! \brief Always fails to read, as this is a writer
  ssize_t read(void*, size_t) {
    return -1;
  }
  //! \brief Always fails to get, as this is a writer
  ssize_t get(void*, size_t) {
    return -1;
  }
  ssize_t put(const void*, size_t size) {
    _offset += size;
    return size;
  }
  //! \brief Always return empty string
  const char* path() const {
    return "";
  };
  int64_t offset() const {
    return _offset;
  };
};

};

#endif // _NULLWRITER_H
