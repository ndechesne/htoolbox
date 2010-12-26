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

#include <hbackup.h>
#include <data.h>

using namespace hbackup;

int main(void) {
  Data data("data");
  if (data.open() < 0) {
    printf("%s creating socket\n", strerror(errno));
    return 0;
  }

  char hash[256];
  char ext[16];
  int comp_level = 5;
  if (data.name("bullsh1t", hash, ext) < 0) {
    printf("name failed\n");
  }
  if (data.write("../client_test", hash, &comp_level, Data::auto_now, NULL) ==
      Data::error) {
    printf("write failed\n");
  }

  data.close();
  return 0;
}
