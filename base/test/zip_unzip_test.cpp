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
#include "filereaderwriter.h"
#include "zipwriter.h"
#include "unzipreader.h"
#include "hasher.h"

using namespace htools;

int main() {
  report.setLevel(regression);

  FileReaderWriter fw("random.gz", true);
  char hash[129];
  ZipWriter zw(&fw, false, 5);
  Hasher hw(&zw, false, Hasher::md5, hash);
  memset(hash, 0, 129);
  if (hw.open() < 0) return 0;
  for (size_t i = 0; i < 2000000; ++i) {
    char byte = static_cast<char>(rand());
    if (hw.write(&byte, 1) < 0) return 0;
  }
  if (hw.close() < 0) return 0;
  hlog_regression("hash = %s", hash);

  FileReaderWriter fr("random.gz", false);
  UnzipReader ur(&fr, false);
  Hasher hr(&ur, false, Hasher::md5, hash);
  ssize_t rc;
  size_t count;

  count = 0;
  memset(hash, 0, 129);
  if (hr.open() < 0) return 0;
  do {
    ++count;
    char buffer[1];
    rc = hr.read(buffer, sizeof(buffer));
    if ((rc > 0) && (rc < sizeof(buffer))) {
      hlog_regression("only read %zd bytes on iteration #%zu", rc, count);
    }
  } while (rc > 0);
  if (hr.close() < 0) return 0;
  hlog_regression("count = %zu, hash = %s", count, hash);

  count = 0;
  memset(hash, 0, 129);
  if (hr.open() < 0) return 0;
  do {
    ++count;
    char buffer[300000];
    rc = hr.read(buffer, sizeof(buffer));
    if ((rc > 0) && (rc < sizeof(buffer))) {
      hlog_regression("only read %zd bytes on iteration #%zu", rc, count);
    }
  } while (rc > 0);
  if (hr.close() < 0) return 0;
  hlog_regression("count = %zu, hash = %s", count, hash);

  count = 0;
  memset(hash, 0, 129);
  if (hr.open() < 0) return 0;
  do {
    ++count;
    char buffer[4000000];
    rc = hr.read(buffer, sizeof(buffer));
    if ((rc > 0) && (rc < sizeof(buffer))) {
      hlog_regression("only read %zd bytes on iteration #%zu", rc, count);
    }
  } while (rc > 0);
  if (hr.close() < 0) return 0;
  hlog_regression("count = %zu, hash = %s", count, hash);

  return 0;
}
