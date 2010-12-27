#include <netdb.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "report.h"
#include "inet_socket.h"

using namespace htools;

struct ServerData {
  char*             hostname;
  uint16_t          port;
  int               listen_socket;
  ServerData() : hostname(NULL) {}
  ~ServerData() { free(hostname); }
};

struct InetSocket::Private {
  const ServerData* master_data;
  ServerData*       data;
  int               conn_socket;
};

InetSocket::InetSocket(int port, const char* hostname) : _d(new Private) {
  _d->data = new ServerData;
  _d->master_data = _d->data;
  _d->data->hostname = strdup(hostname);
  _d->data->port = static_cast<uint16_t>(port);
  _d->data->listen_socket = -1;
  _d->conn_socket = -1;
}

InetSocket::InetSocket(const InetSocket& s) : IReaderWriter(), _d(new Private) {
  _d->master_data = s._d->master_data;
  _d->data = NULL;
  _d->conn_socket = -1;
}

InetSocket::~InetSocket() {
  if ((_d->data != NULL) && (_d->data->listen_socket >= 0)) {
    ::close(_d->data->listen_socket);
  }
  delete _d;
}

int InetSocket::listen(int backlog) {
  if (_d->data == NULL) {
    hlog_error("only the original object can listen");
    return -1;
  }
  if (_d->data->listen_socket != -1) {
    hlog_error("socket already open");
    return -1;
  }
  /* Create server socket */
  struct sockaddr_in sock;
  sock.sin_family = AF_INET;
  uint32_t ip;
  if (getAddress(_d->data->hostname, &ip)) {
    hlog_error("%s getting address", strerror(errno));
    return -1;
  }
  sock.sin_addr.s_addr = ip;
  sock.sin_port = htons(_d->data->port);
  /* We want quick restarts */
  _d->data->listen_socket = ::socket(sock.sin_family, SOCK_STREAM, 0);
  if (_d->data->listen_socket == -1) {
    hlog_error("%s creating socket", strerror(errno));
    return -1;
  }
  int re_use = 1;
  ::setsockopt(_d->data->listen_socket, SOL_SOCKET, SO_REUSEADDR, &re_use,
    sizeof(int));
  /* Use default values to bind and listen */
  if (::bind(_d->data->listen_socket, reinterpret_cast<struct sockaddr*>(&sock),
      sizeof(struct sockaddr_in))) {
    hlog_error("%s binding socket", strerror(errno));
    return -1;
  }
  if (::listen(_d->data->listen_socket, backlog)) {
    hlog_error("%s listening", strerror(errno));
    return -1;
  }
  hlog_regression("fd = %d", _d->data->listen_socket);
  return 0;
}

int InetSocket::open() {
  if (_d->conn_socket != -1) {
    hlog_error("connection already open");
    return -1;
  }
  if ((_d->data != NULL) && (_d->data->listen_socket == -1)) {
    /* Create client socket */
    struct sockaddr_in sock;
    sock.sin_family = AF_INET;
    uint32_t ip;
    if (getAddress(_d->data->hostname, &ip)) {
      hlog_error("%s getting address", strerror(errno));
      return -1;
    }
    sock.sin_addr.s_addr = ip;
    sock.sin_port = htons(_d->data->port);
    _d->conn_socket = ::socket(sock.sin_family, SOCK_STREAM, 0);
    if (::connect(_d->conn_socket, reinterpret_cast<struct sockaddr*>(&sock),
        sizeof(struct sockaddr_in)) < 0) {
      hlog_error("%s connecting", strerror(errno));
      return -1;
    }
  } else {
    // Server
    _d->conn_socket = ::accept(_d->master_data->listen_socket, 0, 0);
  }
  return _d->conn_socket;
}

int InetSocket::close() {
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

ssize_t InetSocket::write(
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
        hlog_regression("sent %u", size);
        sent += size;
    }
  } while (sent < static_cast<ssize_t>(length));
  if (sent > 0) {
    hlog_debug("sent %u", sent);
  }
  return sent;
}

ssize_t InetSocket::read(
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
    hlog_debug("received %u", size);
  }
  return size;
}

int InetSocket::getAddress(
    const char*     hostname,
    uint32_t*       address) {
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  struct addrinfo* res;
  int errcode = getaddrinfo(hostname, NULL, &hints, &res);
  if (errcode != 0) {
    hlog_alert("'%s': %s\n", hostname, gai_strerror(errcode));
    return -1;
  }
  struct sockaddr_in* addr = reinterpret_cast<struct sockaddr_in*>(res->ai_addr);
  *address = addr->sin_addr.s_addr;
  char tmp[32];
  hlog_verbose("'%s': IP = %s\n", hostname,
    inet_ntop(AF_INET, &addr->sin_addr, tmp, sizeof(tmp)));
  freeaddrinfo(res);
  return 0;
}
