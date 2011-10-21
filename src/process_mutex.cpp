/*
    Copyright (C) 2011  Hervé Fache

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

#include <arpa/inet.h>
#include <sys/un.h>
#include <limits.h>

#include <report.h>
#include "process_mutex.h"

using namespace htoolbox;

struct ProcessMutex::Private {
  socklen_t           sock_len_;
  struct sockaddr_un* socket_;
  int                 lock_;
};

ProcessMutex::ProcessMutex(const char* lock_name) : _d(new Private) {
  // Create path
  char path[PATH_MAX] = " ";
  size_t shift = 1;
  if (lock_name[0] != '/') {
    char* cwd = get_current_dir_name();
    strcpy(&path[shift], cwd);
    free(cwd);
    shift = strlen(path);
    path[shift++] = '/';
  }
  strcpy(&path[shift], lock_name);
  // Create socket structure
  _d->sock_len_ =
    static_cast<socklen_t>(sizeof(sa_family_t) + strlen(path) + 2);
  _d->socket_ = static_cast<struct sockaddr_un*>(malloc(_d->sock_len_));
  _d->socket_->sun_family = AF_UNIX;
  strcpy(_d->socket_->sun_path, path);
  _d->socket_->sun_path[0] = '\0';
  // Initialise socket fd
  _d->lock_ = -1;
}

ProcessMutex::~ProcessMutex() {
  free(_d->socket_);
  delete _d;
}

int ProcessMutex::lock() {
  if (_d->lock_ >= 0) {
    hlog_error("Trying to lock @%s twice", &_d->socket_->sun_path[1]);
    errno = EBUSY;
    return -1;
  }
  _d->lock_ = socket(_d->socket_->sun_family, SOCK_STREAM, 0);
  if (_d->lock_ < 0) {
    hlog_error("%m creating socket @%s", &_d->socket_->sun_path[1]);
    return -1;
  }
  if (bind(_d->lock_, reinterpret_cast<struct sockaddr*>(_d->socket_),
      _d->sock_len_)) {
    hlog_error("%m locking @%s", &_d->socket_->sun_path[1]);
    return -1;
  }
  hlog_regression("@%s locked", &_d->socket_->sun_path[1]);
  return 0;
}

int ProcessMutex::unlock() {
  if (_d->lock_ < 0) {
    hlog_error("Trying to unlock while not locked");
    errno = EBADF;
    return -1;
  }
  if (close(_d->lock_) < 0) {
    hlog_error("%m unlocking @%s", &_d->socket_->sun_path[1]);
    return -1;
  }
  _d->lock_ = -1;
  hlog_regression("@%s unlocked", &_d->socket_->sun_path[1]);
  return 0;
}
