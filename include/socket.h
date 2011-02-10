/*
    Copyright (C) 2010 - 2011  Hervé Fache

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _SOCKET_H
#define _SOCKET_H

#include <ireaderwriter.h>

namespace htoolbox {

class Socket : public IReaderWriter {
  struct          Private;
  Private* const  _d;
  const Socket& operator=(const Socket&);
public:
  // Internet socket
  Socket(
    const char* hostname,
    int         port);
  // Unix socket
  Socket(
    const char* name,
    bool        abstract = false);  // Abstract name is Linux-specific
  Socket(const Socket&);
  ~Socket();
  int listen(int backlog);
  int release();
  int setReadTimeout(int seconds, int microseconds = 0);
  int setWriteTimeout(int seconds, int microseconds = 0);
  int open();
  int close();
  // stream returns whatever what was read, not necessarily max_size
  ssize_t read(void* buffer, size_t max_size);
  ssize_t get(void* buffer, size_t size);
  ssize_t put(const void* buffer, size_t size);
  const char* path() const;
  static int getAddress(const char* hostname, uint32_t* address);
};

};

#endif // _SOCKET_H
