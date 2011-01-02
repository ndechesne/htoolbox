/*
     Copyright (C) 2010-2011  Herve Fache

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
  long long _offset;
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
  //! \brief Always fail to read, as this is a writer
  ssize_t read(void*, size_t) {
    return -1;
  }
  ssize_t write(const void*, size_t size) {
    _offset += size;
    return size;
  }
  //! \brief Always return empty string
  const char* path() const {
    return "";
  };
  long long offset() const {
    return _offset;
  };
};

};

#endif // _NULLWRITER_H
