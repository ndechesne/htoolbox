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

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <list>
#include <queue>

using namespace std;

#include <report.h>
#include <queue.h>

using namespace htoolbox;

struct Queue::Private {
  char            name[64];
  bool            queue_open;
  size_t          max_size;
  queue<void*>    objects;
  // Lock
  pthread_mutex_t queue_lock;
  // Conditions
  pthread_cond_t  pop_cond;
  pthread_cond_t  push_cond;
};

Queue::Queue(const char* name, size_t max_size) :_d(new Private) {
  strncpy(_d->name, name, sizeof(_d->name));
  _d->name[sizeof(_d->name) - 1] = '\0';
  _d->queue_open = false;
  _d->max_size = max_size;
  pthread_mutex_init(&_d->queue_lock, NULL);
  pthread_cond_init(&_d->pop_cond, NULL);
  pthread_cond_init(&_d->push_cond, NULL);
}

Queue::~Queue() {
  // FIXME what if queue is not empty?
  pthread_mutex_destroy(&_d->queue_lock);
  pthread_cond_destroy(&_d->pop_cond);
  pthread_cond_destroy(&_d->push_cond);
  delete _d;
}

void Queue::open() {
  _d->queue_open = true;
}

void Queue::close() {
  hlog_regression("%s.%s enter", _d->name, __FUNCTION__);
  _d->queue_open = false;
  pthread_cond_broadcast(&_d->pop_cond);
  hlog_regression("%s.%s exit", _d->name, __FUNCTION__);
}

void Queue::wait() {
  hlog_regression("%s.%s enter", _d->name, __FUNCTION__);
  pthread_mutex_lock(&_d->queue_lock);
  while (! _d->objects.empty()) {
    hlog_regression("%s.%s wait for queue to empty", _d->name, __FUNCTION__);
    pthread_cond_wait(&_d->push_cond, &_d->queue_lock);
  }
  pthread_mutex_unlock(&_d->queue_lock);
  hlog_regression("%s.%s exit", _d->name, __FUNCTION__);
}

bool Queue::empty() const {
  return _d->objects.empty();
}

size_t Queue::size() const {
  return _d->objects.size();
}

int Queue::push(void* data) {
  hlog_regression("%s.%s enter", _d->name, __FUNCTION__);
  pthread_mutex_lock(&_d->queue_lock);
  // Wait for some space
  while ((_d->objects.size() >= _d->max_size) && _d->queue_open) {
    hlog_regression("%s.%s wait for queue to empty some", _d->name, __FUNCTION__);
    pthread_cond_wait(&_d->push_cond, &_d->queue_lock);
  }
  // Check status
  if (_d->queue_open) {
    // Get data
    _d->objects.push(data);
    // Signal not empty
    pthread_cond_broadcast(&_d->pop_cond);
  }
  pthread_mutex_unlock(&_d->queue_lock);
  int rc = _d->queue_open ? 0 : 1;
  hlog_regression("%s.%s exit: rc = %d", _d->name, __FUNCTION__, rc);
  return rc;
}

int Queue::pop(void** data) {
  hlog_regression("%s.%s enter", _d->name, __FUNCTION__);
  bool queue_flushed = false;
  pthread_mutex_lock(&_d->queue_lock);
  // Wait for some data
  while (_d->objects.empty() && _d->queue_open) {
    hlog_regression("%s.%s wait for queue to fill up some", _d->name, __FUNCTION__);
    pthread_cond_wait(&_d->pop_cond, &_d->queue_lock);
  }
  // Check status
  if (! _d->objects.empty()) {
    *data = _d->objects.front();
    _d->objects.pop();
    if (_d->objects.size() <= _d->max_size) {
      pthread_cond_broadcast(&_d->push_cond);
    }
  } else
  if (! _d->queue_open) {
    queue_flushed = true;
    pthread_cond_broadcast(&_d->push_cond);
  }
  pthread_mutex_unlock(&_d->queue_lock);
  int rc = queue_flushed ? 1 : 0;
  hlog_regression("%s.%s exit: rc = %d", _d->name, __FUNCTION__, rc);
  return rc;
}
