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

#ifndef _MISSING_H
#define _MISSING_H

namespace hbackup {

class Missing {
  struct            Private;
  Private* const    _d;
public:
  Missing();
  ~Missing();
  void setProgressCallback(progress_f progress);
  void open(
    const char* path);
  int load();
  int close();
  bool empty() const;
  unsigned int size() const;
  void push_back(const char* checksum);
  string operator[](unsigned int id);
  int search(const char* checksum);
  void setRecovered(unsigned int id);
  void show() const;
};

}

#endif
