/*
     Copyright (C) 2010-2011  Herve Fache

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <report.h>
#include "filereaderwriter.h"

using namespace htoolbox;

int main() {
  report.setLevel(regression);
  int sys_rc = 0;

  {
    FileReaderWriter fw("testfile", true);
    if (fw.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      char buffer[5000];
      memset(buffer, 0x41, sizeof(buffer));
      ssize_t rc;
      size_t count = 0;
      do {
        size_t size;
        if (count == 0) {
          size = 3000;
        } else {
          size = sizeof(buffer) - count;
        }
        rc = fw.write(&buffer[count], size);
        if (rc < 0) {
          hlog_regression("%s writing file", strerror(errno));
        } else {
          hlog_regression("written %zd bytes (total %lld) to %s",
                          rc, fw.offset(), fw.path());
        }
        count += rc;
      } while (rc > 0);
      if (fw.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      }
    }
    sys_rc = system("md5sum testfile");
  }

  {
    char buffer[6000];
    memset(buffer, 0, sizeof(buffer));
    size_t size = 0;

    FileReaderWriter fr("testfile", false);
    if (fr.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      ssize_t rc;
      size_t count = 0;
      do {
        if (count == 0) {
          size = 4096;
        } else {
          size = sizeof(buffer) - count;
        }
        rc = fr.read(&buffer[count], size);
        if (rc < 0) {
          hlog_regression("%s reading file", strerror(errno));
        } else {
          hlog_regression("read %zd bytes (total %lld) from %s",
                          rc, fr.offset(), fr.path());
        }
        count += rc;
      } while (rc > 0);
      if (fr.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      } else {
        // Use printf to avoid truncation
        printf("file contents: '%s'\n", buffer);
      }
      size = count;
    }

    FileReaderWriter fw("testfile2", true);
    if (fw.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      ssize_t rc;
      rc = fw.write(buffer, size);
      if (rc < 0) {
        hlog_regression("%s writing file", strerror(errno));
      } else {
        hlog_regression("written %zd bytes (total %lld) to %s",
                        rc, fw.offset(), fw.path());
      }
      if (fw.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      }
    }
    sys_rc = system("md5sum testfile2");
  }

  return 0;
}