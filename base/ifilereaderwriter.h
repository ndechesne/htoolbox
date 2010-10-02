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

#ifndef _IFILEREADERWRITER_H
#define _IFILEREADERWRITER_H

#include <ireaderwriter.h>

namespace htools {

class IFileReaderWriter : public IReaderWriter {
public:
  virtual ~IFileReaderWriter() {}
  virtual int open() = 0;
  virtual int close() = 0;
  // Read size bytes exactly into buffer, no less until end of file
  virtual ssize_t read(void* buffer, size_t size) = 0;
  // Write size bytes exactly from buffer, no less
  virtual ssize_t write(const void* buffer, size_t size) = 0;
  // File path
  virtual const char* path() const = 0;
  // Offset of on-disk file
  virtual long long offset() const = 0;
};

};

#endif // _IFILEREADERWRITER_H
