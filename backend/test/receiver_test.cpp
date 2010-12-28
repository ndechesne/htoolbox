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

#include "socket.h"
#include "protocol.h"

using namespace htools;

int main(void) {
  Socket sock("socket");
  if ((sock.listen(5) < 0) || (sock.open() < 0)) {
    printf("%s opening socket\n", strerror(errno));
    return 0;
  }

  Receiver receiver(sock);
  Receiver::Type type;
  do {
    uint8_t tag;
    size_t  len;
    char    val[65535];
    type = receiver.receive(&tag, &len, val);
    printf("receive: type=%d tag=%d, len=%d, value='%s'\n",
      type, tag, len, ((len > 0) || (tag == 0)) ? val : "");
  } while (type > Receiver::END);

  sock.close();

  return 0;
}
