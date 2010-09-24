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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>

#include <filereader.h>

using namespace hbackup;

struct FileReader::Private {
  char      path[PATH_MAX];
  int       fd;
  long long offset;
  Private(const char* p) : fd(-1) {
    strcpy(path, p);
  }
};

FileReader::FileReader(const char* path) : _d(new Private(path)) {}

FileReader::~FileReader() {
  /* Auto-close on destroy */
  if (_d->fd >= 0) {
    close();
  }
  delete _d;
}

int FileReader::open() {
  _d->offset = 0;
  _d->fd = ::open64(_d->path, O_RDONLY|O_NOATIME|O_LARGEFILE);
  return _d->fd;
}

int FileReader::close() {
  int rc = ::close(_d->fd);
  _d->fd = -1;
  return rc;
}

ssize_t FileReader::read(void* buffer, size_t size) {
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

long long FileReader::offset() const {
  return _d->offset;
}
