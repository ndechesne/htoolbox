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
  // Open missing list
  void open(const char* path);
  // Load list from disk
  int load();
  // Close list and save it to disk
  int close();
  // Get size of list
  unsigned int size() const;
  // Get checksum at row id
  const string& operator[](unsigned int id) const;
  // Get row of given checksum
  int search(const char* checksum) const;
  // Mark checksum at row id as recovered
  void setRecovered(unsigned int id);
  // Add missing checksum at end of list
  void setMissing(const char* checksum);
  // Add inconsistent checksum at end of list
  void setInconsistent(const char* checksum);
  // Show list of problematic checksums
  void show() const;
};

}

#endif
