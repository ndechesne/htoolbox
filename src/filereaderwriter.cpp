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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>

#include <filereaderwriter.h>

using namespace htoolbox;

struct FileReaderWriter::Private {
  char      path[PATH_MAX];
  bool      writer;
  int       fd;
  int64_t   offset;
  Private(const char* p, bool w) : writer(w), fd(-1) {
    strcpy(path, p);
  }
};

FileReaderWriter::FileReaderWriter(const char* path, bool writer) :
  _d(new Private(path, writer)) {}

FileReaderWriter::~FileReaderWriter() {
  /* Auto-close on destroy */
  if (_d->fd >= 0) {
    close();
  }
  delete _d;
}

int FileReaderWriter::open() {
  _d->offset = 0;
  if (_d->writer) {
    _d->fd = ::open64(_d->path, O_WRONLY|O_CREAT|O_LARGEFILE|O_TRUNC, 0666);
  } else {
    _d->fd = ::open64(_d->path, O_RDONLY|O_NOATIME|O_LARGEFILE);
  }
  return _d->fd < 0 ? -1 : 0;
}

int FileReaderWriter::close() {
  int rc = ::close(_d->fd);
  _d->fd = -1;
  return rc;
}

ssize_t FileReaderWriter::read(void* buffer, size_t size) {
  ssize_t rc = ::read(_d->fd, buffer, size);
  if (rc > 0) {
    _d->offset += rc;
  }
  return rc;
}

ssize_t FileReaderWriter::get(void* buffer, size_t size) {
  char* cbuffer = static_cast<char*>(buffer);
  ssize_t ssize = size;
  ssize_t count = 0;
  while (count < ssize) {
    ssize_t rc = ::read(_d->fd, cbuffer, size - count);
    if (rc < 0) {
      return rc;
    }
    if (rc == 0) {
      /* count < size => end of file */
      break;
    }
    cbuffer += rc;
    count += rc;
    _d->offset += rc;
  }
  return count;
}

ssize_t FileReaderWriter::put(const void* buffer, size_t size) {
  const char* cbuffer = static_cast<const char*>(buffer);
  ssize_t ssize = size;
  ssize_t count = 0;
  while (count < ssize) {
    ssize_t rc = ::write(_d->fd, cbuffer, size - count);
    if (rc < 0) {
      return rc;
    }
    if (rc == 0) {
      /* count < size => end of file */
      break;
    }
    cbuffer += rc;
    count += rc;
    _d->offset += rc;
}
  return count;
}

const char* FileReaderWriter::path() const {
  return _d->path;
}

int64_t FileReaderWriter::offset() const {
  return _d->offset;
}
