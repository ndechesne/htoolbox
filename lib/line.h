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
  unsigned int      _capacity;
  unsigned int      _size;
  char*             _buffer;
  unsigned int      _add_size;
public:
  // Big four
  Line(unsigned int init_size = 100, unsigned int add_size = 10);
  Line(const char* line, unsigned int add_size = 0);
  Line(const Line& line);
  ~Line();
  const Line& operator=(const Line& line);
  // Other methods
  const Line& operator=(const char* line);
  const char& operator[](int pos) const { return _buffer[pos] ; }
  operator const char* () const         { return _buffer; }
  int resize(unsigned int new_size = 100, int new_add_size = -1,
    bool discard = false);
  unsigned int size() const             { return _size; }
  const Line& erase(int pos = 0);
  int find(char c, int pos = 0) const;
  const Line& append(const Line& line, int pos = -1, int num = -1);
  const Line& append(const char* line, int pos = -1, int num = -1);
  const Line& operator+=(const Line& line) { return append(line); }
  const Line& operator+=(const char* line) { return append(line); }
  // For getLine operations
  char** bufferPtr()                    { return &_buffer; }
  unsigned int* capacityPtr()           { return &_capacity; }
  unsigned int* sizePtr()               { return &_size; }
  // For debug
  unsigned int capacity() const         { return _capacity; }
  unsigned int add_size() const         { return _add_size; }
};

}

#endif
