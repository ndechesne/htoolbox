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

#ifndef _IREADERWRITER_H
#define _IREADERWRITER_H

#include <unistd.h>

namespace htools {

class IReaderWriter {
public:
  virtual ~IReaderWriter() {}
  virtual int open() = 0;
  virtual int close() = 0;
  virtual ssize_t read(void* buffer, size_t size) = 0;
  virtual ssize_t write(const void* buffer, size_t size) = 0;
};

};

#endif // _IREADERWRITER_H
