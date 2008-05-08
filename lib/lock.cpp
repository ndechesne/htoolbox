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
#include <errno.h>

#include "lock.h"

using namespace hbackup;

struct Lock::Private {
  pthread_mutex_t mutex;
};

Lock::Lock(bool locked) : _d(new Private) {
  pthread_mutex_init(&_d->mutex, NULL);
  if (locked) {
    pthread_mutex_lock(&_d->mutex);
  }
}

Lock::~Lock() {
  pthread_mutex_destroy(&_d->mutex);
  delete _d;
}

#include <stdio.h>
int Lock::lock(int timeout) {
  if (timeout > 0) {
    struct timespec t;
    t.tv_sec  = time(NULL) + timeout + 1;
    t.tv_nsec = 0;
    int rc = pthread_mutex_timedlock(&_d->mutex, &t);
    switch (rc) {
      case 0:
        return 0;
      case ETIMEDOUT:
        return 1;
      default:
        return -1;
    };
  }
  return pthread_mutex_lock(&_d->mutex) != 0 ? -1 : 0;
}

int Lock::trylock() {
  int rc = pthread_mutex_trylock(&_d->mutex);
  switch (rc) {
    case 0:
      return 0;
    case EBUSY:
      return 1;
    default:
      return -1;
  };
}

int Lock::unlock() {
  return pthread_mutex_unlock(&_d->mutex) != 0 ? -1 : 0;
}
