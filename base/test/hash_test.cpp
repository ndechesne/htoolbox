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
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <hreport.h>

using namespace hreport;

#include "md5sumhasher.h"
#include "filereaderwriter.h"

using namespace hbackup;

int main() {
  report.setLevel(regression);

  {
    FileReaderWriter frw("testfile", true);
    char hash[64];
    MD5SumHasher hh(&frw, false, hash);
    IReaderWriter& fd = hh;
    if (fd.open() < 0) {
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
        rc = fd.write(&buffer[count], size);
        if (rc < 0) {
          hlog_regression("%s writing file", strerror(errno));
        } else {
          hlog_regression("written %zd bytes", rc);
        }
        count += rc;
      } while (rc > 0);
      if (fd.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      } else {
        hlog_regression("hash = '%s'", hash);
      }
    }
    system("md5sum testfile");
  }

  {
    char buffer[6000];
    memset(buffer, 0, sizeof(buffer));
    size_t size;

    FileReaderWriter frw("testfile", false);
    char hash[64];
    MD5SumHasher hh(&frw, false, hash);
    IReaderWriter& fd = hh;
    if (fd.open() < 0) {
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
        rc = fd.read(&buffer[count], size);
        if (rc < 0) {
          hlog_regression("%s reading file", strerror(errno));
        } else {
          hlog_regression("read %zd bytes", rc);
        }
        count += rc;
      } while (rc > 0);
      if (fd.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      } else {
        hlog_regression("hash = '%s'", hash);
      }
      size = count;
    }

    FileReaderWriter frw2("testfile2", true);
    MD5SumHasher hh2(&frw2, false, hash);
    IReaderWriter& fd2 = hh2;
    if (fd2.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      ssize_t rc;
      rc = fd2.write(buffer, size);
      if (rc < 0) {
        hlog_regression("%s writing file", strerror(errno));
      } else {
        hlog_regression("written %zd bytes", rc);
      }
      if (fd2.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      } else {
        hlog_regression("hash = '%s'", hash);
      }
    }
    system("md5sum testfile2");
  }

  return 0;
}
