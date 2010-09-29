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

#include <openssl/evp.h>

#include <hreport.h>
#include "hasher.h"

using namespace htools;

struct Hasher::Private {
  IReaderWriter* child;
  bool           delete_child;
  Digest         digest;
  char*          hash;
  EVP_MD_CTX ctx;
  Private(IReaderWriter* c, bool d, Digest m, char *h) :
    child(c), delete_child(d), digest(m), hash(h) {}
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

int Hasher::Private::update(
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
      hlog_alert("failed to update hasher");
      return -1;
    }
    cbuffer += length;
    size   -= length;
  }
  return 0;
}

Hasher::Hasher(IReaderWriter* c, bool d, Digest m, char* h) :
  _d(new Private(c, d, m, h)) {}

Hasher::~Hasher() {
  if (_d->delete_child) {
    delete _d->child;
  }
  delete _d;
}

int Hasher::open() {
  if (_d->child->open() < 0) {
    return -1;
  }
  const EVP_MD* digest;
  switch (_d->digest) {
    case md_null:
      digest = EVP_md_null();
      break;
    case md2:
      digest = EVP_md2();
      break;
    case md4:
      digest = EVP_md4();
      break;
    case md5:
      digest = EVP_md5();
      break;
    case sha:
      digest = EVP_sha();
      break;
    case sha1:
      digest = EVP_sha1();
      break;
    case dss:
      digest = EVP_dss();
      break;
    case sha224:
      digest = EVP_sha224();
      break;
    case sha256:
      digest = EVP_sha256();
      break;
    case sha384:
      digest = EVP_sha384();
      break;
    case sha512:
      digest = EVP_sha512();
      break;
    case dss1:
      digest = EVP_dss1();
      break;
    case ripemd160:
      digest = EVP_ripemd160();
      break;
    default:
      goto err;
  }
  if (EVP_DigestInit(&_d->ctx, digest) != 1) {
    hlog_alert("failed to intialise hasher");
    goto err;
  }
  return 0;
err:
  _d->child->close();
  return -1;
}

int Hasher::close() {
  unsigned char hash[64];
  unsigned int  length;

  int rc = 0;
  if (EVP_DigestFinal(&_d->ctx, hash, &length) != 1) {
    hlog_alert("failed to finalise hasher");
    rc = -1;
  } else {
    _d->binToHex(_d->hash, hash, length);
  }
  if (_d->child->close() < 0) {
    rc = -1;
  }
  return rc;
}

ssize_t Hasher::read(void* buffer, size_t size) {
  ssize_t rc = _d->child->read(buffer, size);
  if (rc < 0) {
    return -1;
  }
  if (_d->update(buffer, rc) < 0) {
    return -1;
  }
  return rc;
}

ssize_t Hasher::write(const void* buffer, size_t size) {
  ssize_t rc = _d->child->write(buffer, size);
  if (rc < 0) {
    return -1;
  }
  if (_d->update(buffer, rc) < 0) {
    return -1;
  }
  return rc;
}
