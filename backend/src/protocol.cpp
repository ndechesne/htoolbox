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
  OPEN = 0xbadc01d0,
  CLOSE = 0xdeadbeef,
};

int Sender::open() {
  return write(0, OPEN);
}

int Sender::write(uint8_t tag, const char* string, size_t len) {
  if (len == 0) {
    len = strlen(string);
  }
  if (len > 0xffff) {
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
    rc = ::write(_fd, &string[sent], len - sent);
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

int Sender::close() {
  return write(0, CLOSE);
}
