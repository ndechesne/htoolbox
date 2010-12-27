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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "unix_socket.h"

using namespace htools;

struct UnixSocket::Private {
  char* path;
  int fd;
  bool open;
  bool server;
  struct sockaddr_un addr;
  socklen_t addr_len;
};

UnixSocket::UnixSocket(const char* path, bool server) : _d(new Private) {
  _d->path = strdup(path);
  _d->fd = -1;
  _d->open = false;
  _d->server = server;
  memset(&_d->addr, 0, sizeof(struct sockaddr_un));
  _d->addr.sun_family = AF_UNIX;
  strcpy(_d->addr.sun_path, _d->path);
  _d->addr_len = strlen(_d->addr.sun_path) + sizeof(_d->addr.sun_family);
}

UnixSocket::~UnixSocket() {
  if (_d->open) {
    close();
  }
  free(_d->path);
  delete _d;
}

int UnixSocket::open() {
  if ((_d->fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
    return -1;
  }
  if (_d->server) {
    unlink(_d->path);
    if (bind(_d->fd, reinterpret_cast<struct sockaddr*>(&_d->addr),
        strlen(_d->addr.sun_path) + sizeof(_d->addr.sun_family)) < 0) {
      return -1;
    }
  }
  _d->open = true;
  return 0;
}

int UnixSocket::close() {
  _d->open = false;
  int rc = ::close(_d->fd);
  if (_d->server) {
    unlink(_d->path);
  }
  return rc;
}

ssize_t UnixSocket::read(void* message, size_t length) {
  struct sockaddr *addr;
  socklen_t addr_len;
  if (_d->server) {
    addr = NULL;
    addr_len = 0;
  } else {
    addr = reinterpret_cast<struct sockaddr*>(&_d->addr);
    addr_len = _d->addr_len;
  }
  return recvfrom(_d->fd, message, length, MSG_WAITALL, addr, &addr_len);
}

ssize_t UnixSocket::write(const void* message, size_t length) {
  const struct sockaddr *addr;
  socklen_t addr_len;
  if (_d->server) {
    addr = NULL;
    addr_len = 0;
  } else {
    addr = reinterpret_cast<struct sockaddr*>(&_d->addr);
    addr_len = _d->addr_len;
  }
  return sendto(_d->fd, message, length, MSG_NOSIGNAL, addr, addr_len);
}

const char* UnixSocket::path() const {
  return _d->path;
}
