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

#ifndef _STACKHELPER_H
#define _STACKHELPER_H

#include <ifilereaderwriter.h>

namespace htools {

class StackHelper : public IFileReaderWriter {
  IReaderWriter*     _child;
  bool               _delete_child;
  IFileReaderWriter* _fd;
public:
  StackHelper(IReaderWriter* child, bool delete_child, IFileReaderWriter* fd) :
    _child(child), _delete_child(delete_child), _fd(fd) {}
  ~StackHelper() {
    if (_delete_child)
      delete _child;
  }
  int open() {
    return _child->open();
  }
  int close() {
    return _child->close();
  }
  ssize_t read(void* buffer, size_t size) {
    return _child->read(buffer, size);
  }
  ssize_t write(const void* buffer, size_t size) {
    return _child->write(buffer, size);
  }
  // File path
  const char* path() const {
    return _fd->path();
  }
  // Offset of on-disk file
  long long offset() const {
    return _fd->offset();
  }
};

};

#endif // _STACKHELPER_H
