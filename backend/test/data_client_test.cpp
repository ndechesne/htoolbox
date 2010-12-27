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

  char name[256] = "bullsh1t";
  char hash[256] = "";
  char ext[16] = "";
  printf("name:\n-> name = '%s', ext = '%s'\n", name, ext);
  if (data.name(name, hash, ext) < 0) {
    printf("name failed\n");
  } else {
    printf("<- hash = '%s', ext = '%s'\n", hash, ext);
  }

  char path[256] = "../client_test";
  int comp_level = 5;
  printf("write:\n-> path = '%s', comp_level = %d\n", path, comp_level);
  if (data.write(path, hash, &comp_level, Data::auto_now, NULL) ==
      Data::error) {
    printf("write failed\n");
  } else {
    printf("<- hash = '%s', comp_level = %d\n", hash, comp_level);
  }

  data.close();
  return 0;
}
