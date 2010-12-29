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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/un.h>

#include "report.h"
#include "unix_socket.h"

using namespace htools;

struct ServerData {
  char*             path;
  int               listen_socket;
  ServerData() : path(NULL) {}
  ~ServerData() { free(path); }
};

struct UnixSocket::Private {
  const ServerData* master_data;
  ServerData*       data;
  int               conn_socket;
};

UnixSocket::UnixSocket(const char* path) : _d(new Private) {
  _d->data = new ServerData;
  _d->master_data = _d->data;
  _d->data->path = strdup(path);
  _d->data->listen_socket = -1;
  _d->conn_socket = -1;
}

UnixSocket::UnixSocket(const UnixSocket& s) : IReaderWriter(), _d(new Private) {
  _d->master_data = s._d->master_data;
  _d->data = NULL;
  _d->conn_socket = -1;
}

UnixSocket::~UnixSocket() {
  if ((_d->data != NULL) && (_d->data->listen_socket >= 0)) {
    ::close(_d->data->listen_socket);
  }
  delete _d;
}

int UnixSocket::listen(int backlog) {
  if (_d->data == NULL) {
    hlog_error("only the original object can listen");
    return -1;
  }
  if (_d->data->listen_socket != -1) {
    hlog_error("socket already open");
    return -1;
  }
  /* Create server socket */
  struct sockaddr_un sock;
  sock.sun_family = AF_UNIX;
  strcpy(sock.sun_path, _d->data->path);
  /* We want quick restarts */
  _d->data->listen_socket = ::socket(sock.sun_family, SOCK_STREAM, 0);
  if (_d->data->listen_socket == -1) {
    hlog_error("%s creating socket", strerror(errno));
    return -1;
  }
  int re_use = 1;
  ::setsockopt(_d->data->listen_socket, SOL_SOCKET, SO_REUSEADDR, &re_use,
    sizeof(int));
  /* Use default values to bind and listen */
  if (::bind(_d->data->listen_socket, reinterpret_cast<struct sockaddr*>(&sock),
      sizeof(struct sockaddr_un))) {
    hlog_error("%s binding socket", strerror(errno));
    return -1;
  }
  if (::listen(_d->data->listen_socket, backlog)) {
    hlog_error("%s listening", strerror(errno));
    return -1;
  }
  hlog_regression("fd = %d", _d->data->listen_socket);
  return _d->data->listen_socket;
}

int UnixSocket::open() {
  if (_d->conn_socket != -1) {
    hlog_error("connection already open");
    return -1;
  }
  if ((_d->data != NULL) && (_d->data->listen_socket == -1)) {
    /* Create client socket */
    struct sockaddr_un sock;
    sock.sun_family = AF_UNIX;
    strcpy(sock.sun_path, _d->data->path);
    _d->conn_socket = ::socket(sock.sun_family, SOCK_STREAM, 0);
    if (::connect(_d->conn_socket, reinterpret_cast<struct sockaddr*>(&sock),
        sizeof(struct sockaddr_un)) < 0) {
      hlog_error("%s connecting", strerror(errno));
      return -1;
    }
  } else {
    // Server
    _d->conn_socket = ::accept(_d->master_data->listen_socket, 0, 0);
  }
  return _d->conn_socket;
}

int UnixSocket::close() {
  int rc = -1;
  if (_d->conn_socket == -1) {
    hlog_error("connection not open");
  }
  rc = ::close(_d->conn_socket);
  _d->conn_socket = -1;
  if (rc < 0) {
    hlog_warning("%s", strerror(errno));
  }
  return rc;
}

ssize_t UnixSocket::write(
    const void*     data,
    size_t          length) {
  const char* buffer = static_cast<const char*>(data);
  ssize_t sent = 0;
  do {
    ssize_t size;
    bool conclusive = false;
    do {
      size = ::send(_d->conn_socket, &buffer[sent], length, MSG_NOSIGNAL);
      if (size < 0) {
        switch (errno) {
          case EAGAIN:
            // Socket full, wait and try again
            usleep(1000);
            // DO NOT INSERT A BREAK HERE, YOU NAIVE FOOL!
          case EINTR:
            hlog_warning("%s", strerror(errno));
            break;
          default:
            conclusive = true;
        }
      } else {
        conclusive = true;
      }
    } while (! conclusive);
    switch (size) {
      case -1:
        hlog_error("%s sending", strerror(errno));
        return -1;
      case 0:
        hlog_error("socket closed");
        return 0;
      default:
        hlog_regression("sent (partial) %u", size);
        sent += size;
    }
  } while (sent < static_cast<ssize_t>(length));
  if (sent > 0) {
    hlog_debug("sent (total) %u", sent);
  }
  return sent;
}

ssize_t UnixSocket::read(
    void*           data,
    size_t          length) {
  ssize_t size = -1;
  while (size < 0) {
    size = ::recv(_d->conn_socket, data, length, 0);
    if ((size < 0) && (errno != EINTR)) {
      hlog_error("%s reading", strerror(errno));
      break;
    }
  }
  if (size > 0) {
    hlog_regression("received %u", size);
  }
  return size;
}

const char* UnixSocket::path() const {
  return _d->master_data->path;
}
