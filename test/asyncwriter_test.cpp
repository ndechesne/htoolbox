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
#include "hasher.h"
#include "asyncwriter.h"
#include "stackhelper.h"
#include "nullwriter.h"
#include "multiwriter.h"

using namespace htoolbox;

int time = 0;

class Pit : public IReaderWriter {
  long long _offset;
public:
  int open() { _offset = 0; return 0; }
  int close() { return 0; }
  ssize_t read(void*, size_t) { return -1; }
  ssize_t get(void*, size_t) { return -1; }
  ssize_t put(const void*, size_t size) {
    usleep(20000);
    hlog_regression("Pit called for %zd at time %d", size, time);
    usleep(20000);
    ++time;
    _offset += size;
    return size;
  }
  const char* path() const { return "pit"; }
  long long offset() const { return _offset; }
};

class Top : public IReaderWriter {
public:
  Top(IReaderWriter* child) : IReaderWriter(child, true) {}
  int open() { return _child->open(); }
  int close() { return _child->close(); }
  ssize_t read(void*, size_t) { return -1; }
  ssize_t get(void*, size_t) { return -1; }
  ssize_t put(const void* buffer, size_t size) {
    hlog_regression("Top called for %zd at time %d", size, time);
    _child->put(buffer, size);
    return size;
  }
};

int main() {
  report.setLevel(regression);

  IReaderWriter* fw = new Pit;
  char hash1[129];
  memset(hash1, 0, sizeof(hash1));
  char hash2[129];
  memset(hash2, 0, sizeof(hash2));
  fw = new Hasher(fw, true, Hasher::md5, hash1);
  fw = new AsyncWriter(fw, true);
  fw = new Hasher(fw, true, Hasher::md5, hash2);
  fw = new Top(fw);
  fw = new StackHelper(fw, true);

  char buffer1[1024];
  for (size_t i = 0; i < sizeof(buffer1); ++i) {
    buffer1[i] = static_cast<char>(rand());
  }
  char buffer2[5000];
  for (size_t i = 0; i < sizeof(buffer2); ++i) {
    buffer2[i] = static_cast<char>(rand());
  }
  char buffer3[100];
  for (size_t i = 0; i < sizeof(buffer3); ++i) {
    buffer3[i] = static_cast<char>(rand());
  }
  char buffer4[1000000];
  for (size_t i = 0; i < sizeof(buffer4); ++i) {
    buffer4[i] = static_cast<char>(rand());
  }

  hlog_regression("path = '%s'", fw->path());
  if (fw->open() < 0) return 0;
  if (fw->put(buffer1, sizeof(buffer1)) < 0) return 0;
  if (fw->put(buffer2, sizeof(buffer2)) < 0) return 0;
  if (fw->put(buffer3, sizeof(buffer3)) < 0) return 0;
  if (fw->put(buffer4, sizeof(buffer4)) < 0) return 0;
  if (fw->close() < 0) return 0;
  hlog_regression("offset = '%lld'", fw->offset());

  hlog_regression("hash1 = '%s'", hash1);
  memset(hash1, 0, sizeof(hash1));
  hlog_regression("hash2 = '%s'", hash2);
  memset(hash2, 0, sizeof(hash2));

  /* Multi write */
  IReaderWriter* fn = new NullWriter;
  char hash3[129];
  memset(hash3, 0, sizeof(hash3));
  fn = new Hasher(fn, true, Hasher::md5, hash3);
  fn = new AsyncWriter(fn, true);
  fn = new Top(fn);

  MultiWriter* fm = new MultiWriter(fw, true);
  fm->add(fn, true);

  hlog_regression("path = '%s'", fm->path());
  if (fm->open() < 0) return 0;
  if (fm->put(buffer1, sizeof(buffer1)) < 0) return 0;
  if (fm->put(buffer2, sizeof(buffer2)) < 0) return 0;
  if (fm->put(buffer3, sizeof(buffer3)) < 0) return 0;
  if (fm->put(buffer4, sizeof(buffer4)) < 0) return 0;
  if (fm->close() < 0) return 0;
  hlog_regression("offset = '%lld'", fm->offset());

  hlog_regression("hash1 = '%s'", hash1);
  memset(hash1, 0, sizeof(hash1));
  hlog_regression("hash2 = '%s'", hash2);
  memset(hash2, 0, sizeof(hash2));
  hlog_regression("hash3 = '%s'", hash3);
  memset(hash3, 0, sizeof(hash3));

  delete fm;
  return 0;
}
