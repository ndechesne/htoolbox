/*
     Copyright (C) 2010  Herve Fache

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

#ifndef _UNIX_SOCKET_H
#define _UNIX_SOCKET_H

#include <ireaderwriter.h>

namespace htools {

class UnixSocket : public IReaderWriter {
  struct          Private;
  Private* const  _d;
  const UnixSocket& operator=(const UnixSocket&);
public:
  UnixSocket(const char* path);
  UnixSocket(const UnixSocket&);
  ~UnixSocket();
  int listen(int backlog);
  int open();
  int close();
  // WARNING: read returns whatever what was read, not necessarily size
  ssize_t read(void* message, size_t length);
  ssize_t write(const void* message, size_t length);
  const char* path() const;
};

};

#endif // _UNIX_SOCKET_H
