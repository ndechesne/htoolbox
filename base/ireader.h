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

#ifndef _IREADER_H
#define _IREADER_H

#include <unistd.h>

namespace hbackup {

class IReader {
public:
  virtual int open() = 0;
  virtual int close() = 0;
  virtual ssize_t read(void* buffer, size_t size) = 0;
  // Offset of on-disk file
  virtual long long offset() const = 0;
};

};

#endif // _IREADER_H