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

#ifndef _INET_SOCKET_H
#define _INET_SOCKET_H

#include <ireaderwriter.h>

namespace htools {

class InetSocket : public IReaderWriter {
  struct          Private;
  Private* const  _d;
  const InetSocket& operator=(const InetSocket&);
public:
  InetSocket(int port, const char* hostname = "localhost");
  InetSocket(const InetSocket&);
  ~InetSocket();
  int listen(int backlog);
  int open();
  int close();
  ssize_t read(void* message, size_t length);
  ssize_t write(const void* message, size_t length);
  static int getAddress(const char* hostname, uint32_t* address);
};

};

#endif // _INET_SOCKET_H
