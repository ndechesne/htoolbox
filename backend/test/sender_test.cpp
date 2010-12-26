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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <report.h>
#include <unix_socket.h>
#include "protocol.h"

using namespace htools;

int main(void) {
  report.setLevel(regression);
  UnixSocket sock("socket", false);
  if (sock.open() < 0) {
    hlog_error("%s opening socket", strerror(errno));
    return 0;
  }

  Sender sender(sock);
  if (sender.start() < 0) {
    hlog_error("%s writing to socket", strerror(errno));
    return 0;
  }
  sender.write(1, NULL, 0);
  sender.write(0x12, "I am not a stupid protocol!");
  if (sender.end() < 0) {
    hlog_error("%s writing to socket", strerror(errno));
    return 0;
  }

  sock.close();

  return 0;
}
