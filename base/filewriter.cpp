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

#include <filewriter.h>

using namespace hbackup;

struct FileWriter::Private {
  char path[PATH_MAX];
  int  fd;
  Private(const char* p) : fd(-1) {
    strcpy(path, p);
  }
};

FileWriter::FileWriter(const char* path) : _d(new Private(path)) {}

FileWriter::~FileWriter() {
  /* Auto-close on destroy */
  if (_d->fd >= 0) {
    close();
  }
  delete _d;
}

int FileWriter::open() {
  _d->fd = ::open64(_d->path, O_WRONLY|O_CREAT|O_NOATIME|O_LARGEFILE|O_TRUNC, 0666);
  return _d->fd < 0 ? -1 : 0;
}

int FileWriter::close() {
  int rc = ::close(_d->fd);
  _d->fd = -1;
  return rc;
}

ssize_t FileWriter::write(const void* buffer, size_t size) {
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
  }
  return count;
}
