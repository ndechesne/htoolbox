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

#ifndef _COPIER_H
#define _COPIER_H

#include <ireaderwriter.h>

namespace htoolbox {

class Copier : public IReaderWriter {
  struct          Private;
  Private* const  _d;
  Copier(const Copier&);
  const Copier& operator=(const Copier&);
public:
  Copier(IReaderWriter* source, bool delete_source, size_t chunk_size);
  ~Copier();
  void addDest(IReaderWriter* dest, bool delete_dest);
  int open();
  int close();
  // Copies as it reads. Chunk size copy if size = 0 or size > chunk size
  ssize_t stream(void* buffer = NULL, size_t max_size = 0);
  // Copies as it reads. Full copy if size = 0
  ssize_t read(void* buffer = NULL, size_t size = 0);
  // Inserts data into destination
  ssize_t write(const void*, size_t);
};

};

#endif // _COPIER_H