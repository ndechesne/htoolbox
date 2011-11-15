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

#include <report.h>
#include <tlv.h>
#include "tlv_helper.h"

using namespace htoolbox;
using namespace tlv;

int ReceptionManager::Blob::submit(size_t size, const char* val) {
  if (size + _received > _capacity) {
    return -ERANGE;
  }
  if (size > 0) {
    memcpy(&_buffer[_received], val, size);
    _received += size;
  }
  if (size < BUFFER_MAX) {
    // Sugar for strings
    if (_received < _capacity) {
      _buffer[_received] = '\0';
    }
    // End of data, re-arm
    _received = 0;
  }
  return 0;
}

void ReceptionManager::remove(uint16_t tag) {
  for (std::list<IObject*>::iterator it = _objects.begin();
      it != _objects.end(); ++it) {
    if ((*it)->tag() == tag) {
      delete *it;
      _objects.erase(it);
      break;
    }
  }
}

bool TransmissionManager::Blob::ready() {
  // Need to always send less than BUFFER_MAX to signal the end of data
  if (_ended) {
    // Re-arm
    _ended = false;
    _sent = 0;
    return false;
  }
  size_t left = _size - _sent;
  if (left >= BUFFER_MAX) {
    _len = BUFFER_MAX;
  } else {
    _len = left;
    _ended = true;
  }
  _reader = &_buffer[_sent];
  _sent += _len;
  return true;
}

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
    if (type < 0) {
      hlog_debug("receive: type=%d tag=%d len=%zu msg=%s", type, tag, len,
        val);
    } else {
      hlog_debug_buffer(len, val, "receive: type=%d tag=%d len=%zu val:",
        type, tag, len);
    }
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

ssize_t TransmissionManager::send(
    Sender&         sender,
    bool            start_and_end,
    useconds_t      udelay) {
  ssize_t rc = 0;
  if (start_and_end && ! _started) {
    rc = this->start(sender);
    hlog_regression("send: rc=%zd start", rc);
    if (rc < 0) return -errno;
    if (udelay > 0) {
      usleep(udelay);
    }
  }
  const ITransmissionManager* src = this;
  while (src != NULL) {
    for (std::list<IObject*>::const_iterator it = _objects.begin();
        it != _objects.end(); ++it) {
      IObject& o = **it;
      while (o.ready()) {
        if (o.length() < 0) {
          rc = sender.error(static_cast<int>(-o.length()));
          if (rc < 0) return -errno;
          return o.length();
        } else {
          rc = sender.write(o.tag(), o.value(), o.length());
          hlog_generic_buffer(debug, Report::HLOG_TLV_NOSEND, -1,
            o.length(), o.value(),
            "send: rc=%zd tag=%d len=%zu val:", rc, o.tag(), o.length());
          if (rc < 0) return -errno;
          if (udelay > 0) {
            usleep(udelay);
          }
        }
      }
    }
    src = src->next();
  }
  if (start_and_end && _started) {
    rc = sender.end();
    hlog_regression("send: rc=%zd end", rc);
    if (rc < 0) return -errno;
    _started = false;
    if (udelay > 0) {
      usleep(udelay);
    }
  }
  return 0;
}
