/*
    Copyright (C) 2011  Hervé Fache

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

#include <tlv.h>
#include "tlv_helper.h"

using namespace htoolbox;
using namespace tlv;

int ReceptionManager::Int::submit(size_t size, const char* val) {
  if (_int_32 != NULL) {
    if (size >= 11) {
      return -ERANGE;
    }
    if (sscanf(val, "%d", _int_32) < 1) {
      return -EINVAL;
    }
  } else
  if (_uint_32 != NULL) {
    if (size >= 10) {
      return -ERANGE;
    }
    if (sscanf(val, "%u", _uint_32) < 1) {
      return -EINVAL;
    }
  } else
  if (_int_64 != NULL) {
    if (size >= 20) {
      return -ERANGE;
    }
    if (sscanf(val, "%jd", _int_64) < 1) {
      return -EINVAL;
    }
  } else
  if (_uint_64 != NULL) {
    if (size >= 20) {
      return -ERANGE;
    }
    if (sscanf(val, "%ju", _uint_64) < 1) {
      return -EINVAL;
    }
  } else
  {
    return -EINVAL;
  }
  return 0;
}

int ReceptionManager::receive(Socket& sock, abort_cb_f abort_cb, void* user) {
  Receiver        receiver(sock);
  Receiver::Type  type;
  uint16_t        tag;
  size_t          len;
  char            val[65536];
  do {
    type = receiver.receive(&tag, &len, val);
    switch (type) {
      case Receiver::CHECK:
        if ((abort_cb != NULL) && abort_cb(user)) {
          return -ECANCELED;
        }
      case Receiver::START:
      case Receiver::END:
        break;
      case Receiver::DATA: {
        int sub_rc = submit(tag, len, val);
        if (sub_rc < 0) {
          return sub_rc;
        }
      } break;
      default:
        return -EBADE;
    }
  } while (type > Receiver::END);
  return 0;
}
