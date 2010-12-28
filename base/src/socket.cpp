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
#include <arpa/inet.h>
#include <sys/un.h>

#include "report.h"
#include "socket.h"

using namespace htools;

struct ServerData {
  char*             hostname;
  size_t            hostname_len;
  uint16_t          port;
  int               listen_socket;
  ServerData() : hostname(NULL) {}
  ~ServerData() { free(hostname); }
  int createSocket(
    sa_family_t*      family,
    struct sockaddr**  sock_p,
    socklen_t*        sock_len) const;
};

int ServerData::createSocket(
    sa_family_t*      family,
    struct sockaddr**  sock,
    socklen_t*        sock_len) const {
  if (port == 0) {
    // UNIX
    *family = AF_UNIX;
    struct sockaddr_un sock_un;
    sock_un.sun_family = *family;
    strcpy(sock_un.sun_path, hostname);
    *sock = reinterpret_cast<struct sockaddr*>(&sock_un);
    *sock_len = sizeof(struct sockaddr_un);
  } else {
    // INET
    *family = AF_INET;
    struct sockaddr_in sock_in;
    sock_in.sin_family = *family;
    uint32_t ip;
    hostname[hostname_len] = '\0';
    int rc = Socket::getAddress(hostname, &ip);
    hostname[hostname_len] = ':';
    if (rc < 0) {
      hlog_error("%s getting address", strerror(errno));
      return -1;
    }
    sock_in.sin_addr.s_addr = ip;
    sock_in.sin_port = htons(port);
    *sock = reinterpret_cast<struct sockaddr*>(&sock_in);
    *sock_len = sizeof(struct sockaddr_in);
  }
  return 0;
}

struct Socket::Private {
  const ServerData* master_data;
  ServerData*       data;
  int               conn_socket;
};

Socket::Socket(const char* hostname, int port) : _d(new Private) {
  _d->data = new ServerData;
  _d->master_data = _d->data;
  _d->data->hostname_len = strlen(hostname);
  if (port > 0) {
    _d->data->hostname = static_cast<char*>(malloc(_d->data->hostname_len + 8));
    sprintf(_d->data->hostname, "%s:%u", hostname, port);
  } else {
    _d->data->hostname = strdup(hostname);
  }
  _d->data->port = static_cast<uint16_t>(port);
  _d->data->listen_socket = -1;
  _d->conn_socket = -1;
}

Socket::Socket(const Socket& s) : IReaderWriter(), _d(new Private) {
  _d->master_data = s._d->master_data;
  _d->data = NULL;
  _d->conn_socket = -1;
}

Socket::~Socket() {
  if ((_d->data != NULL) && (_d->data->listen_socket >= 0)) {
    ::close(_d->data->listen_socket);
  }
  delete _d;
}

int Socket::listen(int backlog) {
  if (_d->data == NULL) {
    hlog_error("only the original object can listen");
    return -1;
  }
  if (_d->data->listen_socket != -1) {
    hlog_error("socket already open");
    return -1;
  }
  /* Create server socket */
  sa_family_t family;
  struct sockaddr* sock;
  socklen_t sock_len;
  if (_d->data->createSocket(&family, &sock, &sock_len) < 0) {
    return -1;
  }
  /* We want quick restarts */
  _d->data->listen_socket = ::socket(family, SOCK_STREAM, 0);
  if (_d->data->listen_socket == -1) {
    hlog_error("%s creating socket", strerror(errno));
    return -1;
  }
  int re_use = 1;
  ::setsockopt(_d->data->listen_socket, SOL_SOCKET, SO_REUSEADDR, &re_use,
    sizeof(int));
  /* Use default values to bind and listen */
  if (::bind(_d->data->listen_socket, sock, sock_len)) {
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

int Socket::open() {
  if (_d->conn_socket != -1) {
    hlog_error("connection already open");
    return -1;
  }
  if ((_d->data != NULL) && (_d->data->listen_socket == -1)) {
    sa_family_t family;
    struct sockaddr* sock;
    socklen_t sock_len;
    if (_d->data->createSocket(&family, &sock, &sock_len) < 0) {
      return -1;
    }
    // Connect to server
    _d->conn_socket = ::socket(family, SOCK_STREAM, 0);
    if (::connect(_d->conn_socket, sock, sock_len) < 0) {
      hlog_error("%s connecting", strerror(errno));
      return -1;
    }
  } else {
    // Accept connection from client
    _d->conn_socket = ::accept(_d->master_data->listen_socket, 0, 0);
  }
  return _d->conn_socket;
}

int Socket::close() {
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

ssize_t Socket::write(
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

ssize_t Socket::read(
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

const char* Socket::path() const {
  return _d->master_data->hostname;
}

int Socket::getAddress(
    const char*     hostname,
    uint32_t*       address) {
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  struct addrinfo* res;
  int errcode = getaddrinfo(hostname, NULL, &hints, &res);
  if (errcode != 0) {
    hlog_alert("'%s': %s", hostname, gai_strerror(errcode));
    return -1;
  }
  struct sockaddr_in* addr = reinterpret_cast<struct sockaddr_in*>(res->ai_addr);
  *address = addr->sin_addr.s_addr;
  char tmp[32];
  hlog_verbose("'%s': IP = %s", hostname,
    inet_ntop(AF_INET, &addr->sin_addr, tmp, sizeof(tmp)));
  freeaddrinfo(res);
  return 0;
}
