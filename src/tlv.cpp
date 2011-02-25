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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "tlv.h"

using namespace htoolbox;
using namespace tlv;

enum {
  START_TAG  = 65530,
  CHECK_TAG  = 65531,
  END_TAG    = 65532,
  ERROR_TAG  = 65533,
  MAX_LENGTH = 0xffff,
};

int Sender::start() {
  if (_started) {
    errno = EBUSY;
    _failed = true;
    return -1;
  }
  _started = true;
  int rc = write(START_TAG, "", 0);
  _failed = rc < 0;
  return rc;
}

int Sender::check() {
  if (! _started) {
    errno = EBADF;
    _failed = true;
    return -1;
  }
  int rc = write(CHECK_TAG, "", 0);
  _failed = rc < 0;
  return rc;
}

int Sender::write(uint16_t tag, const void* buffer, size_t len) {
  if (! _started) {
    errno = EBADF;
    _failed = true;
    return -1;
  }
  const char* cbuffer = static_cast<const char*>(buffer);
  if (len > MAX_LENGTH) {
    errno = ERANGE;
    _failed = true;
    return -1;
  }
  ssize_t rc;
  // Tag and length
  uint8_t tag_len[4];
  tag_len[0] = static_cast<uint8_t>(tag >> 8);
  tag_len[1] = static_cast<uint8_t>(tag);
  tag_len[2] = static_cast<uint8_t>(len >> 8);
  tag_len[3] = static_cast<uint8_t>(len);
  rc = _fd.put(tag_len, sizeof(tag_len));
  if (rc < 3) {
    _failed = true;
    return -1;
  }
  // Value
  rc = _fd.put(cbuffer, len);
  if (rc < static_cast<ssize_t>(len)) {
    _failed = true;
    return -1;
  }
  return 0;
}

int Sender::write(uint16_t tag, int32_t number) {
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
  write(END_TAG, "", 0);
  _started = false;
  return _failed ? -1 : 0;
}

int Sender::error(int error_no) {
  if (! _started) {
    errno = EBADF;
    _failed = true;
    return -1;
  }
  write(ERROR_TAG, error_no);
  _started = false;
  return _failed ? -1 : 0;
}

Receiver::Type Receiver::receive(
    uint16_t*   tag,
    size_t*     len,
    char*       val) {
  ssize_t rc;
  // Receive header first (TL)
  uint8_t tag_len[4];
  rc = _fd.get(tag_len, sizeof(tag_len));
  if (rc < 3) {
    if (rc == 0) {
      *tag = 0;
      *len = 0;
      strcpy(val, "connection closed by sender");
    } else {
      *tag = 1;
      *len = errno;
      sprintf(val, "%s receiving tag and length", strerror(errno));
    }
    return Receiver::ERROR;
  }
  *tag = static_cast<uint16_t>((tag_len[0] << 8) + tag_len[1]);
  *len = static_cast<uint16_t>((tag_len[2] << 8) + tag_len[3]);
  // Receive value (V)
  size_t count = 0;
  while (count < *len) {
    rc = _fd.get(&val[count], *len - count);
    if (rc <= 0) {
      *len = errno;
      if (rc == 0) {
        *tag = 0;
        strcpy(val, "connection unexpectedly closed while receiving value");
      } else {
        sprintf(val, "%s receiving value", strerror(errno));
      }
      return Receiver::ERROR;
    }
    count += rc;
  }
  val[*len] = '\0';
  // Call listener
  Type type;
  switch (*tag) {
    case START_TAG:
      strcpy(val, "start");
      type = Receiver::START;
      break;
    case CHECK_TAG:
      strcpy(val, "check");
      type = Receiver::CHECK;
      break;
    case END_TAG:
      strcpy(val, "end");
      type = Receiver::END;
      break;
    case ERROR_TAG:
      sscanf(val, "%d", len);
      type = Receiver::ERROR;
      break;
    default:
      type = Receiver::DATA;
  }
  return type;
}
