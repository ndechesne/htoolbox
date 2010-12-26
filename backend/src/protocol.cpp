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
  START_CODE = 0xbadc01d0,
  END_CODE = 0xdeadbeef,
  MAX_LENGTH = 0xffff,
};

int Sender::start() {
  int rc = write(0, START_CODE);
  _failed = rc < 0;
  return rc;
}

int Sender::write(uint8_t tag, const void* buffer, size_t len) {
  const char* cbuffer = static_cast<const char*>(buffer);
  if ((len == 0) && (cbuffer != NULL)) {
    len = strlen(cbuffer);
  }
  if (len > MAX_LENGTH) {
    errno = ERANGE;
    _failed = true;
    return -1;
  }
  ssize_t rc;
  // Tag
  rc = ::write(_fd, &tag, 1);
  if (rc < 1) {
    _failed = true;
    return -1;
  }
  // Length
  char len_str[8];
  rc = sprintf(len_str, "%04x", len);
  rc = ::write(_fd, len_str, rc);
  if (rc < 1) {
    _failed = true;
    return -1;
  }
  // Value
  if (len > 0) {
    size_t sent = 0;
    while (sent < len) {
      rc = ::write(_fd, &cbuffer[sent], len - sent);
      if (rc <= 0) {
        _failed = true;
        return -1;
      }
      sent += rc;
    }
  }
  return 0;
}

int Sender::write(uint8_t tag, int32_t number) {
  char string[16];
  int len = sprintf(string, "%d", number);
  return write(tag, string, len);
}

int Sender::end() {
  return ((write(0, END_CODE) < 0) || _failed) ? -1 : 0;
}

Receiver::Type Receiver::receive(
    uint8_t*    tag,
    size_t*     len,
    char*       val) {
  ssize_t rc;
  // Receive header first (TL)
  rc = read(_fd, tag, 1);
  if (rc < 1) {
    *tag = rc < 0;
    *len = errno;
    strcpy(val, "receiving tag");
    return Receiver::ERROR;
  }
  char len_str[8];
  rc = read(_fd, len_str, 4);
  if (rc < 4) {
    *len = errno;
    strcpy(val, "receiving length");
    return Receiver::ERROR;
  }
  if (sscanf(len_str, "%x", len) < 1) {
    *len = errno;
    strcpy(val, "decoding length");
    return Receiver::ERROR;
  }
  // Receive value (V)
  size_t count = 0;
  while (count < *len) {
    rc = read(_fd, &val[count], *len - count);
    if (rc <= 0) {
      *len = errno;
      strcpy(val, "receiving value");
      return Receiver::ERROR;
    }
    count += rc;
  }
  // Call listener
  if (*tag == 0) {
    uint32_t code;
    if (sscanf(val, "%d", &code) < 1) {
      char message[128];
      sprintf(message, "decoding value '%s' %x", val, val[0]);
      return Receiver::ERROR;
    } else
    if (code == START_CODE) {
      *len = 0;
      strcpy(val, "start");
      return Receiver::START;
    } else
    if (code == END_CODE) {
      *len = 0;
      strcpy(val, "end");
      return Receiver::END;
    } else
    {
      *len = EINVAL;
      sprintf(val, "interpreting value %x", code);
      return Receiver::ERROR;
    }
  } else {
    return Receiver::DATA;
  }
}
