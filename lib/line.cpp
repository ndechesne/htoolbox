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

#include <stdlib.h>
#include <string.h>

using namespace std;

#include "line.h"

using namespace hbackup;

int LineBuffer::grow(unsigned int required) {
  unsigned int new_capacity;
  if (_capacity != 0) {
    new_capacity = _capacity;
  } else {
    new_capacity = _min_capacity;
  }
  while (new_capacity < required) {
    // No check for absolute maximum
    new_capacity <<= 1;
  }
  if (new_capacity > _capacity) {
    _capacity = new_capacity;
    char* new_buffer = static_cast<char*>(realloc(_line, _capacity));
    if (new_buffer != NULL) {
      _line = new_buffer;
    } else {
      return -1;
    }
  }
  return 0;
}
