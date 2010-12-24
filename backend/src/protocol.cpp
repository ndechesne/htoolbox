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
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "protocol.h"

using namespace hbackend;

enum {
  START = 0xbadc01d0,
  END = 0xdeadbeef,
  MAX_LENGTH = 0xffff,
};

int Sender::start() {
  return write(0, START);
}

int Sender::write(uint8_t tag, const void* buffer, size_t len) {
  const char* cbuffer = static_cast<const char*>(buffer);
  if (len == 0) {
    len = strlen(cbuffer);
  }
  if (len > MAX_LENGTH) {
    errno = ERANGE;
    return -1;
  }
  ssize_t rc;
  // Tag
  rc = ::write(_fd, &tag, 1);
  if (rc < 1) {
    return -1;
  }
  // Length
  char len_str[8];
  rc = sprintf(len_str, "%04x", len);
  rc = ::write(_fd, len_str, rc);
  if (rc < 1) {
    return -1;
  }
  // Value
  size_t sent = 0;
  while (sent < len) {
    rc = ::write(_fd, &cbuffer[sent], len - sent);
    if (rc <= 0) {
      return -1;
    }
    sent += rc;
  }
  return 0;
}

int Sender::write(uint8_t tag, int32_t number) {
  char string[16];
  int len = sprintf(string, "%x", number);
  return write(tag, string, len);
}

int Sender::end() {
  return write(0, END);
}

struct Receiver::Private {
  int       fd;
  read_cb_f cb;
  pthread_t thread;
  Private(int fd_, read_cb_f cb_) : fd(fd_), cb(cb_) {}
};

void* receiver_thread(void* user) {
  Receiver::Private* _d = static_cast<Receiver::Private*>(user);
  uint8_t tag;
  char len_str[8];
  size_t  len = 0;
  char value[MAX_LENGTH];
  ssize_t rc;
  while (true) {
    // Receive header first (TL)
    rc = read(_d->fd, &tag, 1);
    if (rc < 1) {
      _d->cb(Receiver::ERROR, rc < 0, errno, "receiving tag");
      break;
    }
    rc = read(_d->fd, len_str, 4);
    if (rc < 4) {
      _d->cb(Receiver::ERROR, tag, errno, "receiving length");
      break;
    }
    if (sscanf(len_str, "%x", &len) < 1) {
      _d->cb(Receiver::ERROR, tag, errno, "decoding length");
      break;
    }
    // Receive value (V)
    size_t count = 0;
    while (count < len) {
      rc = read(_d->fd, &value[count], len - count);
      if (rc <= 0) {
        _d->cb(Receiver::ERROR, tag, errno, "receiving value");
        break;
      }
      count += rc;
    }
    // Call listener
    if (tag == 0) {
      uint32_t code;
      if (sscanf(value, "%x", &code) < 1) {
        char message[128];
        sprintf(message, "decoding value '%s' %x", value, value[0]);
        _d->cb(Receiver::ERROR, tag, len, message);
        break;
      }
      if (code == START) {
        _d->cb(Receiver::START, tag, 0, "start");
      } else
      if (code == END) {
        _d->cb(Receiver::END, tag, 0, "end");
      } else
      {
        _d->cb(Receiver::ERROR, tag, 0, "interpreting value");
      }
    } else {
      _d->cb(Receiver::DATA, tag, len, value);
    }
  }
  return NULL;
}

Receiver::Receiver(int fd, read_cb_f cb) : _d(new Private(fd, cb)) {}

int Receiver::open() {
  return pthread_create(&_d->thread, NULL, receiver_thread, _d);
}

int Receiver::close() {
  return pthread_cancel(_d->thread);
}
