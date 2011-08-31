/*
     Copyright (C) 2011  Hervé Fache

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

#ifndef _SHARED_PATH_H
#define _SHARED_PATH_H

#include <stdio.h>
#include <string.h>

namespace htoolbox {

// The class T must implement the following operator for this to work:
//   operator const char*() const { return hash; }

//! \brief Class to easily re-use an existing path string.
/*!
  This class implements a way to share a path string, typically defined as
  char path[PATH_MAX], without risk of losing its original value.

  Example:
  char path[PATH_MAX] = "/my/path";               // path = /my/path
  {
    SharedPath spath(path, strlen(path), "ends"); // path = /my/path/ends
    spath.accept();                               // path = /my/path/ends
    spath.append("here");                         // path = /my/path/ends/here
    spath.append(10);                             // path = /my/path/ends-10
  }
  // blah                                         // path = /my/path/ends

*/
class SharedPath {
  char*             _path;
  size_t            _length;
  size_t            _total_length;
public:
  //! \brief Construct appending string.
  /*!
   \param p             original path
   \param l             original path length
   \param p             path to append
  */
  SharedPath(char* p, size_t l, const char* a) : _path(p), _length(l) {
    append(a);
  }
  //! \brief Construct appending index.
  /*!
   \param p             original path
   \param l             original path length
   \param p             index to append
  */
  SharedPath(char* p, size_t l, int v) : _path(p), _length(l) {
    append(v);
  }
  //! \brief Destroy and reset to recorded length.
  ~SharedPath() {
    reset();
  }
  //! \brief Append string.
  /*!
   \param append        path to append
  */
  void append(const char* append) {
    _path[_length] = '/';
    strcpy(&_path[_length + 1], append);
    _total_length = _length + strlen(append) + 1;
  }
  //! \brief Append index.
  /*!
   \param value         index to append
  */
  void append(int value) {
    _path[_length] = '-';
    _total_length = _length + sprintf(&_path[_length + 1], "%d", value) + 1;
  }
  //! \brief Reset to recorded length.
  void reset() {
    _path[_length] = '\0';
    _total_length = _length;
  }
  //! \brief Change recorded length to current.
  void accept() {
    _length = _total_length;
  }
  //! \brief Return current length.
  /*!
    \return             current length
  */
  size_t length() const {
    return _total_length;
  }
};

}

#endif // _SHARED_PATH_H
