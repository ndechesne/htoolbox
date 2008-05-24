/*
     Copyright (C) 2008  Herve Fache

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

#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

#include "lock.h"

using namespace hbackup;

struct Lock::Private {
  pthread_mutex_t mutex;
  bool            locked;
};

Lock::Lock(bool locked) : _d(new Private) {
  _d->locked = false;
  pthread_mutex_init(&_d->mutex, NULL);
  if (locked) {
    lock();
  }
}

Lock::~Lock() {
  if (_d->locked) {
    pthread_mutex_unlock(&_d->mutex);
  }
  pthread_mutex_destroy(&_d->mutex);
  delete _d;
}

int Lock::lock(int timeout) {
  if (timeout > 0) {
    struct timeval tm;
    struct timespec t;
    gettimeofday(&tm, NULL);
    t.tv_sec  = tm.tv_sec + timeout;
    t.tv_nsec = tm.tv_usec * 1000;
    switch (pthread_mutex_timedlock(&_d->mutex, &t)) {
      case 0:
        break;
      case ETIMEDOUT:
        return 1;
      default:
        return -1;
    };
  } else {
    if (pthread_mutex_lock(&_d->mutex) != 0) {
      return -1;
    }
  }
  _d->locked = true;
  return 0;
}

int Lock::trylock() {
  int rc = pthread_mutex_trylock(&_d->mutex);
  switch (rc) {
    case 0:
      _d->locked = true;
      return 0;
    case EBUSY:
      return 1;
    default:
      return -1;
  };
}

int Lock::release() {
  _d->locked = false;
  return pthread_mutex_unlock(&_d->mutex) != 0 ? -1 : 0;
}

bool Lock::isLocked() {
  return _d->locked;
}