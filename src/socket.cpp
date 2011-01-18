/*
     Copyright (C) 2010-2011  Herve Fache

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

using namespace htoolbox;

struct SharedData {
  char*             hostname;
  size_t            hostname_len;
  uint16_t          port;
  int               listen_socket;
  SharedData() : hostname(NULL) {}
  ~SharedData() { free(hostname); }
  int createSocket(
    sa_family_t*      family,
    struct sockaddr**  sock_p,
    socklen_t*        sock_len) const;
};

int SharedData::createSocket(
    sa_family_t*      family,
    struct sockaddr**  sock,
    socklen_t*        sock_len) const {
  if (port == 0) {
    // UNIX
    *family = AF_UNIX;
    *sock_len = static_cast<socklen_t>(sizeof(sa_family_t) +
      strlen(hostname) + 1);
    struct sockaddr_un* sock_un =
      static_cast<struct sockaddr_un*>(malloc(*sock_len));
    sock_un->sun_family = *family;
    strcpy(sock_un->sun_path, hostname);
    *sock = reinterpret_cast<struct sockaddr*>(sock_un);
  } else {
    // INET
    *family = AF_INET;
    *sock_len = sizeof(struct sockaddr_in);
    struct sockaddr_in* sock_in =
      static_cast<struct sockaddr_in*>(malloc(*sock_len));
    sock_in->sin_family = *family;
    uint32_t ip;
    hostname[hostname_len] = '\0';
    int rc = Socket::getAddress(hostname, &ip);
    hostname[hostname_len] = ':';
    if (rc < 0) {
      return -1;
    }
    sock_in->sin_addr.s_addr = ip;
    sock_in->sin_port = htons(port);
    *sock = reinterpret_cast<struct sockaddr*>(sock_in);
  }
  return 0;
}

struct Socket::Private {
  const SharedData* master_data;
  SharedData*       data;
  int               conn_socket;
  struct timeval    read_timeout;
  struct timeval    write_timeout;
  Private() : master_data(NULL), data(NULL), conn_socket(-1) {
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 0;
    write_timeout.tv_sec = 0;
    write_timeout.tv_usec = 0;
  }
  void setReadTimeout(__time_t seconds,  __suseconds_t useconds) {
    read_timeout.tv_sec = seconds;
    read_timeout.tv_usec = useconds;
  }
  void setWriteTimeout(__time_t seconds,  __suseconds_t useconds) {
    write_timeout.tv_sec = seconds;
    write_timeout.tv_usec = useconds;
  }
};

Socket::Socket(const char* hostname, int port) : _d(new Private) {
  _d->data = new SharedData;
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
  _d->read_timeout = s._d->read_timeout;
  _d->write_timeout = s._d->write_timeout;
}

Socket::~Socket() {
  if (_d->data != NULL) {
    if (_d->data->listen_socket >= 0) {
      ::close(_d->data->listen_socket);
    }
    delete _d->data;
  }
  delete _d;
}

int Socket::listen(int backlog) {
  if (_d->data == NULL) {
    errno = EPERM;
    return -1;
  }
  if (_d->data->listen_socket != -1) {
    errno = EBUSY;
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
    free(sock);
    return -1;
  }
  int re_use = 1;
  ::setsockopt(_d->data->listen_socket, SOL_SOCKET, SO_REUSEADDR, &re_use,
    sizeof(int));
  /* Use default values to bind and listen */
  if (family == AF_UNIX) {
    unlink(_d->data->hostname);
  }
  if (::bind(_d->data->listen_socket, sock, sock_len)) {
    free(sock);
    return -1;
  }
  free(sock);
  if (::listen(_d->data->listen_socket, backlog)) {
    return -1;
  }
  hlog_regression("fd = %d", _d->data->listen_socket);
  return _d->data->listen_socket;
}

int Socket::release() {
  int rc = -1;
  if (_d->data == NULL) {
    errno = EPERM;
  } else
  if (_d->data->listen_socket == -1) {
    errno = EBADF;
  } else
  {
    rc = ::close(_d->data->listen_socket);
    _d->data->listen_socket = -1;
    if (rc < 0) {
      hlog_warning("%s", strerror(errno));
    }
    // Port is 0 if socket family is AF_UNIX
    if (_d->data->port == 0) {
      unlink(_d->data->hostname);
    }
  }
  return rc;
}

int Socket::setReadTimeout(int seconds, int microseconds) {
  _d->read_timeout.tv_sec = seconds;
  _d->read_timeout.tv_usec = microseconds;
  struct timeval& tv = _d->read_timeout;
  if ((_d->conn_socket != -1) && (tv.tv_sec > 0) && (tv.tv_usec > 0)) {
    if(::setsockopt(_d->data->listen_socket, SOL_SOCKET, SO_RCVTIMEO, &tv,
        sizeof(struct timeval)) < 0) {
      return -1;
    }
  }
  return 0;
}

int Socket::setWriteTimeout(int seconds, int microseconds) {
  _d->write_timeout.tv_sec = seconds;
  _d->write_timeout.tv_usec = microseconds;
  struct timeval& tv = _d->write_timeout;
  if ((_d->conn_socket != -1) && (tv.tv_sec > 0) && (tv.tv_usec > 0)) {
    if(::setsockopt(_d->data->listen_socket, SOL_SOCKET, SO_SNDTIMEO, &tv,
        sizeof(struct timeval)) < 0) {
      return -1;
    }
  }
  return 0;
}


int Socket::open() {
  if (_d->conn_socket != -1) {
    errno = EBUSY;
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
      _d->conn_socket = -1;
    }
    free(sock);
  } else {
    // Accept connection from client
    _d->conn_socket = ::accept(_d->master_data->listen_socket, 0, 0);
  }
  if (_d->conn_socket >= 0) {
    _d->setReadTimeout(_d->read_timeout.tv_sec, _d->read_timeout.tv_usec);
    _d->setWriteTimeout(_d->write_timeout.tv_sec, _d->write_timeout.tv_usec);
  }
  return _d->conn_socket;
}

int Socket::close() {
  int rc = -1;
  if (_d->conn_socket == -1) {
    errno = EBADF;
  } else {
    rc = ::close(_d->conn_socket);
    _d->conn_socket = -1;
    if (rc < 0) {
      hlog_warning("%s", strerror(errno));
    }
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
        return -1;
      case 0:
        hlog_error("socket closed");
        return 0;
      default:
        hlog_regression("sent (partial) %zd", size);
        sent += size;
    }
  } while (sent < static_cast<ssize_t>(length));
  if (sent > 0) {
    hlog_regression("sent (total) %zd", sent);
  }
  return sent;
}

ssize_t Socket::stream(void* buffer, size_t max_size) {
  ssize_t size = -1;
  while (size < 0) {
    size = ::recv(_d->conn_socket, buffer, max_size, 0);
    if ((size < 0) && (errno != EINTR)) {
      break;
    }
  }
  if (size > 0) {
    hlog_regression("received %zd", size);
  }
  return size;
}

ssize_t Socket::read(
    void*           buffer,
    size_t          size) {
  char* cbuffer = static_cast<char*>(buffer);
  size_t count = 0;
  while (count < size) {
    ssize_t rc = ::recv(_d->conn_socket, &cbuffer[count], size - count, 0);
    if (rc <= 0) {
      if (rc < 0) {
        if (errno == EINTR) {
          continue;
        }
      }
      break;
    } else {
      count += rc;
    }
  }
  if (count > 0) {
    hlog_regression("received %zu", size);
  }
  return count;
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
