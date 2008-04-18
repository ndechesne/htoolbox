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

#ifndef LINE_H
#define LINE_H

#define HAVE_LINE_H 1

namespace hbackup {

class Line {
  int               _capacity;
  int               _size;
  char*             _buffer;
public:
  // Big four
  Line(const char* line = NULL);
  Line(const Line& line);
  ~Line();
  const Line& operator=(const Line& line);
  // Other methods
  const Line& operator=(const char* line);
  const char& operator[](int pos) const { return _buffer[pos] ; }
  operator const char* () const         { return _buffer; }
  int size() const                      { return _size; }
  const Line& erase(int pos = 0);
  int find(char c, int pos = 0) const;
  const Line& append(const Line& line, int pos = -1, int num = -1);
  const Line& append(const char* line, int pos = -1, int num = -1);
  // For getLine operations
  char** bufferPtr()                    { return &_buffer; }
  int* capacityPtr()                    { return &_capacity; }
  int* sizePtr()                        { return &_size; }
};

}

#endif
