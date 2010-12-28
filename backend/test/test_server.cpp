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
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <report.h>
#include <socket.h>
#include "protocol.h"

#include "data_common.h"

using namespace htools;
using namespace hbackend;

struct AllFields {
  Methods method;
  char    path[PATH_MAX];
  char    hash[256];
  char    ext[16];
  bool    ext_wanted;
};

int main(void) {
  report.setLevel(info);
  Socket sock("localhost", 12345);
  if (sock.listen(1) < 0) {
    hlog_error("%s listening", strerror(errno));
  }
  if (sock.open() < 0) {
    hlog_error("%s creating socket", strerror(errno));
    return 0;
  }

  Receiver        receiver(sock);
  Sender          sender(sock);
  Receiver::Type  type;
  bool            started = false;
  AllFields       fields;
  do {
    uint8_t tag;
    size_t  len;
    char    val[65536];
    type = receiver.receive(&tag, &len, val);
    if (tag != 0) {
      val[len] = '\0';
    }
    hlog_regression("receive: type=%d tag=%d, len=%d, value='%s'",
      type, tag, len, ((len > 0) || (tag == 0)) ? val : "");
    switch (type) {
      case Receiver::START:
        if (started) {
          hlog_error("already started");
        } else {
          hlog_debug("start");
        }
        started = true;
        break;
      case Receiver::END:
        if (! started) {
          hlog_error("not started");
        } else {
          hlog_debug("end");
        }
        started = false;
        switch (fields.method) {
          case NAME:
            sender.start();
            sender.write(static_cast<uint8_t>(STATUS), 0);
            sender.write(static_cast<uint8_t>(PATH),
                "data/01/23/45/56789-0/data");
            sender.write(static_cast<uint8_t>(EXTENSION), ".gz");
            if (sender.end() < 0) {
              hlog_info("%s replying", strerror(errno));
              return 0;
            }
            break;
          case WRITE:
            sender.start();
            sender.write(static_cast<uint8_t>(STATUS), 0);
            sender.write(static_cast<uint8_t>(HASH), "987654321-0");
            sender.write(static_cast<uint8_t>(COMPRESSION_LEVEL), 8);
            if (sender.end() < 0) {
              hlog_info("%s replying", strerror(errno));
              return 0;
            }
            break;
        }
        break;
      case Receiver::DATA:
        if (! started) {
          hlog_error("not started");
        } else {
          hlog_debug("server: received tag %d, data: '%s'", tag, val);
          switch (tag) {
            case METHOD:
              fields.method = static_cast<Methods>(atoi(val));
              switch (fields.method) {
                case NAME:
                  fields.ext_wanted = false;
                  break;
              };
              break;
            case HASH:
              strcpy(fields.hash, val);
              break;
            case EXTENSION:
              fields.ext_wanted = true;
              break;
          };
        }
        break;
      default:
        if (len > 0) {
          hlog_error("reception error");
        } else {
          hlog_verbose("connection closed by client");
        }
    };
  } while (type > Receiver::ERROR);

  sock.close();
  hlog_debug("server process ended");
  return 0;
}
