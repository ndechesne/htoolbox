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

#include <openssl/md5.h>
#include <openssl/evp.h>

#include "md5sumhasher.h"

using namespace hbackup;

struct MD5SumHasher::Private {
  IReader*   reader;
  IWriter*   writer;
  char*      checksum;
  EVP_MD_CTX ctx;
  Private(char *c) : checksum(c) {}
  void binToHex(char* out, const unsigned char* in, int bytes) {
    const char* hex = "0123456789abcdef";

    while (bytes-- != 0) {
      *out++ = hex[*in >> 4];
      *out++ = hex[*in & 0xf];
      in++;
    }
    *out = '\0';
  }
  int update(const void* buffer, size_t size);
};

int MD5SumHasher::Private::update(
    const void*     buffer,
    size_t          size) {
  size_t      max = 409600; // That's as much as openssl/md5 accepts
  const char* cbuffer = static_cast<const char*>(buffer);
  while (size > 0) {
    size_t length;
    if (size >= max) {
      length = max;
    } else {
      length = size;
    }
    if (EVP_DigestUpdate(&ctx, cbuffer, length) != 1) {
      return -1;
    }
    cbuffer += length;
    size   -= length;
  }
  return 0;
}

MD5SumHasher::MD5SumHasher(IReader& r, char* csum) : _d(new Private(csum)) {
  _d->reader = &r;
  _d->writer = NULL;
}

MD5SumHasher::MD5SumHasher(IWriter& w, char* csum) : _d(new Private(csum)) {
  _d->reader = NULL;
  _d->writer = &w;
}

MD5SumHasher::~MD5SumHasher() {
  delete _d;
}

int MD5SumHasher::open() {
  if ((_d->reader != NULL) && (_d->reader->open() < 0)) {
    return -1;
  }
  if ((_d->writer != NULL) && (_d->writer->open() < 0)) {
    return -1;
  }
  if (EVP_DigestInit(&_d->ctx, EVP_md5()) != 1) {
    return -1;
  }
  return 0;
}

int MD5SumHasher::close() {
  unsigned char checksum[36];
  unsigned int  length;

  if (EVP_DigestFinal(&_d->ctx, checksum, &length) != 1) {
    return -1;
  }
  _d->binToHex(_d->checksum, checksum, length);
  if (_d->reader != NULL) {
    return _d->reader->close();
  }
  if (_d->writer != NULL) {
    return _d->writer->close();
  }
  return 0;
}

ssize_t MD5SumHasher::read(void* buffer, size_t size) {
  ssize_t rc = _d->reader->read(buffer, size);
  if (rc < 0) return rc;
  if (_d->update(buffer, rc) < 0) {
    return -1;
  }
  return rc;
}

ssize_t MD5SumHasher::write(const void* buffer, size_t size) {
  ssize_t rc = _d->writer->write(buffer, size);
  if (rc < 0) return rc;
  if (_d->update(buffer, rc) < 0) {
    return -1;
  }
  return rc;
}

long long MD5SumHasher::offset() const {
  return _d->reader->offset();
}
