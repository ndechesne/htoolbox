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

#include <iostream>
#include <string>

using namespace std;

#include "strings.h"

using namespace hbackup;

StrPath::StrPath(const char* dir, const char* name) {
  if (dir == NULL) {
    *this = "";
  } else
  if ((name == NULL) || (name[0] == '\0')) {
    *this = dir;
  } else
  if (dir[0] == '\0') {
    *this = name;
  } else
  {
    *this = dir;
    *this += "/";
    *this += name;
  }
}

int StrPath::compare(const char* string, size_t length) const {
  const char* s1 = this->c_str();
  const char* s2 = string;
  while (true) {
    if (length == 0) {
      return 0;
    }
    if (*s1 == '\0') {
      if (*s2 == '\0') {
        return 0;
      } else {
        return -1;
      }
    } else {
      if (*s2 == '\0') {
        return 1;
      } else {
        if (*s1 == '/') {
          if (*s2 == '/') {
            s1++;
            s2++;
          } else {
            if (*s2 < ' ') {
              return 1;
            } else {
              return -1;
            }
          }
        } else {
          if (*s2 == '/') {
            if (*s1 < ' ') {
              return -1;
            } else {
              return 1;
            }
          } else {
            if (*s1 < *s2) {
              return -1;
            } else if (*s1 > *s2) {
              return 1;
            } else {
              s1++;
              s2++;
            }
          }
        }
      }
    }
    if (length > 0) {
      length--;
    }
  }
}

const StrPath& StrPath::toUnix() {
  char* pos = &(*this)[this->length()];
  while (--pos >= &(*this)[0]) {
    if (*pos == '\\') {
      *pos = '/';
    }
  }
  return *this;
}

const StrPath& StrPath::noEndingSlash() {
  char* pos = &(*this)[this->length()];
  while ((--pos >= &(*this)[0]) && (*pos == '/')) {
    *pos = '\0';
  }
  return *this;
}

const StrPath StrPath::basename() {
  size_t pos = this->find_last_of('/');
  if (pos != string::npos) {
    return StrPath(this->substr(++pos));
  } else {
    return StrPath(*this);
  }
}

const StrPath StrPath::dirname() {
  size_t pos = this->find_last_of('/');
  if (pos != string::npos) {
    return StrPath(this->substr(0, pos));
  } else {
    return StrPath(".");
  }
}
