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

#ifndef _COMPDATA_H
#define _COMPDATA_H

namespace hbackup {

class CompData {
  char*             _checksum;
  long long         _size;
  bool              _compressed;
  CompData();
  CompData& operator=(const CompData& c);
public:
  CompData(const CompData& c) :
      _checksum(strdup(c._checksum)), _size(c._size),
      _compressed(c._compressed) {}
  CompData(const char* c, long long s, bool z = false) :
      _checksum(strdup(c)), _size(s), _compressed(z) {}
  ~CompData() { free(_checksum); }
  void signalBroken() { _size = -1; }
  inline const char* checksum() const { return _checksum; }
  inline long long size() const       { return _size; }
  inline bool compressed() const      { return _compressed; }
  bool operator<(const CompData& d)  {
    int cmp = strcmp(_checksum, d._checksum);
    if (cmp != 0) return cmp < 0;
    return _checksum < d._checksum;
  }
};

}

#endif
