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

#ifndef BUFFER_H
#define BUFFER_H

#include <unistd.h>

namespace hbackup {

class Buffer {
  struct            Private;
  Private* const    _d;
public:
  Buffer(size_t size = 0);
  ~Buffer();
  // Management
  void create(size_t size = 102400);
  void destroy();
  void empty();
  bool exists() const;
  size_t capacity() const;
  size_t usage() const;
  // Status
  bool isEmpty() const;
  bool isFull() const;
  // Writing
  char* writer();
  size_t writeable() const;
  void written(size_t size);
  ssize_t write(const void* buffer, size_t size);
  // Reading
  const char* reader() const;
  size_t readable() const;
  void readn(size_t size);
  ssize_t read(void* buffer, size_t size);
};

}

#endif
