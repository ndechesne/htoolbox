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
#include <string.h>
#include <errno.h>

#include <string>

using namespace std;

#include <report.h>
#include <files.h>

using namespace htools;

#include <hbackup.h>  // progress_f (needs architecture review)
#include <data.h>

using namespace hbackup;

#include "inet_socket.h"
#include "protocol.h"
#include "data_common.h"

using namespace hbackend;

struct Data::Private {
  string            path;
  InetSocket        sock;
  Receiver*         receiver;
  progress_f        progress;
  Private(const char* hostname, int port)
  : sock(port, hostname), receiver(NULL), progress(NULL) {}
};

Data::Data(const char* path) : _d(new Private("localhost", 12345)) {
  _d->path = path;
}

Data::~Data() {
  delete _d;
}

int Data::open(
    bool            create) {
  (void) create;
  if (_d->sock.open() < 0) {
    hlog_error("%s opening socket", strerror(errno));
    return -1;
  }
  _d->receiver = new Receiver(_d->sock);
  return 0;
}

int Data::close() {
  if (_d->receiver != NULL) {
    delete _d->receiver;
    _d->receiver = NULL;
  }
  return _d->sock.close();
}

void Data::setProgressCallback(progress_f progress) {
  _d->progress = progress;
  Sender sender(_d->sock);
  sender.start();
  sender.write(static_cast<uint8_t>(METHOD), PROGRESS);
  if (sender.end() >= 0) {
    // Get STATUS
    Receiver        receiver(_d->sock);
    Receiver::Type  type;
    uint8_t         tag;
    size_t          len;
    char            val[65536];
    do {
      type = receiver.receive(&tag, &len, val);
    } while (type > Receiver::END);
  }
}

int Data::name(
    const char*     hash,
    char*           path,
    char*           extension) const {
  // Send hash, get path, and extension if not null
  Sender sender(_d->sock);
  sender.start();
  sender.write(static_cast<uint8_t>(METHOD), NAME);
  sender.write(static_cast<uint8_t>(HASH), hash);
  if (extension != NULL) {
    sender.write(static_cast<uint8_t>(EXTENSION), NULL, 0);
  }
  if (sender.end() < 0) {
    return -1;
  }
  // Get STATUS PATH [EXTENSION]
  Receiver        receiver(_d->sock);
  Receiver::Type  type;
  uint8_t         tag;
  size_t          len;
  char            val[65536];
  int             rc = 0;
  do {
    type = receiver.receive(&tag, &len, val);
    switch (type) {
      case Receiver::START:
      case Receiver::END:
        break;
      case Receiver::DATA:
        switch (tag) {
          case STATUS:
            rc = atoi(val);
            break;
          case PATH:
            strcpy(path, val);
            break;
          case EXTENSION:
            strcpy(extension, val);
            break;
          default:
            hlog_error("unexpected tag %d", tag);
            return -1;
        }
        break;
      default:
        hlog_error("error reported");
        return -1;
    }
  } while (type > Receiver::END);
  return rc;
}

int Data::read(
    const char*     path,
    const char*     hash) const {
  Sender sender(_d->sock);
  sender.start();
  sender.write(static_cast<uint8_t>(METHOD), READ);
  sender.write(static_cast<uint8_t>(PATH), path);
  sender.write(static_cast<uint8_t>(HASH), hash);
  if (sender.end() < 0) {
    return -1;
  }
  // Get STATUS
  Receiver        receiver(_d->sock);
  Receiver::Type  type;
  uint8_t         tag;
  size_t          len;
  char            val[65536];
  int             rc = 0;
  do {
    type = receiver.receive(&tag, &len, val);
    switch (type) {
      case Receiver::START:
      case Receiver::END:
        break;
      case Receiver::DATA:
        switch (tag) {
          case STATUS:
            rc = atoi(val);
            break;
          default:
            hlog_error("unexpected tag %d", tag);
            return -1;
        }
        break;
      default:
        hlog_error("error reported");
        return -1;
    }
  } while (type > Receiver::END);
  return rc;
}

Data::WriteStatus Data::write(
    const char*     path,
    char            hash[64],
    int*            comp_level,
    CompressionCase comp_case,
    char*           store_path) const {
  Sender sender(_d->sock);
  sender.start();
  sender.write(static_cast<uint8_t>(METHOD), WRITE);
  sender.write(static_cast<uint8_t>(PATH), path);
  sender.write(static_cast<uint8_t>(COMPRESSION_LEVEL), *comp_level);
  sender.write(static_cast<uint8_t>(COMPRESSION_CASE), comp_case);
  if (store_path != NULL) {
    sender.write(static_cast<uint8_t>(STORE_PATH), NULL, 0);
  }
  if (sender.end() < 0) {
    hlog_error("%s sending message", strerror(errno));
    return error;
  }
  // Get STATUS
  Receiver        receiver(_d->sock);
  Receiver::Type  type;
  uint8_t         tag;
  size_t          len;
  char            val[65536];
  WriteStatus     status = error;
  do {
    type = receiver.receive(&tag, &len, val);
    switch (type) {
      case Receiver::START:
      case Receiver::END:
        break;
      case Receiver::DATA:
        switch (tag) {
          case STATUS:
            status = static_cast<WriteStatus>(atoi(val));
            break;
          case HASH:
            strcpy(hash, val);
            break;
          case COMPRESSION_LEVEL:
            *comp_level = atoi(val);
            break;
          case STORE_PATH:
            strcpy(store_path, val);
            break;
          default:
            hlog_error("unexpected tag %d", tag);
            return error;
        }
        break;
      default:
        hlog_error("error reported");
        return error;
    }
  } while (type > Receiver::END);
  return status;
}

int Data::remove(
    const char*     hash) const {
  Sender sender(_d->sock);
  sender.start();
  sender.write(static_cast<uint8_t>(METHOD), REMOVE);
  sender.write(static_cast<uint8_t>(HASH), hash);
  if (sender.end() < 0) {
    return -1;
  }
  // Get STATUS
  Receiver        receiver(_d->sock);
  Receiver::Type  type;
  uint8_t         tag;
  size_t          len;
  char            val[65536];
  int             rc = 0;
  do {
    type = receiver.receive(&tag, &len, val);
    switch (type) {
      case Receiver::START:
      case Receiver::END:
        break;
      case Receiver::DATA:
        switch (tag) {
          case STATUS:
            rc = atoi(val);
            break;
          default:
            hlog_error("unexpected tag %d", tag);
            return -1;
        }
        break;
      default:
        hlog_error("error reported");
        return -1;
    }
  } while (type > Receiver::END);
  return rc;
}

int Data::crawl(
    bool            thorough,
    bool            repair,
    Collector*      collector) const {
  Sender sender(_d->sock);
  sender.start();
  sender.write(static_cast<uint8_t>(METHOD), CRAWL);
  if (thorough) sender.write(static_cast<uint8_t>(THOROUGH), NULL, 0);
  if (repair) sender.write(static_cast<uint8_t>(REPAIR), NULL, 0);
  if (collector != NULL) sender.write(static_cast<uint8_t>(COLLECTOR), NULL, 0);
  if (sender.end() < 0) {
    return -1;
  }
  // Wait for answer! (STATUS, COLLECTOR)
  // Get STATUS
  Receiver        receiver(_d->sock);
  Receiver::Type  type;
  uint8_t         tag;
  size_t          len;
  char            val[65536];
  int             rc = 0;
  do {
    type = receiver.receive(&tag, &len, val);
    char      hash[256] = "";
    long long data_size = -1;
    long long file_size = -1;
    switch (type) {
      case Receiver::START:
      case Receiver::END:
        break;
      case Receiver::DATA:
        switch (tag) {
          case STATUS:
            rc = atoi(val);
            break;
          case COLLECTOR_HASH:
            strcpy(hash, val);
            break;
          case COLLECTOR_DATA:
            if (sscanf(val, "%lld", &data_size) < 1) {
              data_size = -1;
            }
            break;
          case COLLECTOR_FILE:
            if (sscanf(val, "%lld", &file_size) == 1) {
              collector->add(hash, data_size, file_size);
            }
            break;
          default:
            hlog_error("unexpected tag %d", tag);
            return -1;
        }
        break;
      default:
        hlog_error("error reported");
        return -1;
    }
  } while (type > Receiver::END);
  return rc;
}
