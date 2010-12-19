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
  char              _hash[129];
  long long         _data_size;
  long long         _file_size;
  CompData();
  CompData& operator=(const CompData& c);
public:
  CompData(const CompData& c)
    : _data_size(c._data_size), _file_size(c._file_size) {
    strcpy(_hash, c._hash);
  }
  CompData(const char* h, long long d, long long f = -1)
    : _data_size(d), _file_size(f) {
    strcpy(_hash, h);
  }
  void signalBroken()               { _data_size = -1; }
  bool isBroken() const             { return _data_size == -1; }
  inline const char* hash() const   { return _hash; }
  inline long long size() const     { return _data_size; }
  inline long long fileSize() const { return _file_size; }
  bool operator<(const CompData& d) {
    int cmp = strcmp(_hash, d._hash);
    if (cmp != 0) return cmp < 0;
    return _hash < d._hash;
  }
};

}

#endif
