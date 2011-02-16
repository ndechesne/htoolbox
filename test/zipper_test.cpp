/*
    Copyright (C) 2010 - 2011  Hervé Fache

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
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <report.h>
#include "filereaderwriter.h"
#include "zipper.h"
#include "hasher.h"

using namespace htoolbox;

int main() {
  report.setLevel(regression);
  {
    ssize_t rc;
    size_t count;
    size_t size = 0;
    // Write-compress file containing random data
    FileReaderWriter fw("random.gz", true);
    char hash[129];
    Zipper zw(&fw, false, 5);
    Hasher hw(&zw, false, Hasher::md5, hash);
    memset(hash, 0, 129);
    if (hw.open() < 0) return 0;
    for (size_t i = 0; i < 2000000; ++i) {
      ++count;
      char byte = static_cast<char>(rand());
      rc = hw.put(&byte, 1);
      if (rc < 0) return 0;
      size += rc;
    }
    if (hw.close() < 0) return 0;
    hlog_regression("wz: count = %zu, size = %zu, hash = %s", count, size, hash);

    // Read-uncompress file
    FileReaderWriter fr("random.gz", false);
    Zipper ur(&fr, false);
    Hasher hr(&ur, false, Hasher::md5, hash);

    count = 0;
    size = 0;
    memset(hash, 0, 129);
    if (hr.open() < 0) return 0;
    do {
      ++count;
      char buffer[1];
      rc = hr.get(buffer, sizeof(buffer));
      if ((rc > 0) && (rc < static_cast<ssize_t>(sizeof(buffer)))) {
        hlog_regression("only read %zd bytes on iteration #%zu", rc, count);
      }
      size += rc;
    } while (rc > 0);
    if (hr.close() < 0) return 0;
    hlog_regression("gu: count = %zu, size = %zu, hash = %s", count, size, hash);

    count = 0;
    size = 0;
    memset(hash, 0, 129);
    if (hr.open() < 0) return 0;
    do {
      ++count;
      char buffer[300000];
      rc = hr.get(buffer, sizeof(buffer));
      if ((rc > 0) && (rc < static_cast<ssize_t>(sizeof(buffer)))) {
        hlog_regression("only read %zd bytes on iteration #%zu", rc, count);
      }
      size += rc;
    } while (rc > 0);
    if (hr.close() < 0) return 0;
    hlog_regression("gu: count = %zu, size = %zu, hash = %s", count, size, hash);

    count = 0;
    size = 0;
    memset(hash, 0, 129);
    if (hr.open() < 0) return 0;
    do {
      ++count;
      char buffer[4000000];
      rc = hr.get(buffer, sizeof(buffer));
      if ((rc > 0) && (rc < static_cast<ssize_t>(sizeof(buffer)))) {
        hlog_regression("only read %zd bytes on iteration #%zu", rc, count);
      }
      size += rc;
    } while (rc > 0);
    if (hr.close() < 0) return 0;
    hlog_regression("gu: count = %zu, size = %zu, hash = %s", count, size, hash);
  }
  (void) system("gunzip random.gz");
  {
    // Read-compress file
    FileReaderWriter fr("random", false);
    Zipper ur(&fr, false, 5);
    char hash[129];
    Hasher hr(&ur, false, Hasher::md5, hash);
    ssize_t rc;
    size_t count;
    size_t size;

    count = 0;
    size = 0;
    memset(hash, 0, 129);
    if (hr.open() < 0) return 0;
    do {
      ++count;
      char buffer[1];
      rc = hr.get(buffer, sizeof(buffer));
      if ((rc > 0) && (rc < static_cast<ssize_t>(sizeof(buffer)))) {
        hlog_regression("only read %zd bytes on iteration #%zu", rc, count);
      }
      size += rc;
    } while (rc > 0);
    if (hr.close() < 0) return 0;
    hlog_regression("gz: count = %zu, size = %zu, hash = %s", count, size, hash);

    count = 0;
    size = 0;
    memset(hash, 0, 129);
    if (hr.open() < 0) return 0;
    do {
      ++count;
      char buffer[300000];
      rc = hr.read(buffer, sizeof(buffer));
      if ((rc > 0) && (rc < static_cast<ssize_t>(sizeof(buffer)))) {
        hlog_regression("rz: only read %zd bytes on iteration #%zu", rc, count);
      }
      size += rc;
    } while (rc > 0);
    if (hr.close() < 0) return 0;
    hlog_regression("count = %zu, size = %zu, hash = %s", count, size, hash);

    char buffer[4000000];
    count = 0;
    size = 0;
    memset(hash, 0, 129);
    if (hr.open() < 0) return 0;
    do {
      ++count;
      rc = hr.get(buffer, sizeof(buffer));
      if ((rc > 0) && (rc < static_cast<ssize_t>(sizeof(buffer)))) {
        hlog_regression("only read %zd bytes on iteration #%zu", rc, count);
      }
      size += rc;
    } while (rc > 0);
    if (hr.close() < 0) return 0;
    hlog_regression("gz: count = %zu, size = %zu, hash = %s", count, size, hash);

    // Write-decompress file containing random data
    FileReaderWriter fw("random", true);
    Zipper zw(&fw, false);
    Hasher hw(&zw, false, Hasher::md5, hash);
    memset(hash, 0, 129);
    if (hw.open() < 0) return 0;
    if (hw.put(buffer, size) < static_cast<ssize_t>(size)) return 0;
    if (hw.close() < 0) return 0;
    hlog_regression("pu: size = %zu, hash = %s", size, hash);
  }
  return 0;
}
