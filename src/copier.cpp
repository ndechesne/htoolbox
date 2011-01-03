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

#include <errno.h>
#include <string.h>

#include <list>

using namespace std;

#include <report.h>
#include "multiwriter.h"
#include "asyncwriter.h"
#include "copier.h"

using namespace htoolbox;

struct Copier::Private {
  bool          failed;
  MultiWriter*  writer;
  size_t        buffer_size;
  void*         buffer1;
  void*         buffer2;
  void*         buffer;
  Private(size_t size) : writer(NULL), buffer1(NULL), buffer2(NULL) {
    buffer_size = size;
    buffer1 = malloc(buffer_size);
    buffer2 = malloc(buffer_size);
    buffer = buffer1;
  }
  ~Private() {
    if (writer != NULL) {
      delete writer;
    }
    free(buffer1);
    free(buffer2);
  }
};

Copier::Copier(IReaderWriter* child, bool delete_child, size_t size) :
  IReaderWriter(child, delete_child), _d(new Private(size)) {}

Copier::~Copier() {
  delete _d;
}

void Copier::addDest(IReaderWriter* child, bool delete_child) {
#if 0
if (_d->writer == NULL) {
  _d->writer = new MultiWriter(child, delete_child);
} else {
  _d->writer->add(child, delete_child);
}
return;
#endif
  AsyncWriter* c = new AsyncWriter(child, delete_child);
  if (_d->writer == NULL) {
    _d->writer = new MultiWriter(c, true);
  } else {
    _d->writer->add(c, true);
  }
}

int Copier::open() {
  _d->failed = false;
  if (_d->writer == NULL) {
    _d->failed = true;
    return -1;
  }
  if (_child->open() < 0) {
    _d->failed = true;
    return -1;
  }
  if (_d->writer->open() < 0) {
    _child->close();
    _d->failed = true;
    return -1;
  }
  return 0;
}

int Copier::close() {
  if ((_child->close() < 0) || (_d->writer->close() < 0)) {
    _d->failed = true;
  }
  return _d->failed ? -1 : 0;
}

ssize_t Copier::stream(void* buffer, size_t max_size) {
  if ((max_size == 0) || (max_size > _d->buffer_size)) {
    max_size = _d->buffer_size;
  }
  ssize_t size = _child->read(_d->buffer, max_size);
  if (size <= 0) {
    if (size < 0) {
      _d->failed = true;
    }
    return size;
  }
  if (buffer != NULL) {
    memcpy(buffer, _d->buffer, size);
  }
  if (_d->writer->write(_d->buffer, size) < 0) {
    _d->failed = true;
    return -1;
  }
  // Swap unused buffers
  if (_d->buffer == _d->buffer1) {
    _d->buffer = _d->buffer2;
  } else {
    _d->buffer = _d->buffer1;
  }
  return _d->failed ? -1 : size;
}

ssize_t Copier::read(void* buffer, size_t size) {
  char* cbuffer = static_cast<char*>(buffer);
  ssize_t rc;
  size_t  count = 0;
  do {
    // size will be BUFFER_SIZE unless the end of file has been reached
    rc = stream(cbuffer != NULL ? &cbuffer[count] : NULL, size - count);
    if (rc < 0) {
      return -1;
    } else
    if (rc == 0) {
      break;
    }
    count += rc;
  } while ((size == 0) || (count < size));
  return count;
}

ssize_t Copier::Copier::write(const void* buffer, size_t size) {
  return _d->writer->write(buffer, size);
}
