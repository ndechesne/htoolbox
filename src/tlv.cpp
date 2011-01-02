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

#include "report.h"
#include "tlv.h"

using namespace htools;
using namespace tlv;

enum {
  START_CODE = 0xbadc01d0,
  END_CODE = 0xdeadbeef,
  MAX_LENGTH = 0xffff,
};

int Sender::start() {
  if (_started) {
    errno = EBADF;
    _failed = true;
    return -1;
  }
  _started = true;
  int rc = write(0, START_CODE);
  _failed = rc < 0;
  return rc;
}

int Sender::write(uint8_t tag, const void* buffer, size_t len) {
  if (! _started) {
    errno = EBADF;
    _failed = true;
    return -1;
  }
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
  // Tag and length
  char tag_len[3];
  tag_len[0] = tag;
  tag_len[1] = static_cast<char>(len >> 8);
  tag_len[2] = static_cast<char>(len);
  rc = _fd.write(tag_len, sizeof(tag_len));
  if (rc < 3) {
    _failed = true;
    return -1;
  }
  // Value
  if (len > 0) {
    size_t sent = 0;
    while (sent < len) {
      rc = _fd.write(&cbuffer[sent], len - sent);
      if (rc <= 0) {
        hlog_error("%s sending value", strerror(errno));
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
  if (! _started) {
    errno = EBADF;
    _failed = true;
    return -1;
  }
  write(0, END_CODE);
  _started = false;
  return _failed ? -1 : 0;
}

Receiver::Type Receiver::receive(
    uint8_t*    tag,
    size_t*     len,
    char*       val) {
  ssize_t rc;
  // Receive header first (TL)
  char tag_len[3];
  rc = _fd.read(tag_len, sizeof(tag_len));
  if (rc < 3) {
    *tag = rc < 0;
    *len = errno;
    strcpy(val, "receiving tag and length");
    return Receiver::ERROR;
  }
  *tag = tag_len[0];
  *len = tag_len[1] << 8;
  *len |= tag_len[2];
  // Receive value (V)
  size_t count = 0;
  while (count < *len) {
    rc = _fd.read(&val[count], *len - count);
    if (rc <= 0) {
      *len = errno;
      strcpy(val, "receiving value");
      return Receiver::ERROR;
    }
    count += rc;
  }
  val[*len] = '\0';
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
      hlog_error("failed to interpret '%s' -> %x", val, code);
      sprintf(val, "interpreting value %x, len = %zu", code, *len);
      *len = EINVAL;
      return Receiver::ERROR;
    }
  } else {
    return Receiver::DATA;
  }
}
