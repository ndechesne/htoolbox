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

int ReceptionManager::submit(uint16_t tag, size_t size, const char* val) {
  for (std::list<IObject*>::iterator it = _objects.begin();
      it != _objects.end(); ++it) {
    if ((*it)->tag() == tag) {
      return (*it)->submit(size, val);
    }
  }
  if (_next != NULL) {
    return _next->submit(tag, size, val);
  }
  return -ENOSYS;
}

int ReceptionManager::receive(Receiver& rec, abort_cb_f abort_cb, void* user) {
  Receiver::Type  type;
  uint16_t        tag;
  size_t          len;
  char            val[65536];
  do {
    type = rec.receive(&tag, &len, val);
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

int TransmissionManager::send(Sender& sender, bool start_and_end) {
  int rc = 0;
  if (start_and_end && ! _started) {
    rc = this->start(sender);
    if (rc < 0) return -errno;
  }
  const ITransmissionManager* src = this;
  while (src != NULL) {
    for (std::list<IObject*>::const_iterator it = _objects.begin();
        it != _objects.end(); ++it) {
      IObject& o = **it;
      while (o.ready()) {
        rc = sender.write(o.tag(), o.value(), o.length());
        if (rc < 0) return -errno;
      }
    }
    src = src->next();
  }
  if (start_and_end && _started) {
    rc = sender.end();
    if (rc < 0) return -errno;
    _started = false;
  }
  return 0;
}
