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

int hbackup::pathCompare(const char* s1, const char* s2, size_t length) {
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

const StrPath& StrPath::toUnix() {
  char* pos = &(*this)[this->length()];
  while (--pos >= &(*this)[0]) {
    if (*pos == '\\') {
      *pos = '/';
    }
  }
  if (((*this)[1] == ':') && ((*this)[2] == '/')
   && ((*this)[0] >= 'a') && ((*this)[0] <= 'z')) {
    (*this)[0] -= 0x20;
  }
  return *this;
}

const StrPath& StrPath::noEndingSlash() {
  size_t pos = this->length();
  while ((pos > 0) && ((*this)[pos - 1] == '/')) {
    pos--;
  }
  *this = this->substr(0, pos);
  return *this;
}

const StrPath StrPath::basename() const {
  size_t pos = this->find_last_of('/');
  if (pos != string::npos) {
    return StrPath(this->substr(++pos));
  } else {
    return StrPath(*this);
  }
}

const StrPath StrPath::dirname() const {
  size_t pos = this->find_last_of('/');
  if (pos != string::npos) {
    return StrPath(this->substr(0, pos));
  } else {
    return StrPath(".");
  }
}

int StrPath::countChar(char c) const {
  int occurences = 0;
  size_t length = this->size();
  const char *reader = this->c_str();
  while (length-- > 0) {
    if (*reader++ == c) {
      occurences++;
    }
  }
  return occurences;
}
