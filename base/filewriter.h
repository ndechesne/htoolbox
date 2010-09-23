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

#ifndef _FILEWRITER_H
#define _FILEWRITER_H

#include <iwriter.h>

namespace hbackup {

class FileWriter : public IWriter {
  struct         Private;
  Private* const _d;
public:
  FileWriter(const char* path);
  ~FileWriter();
  int open();
  int close();
  ssize_t write(const void* buffer, size_t size);
};

};

#endif // _FILEWRITER_H
