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

#ifndef _UNZIPREADER_H
#define _UNZIPREADER_H

#include <ireaderwriter.h>

namespace htools {

class UnzipReader : public IReaderWriter {
  struct         Private;
  Private* const _d;
public:
  UnzipReader(IReaderWriter* child, bool delete_child);
  ~UnzipReader();
  int open();
  int close();
  ssize_t read(void* buffer, size_t size);
  ssize_t write(const void* buffer, size_t size);
};

};

#endif // _UNZIPREADER_H
