/*
     Copyright (C) 2006-2007  Herve Fache

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

#ifndef STRINGS_H
#define STRINGS_H

namespace hbackup {

extern int pathCompare(const char* s1, const char* s2, size_t length = -1);

class StrPath : public string {
public:
  using string::operator=;
  StrPath(const string& string) {
    *this = string;
  }
  StrPath(const char* dir = NULL, const char* name = NULL);
  int compare(const char* string, size_t length = -1) const {
    return pathCompare(this->c_str(), string, length);
  }
  int compare(const string& string, size_t length = -1) const {
    return pathCompare(this->c_str(), string.c_str(), length);
  }
  int compare(const StrPath& string, size_t length = -1) const {
    return pathCompare(this->c_str(), string.c_str(), length);
  }
  bool    operator<(const char* string) const {
    return compare(string) < 0;
  }
  bool    operator>(const char* string) const {
    return compare(string) > 0;
  }
  bool    operator<=(const char* string) const {
    return compare(string) <= 0;
  }
  bool    operator>=(const char* string) const {
    return compare(string) >= 0;
  }
  bool    operator==(const char* string) const {
    return compare(string) == 0;
  }
  bool    operator!=(const char* string) const {
    return compare(string) != 0;
  }
  bool    operator<(const string& string) const {
    return compare(string) < 0;
  }
  bool    operator>(const string& string) const {
    return compare(string) > 0;
  }
  bool    operator<=(const string& string) const {
    return compare(string) <= 0;
  }
  bool    operator>=(const string& string) const {
    return compare(string) >= 0;
  }
  bool    operator==(const string& string) const {
    return compare(string) == 0;
  }
  bool    operator!=(const string& string) const {
    return compare(string) != 0;
  }
  bool    operator<(const StrPath& string) const {
    return compare(string) < 0;
  }
  bool    operator>(const StrPath& string) const {
    return compare(string) > 0;
  }
  bool    operator<=(const StrPath& string) const {
    return compare(string) <= 0;
  }
  bool    operator>=(const StrPath& string) const {
    return compare(string) >= 0;
  }
  bool    operator==(const StrPath& string) const {
    return compare(string) == 0;
  }
  bool    operator!=(const StrPath& string) const {
    return compare(string) != 0;
  }
  const StrPath& toUnix();
  const StrPath& noEndingSlash();
  const StrPath  basename() const;
  const StrPath  dirname()  const;
  int            countChar(char c) const;
  int            countBlocks(char c) const;
};

}

#endif
