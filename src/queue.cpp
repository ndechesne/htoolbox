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

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <report.h>
#include <queue.h>

using namespace htoolbox;

struct Queue::Private {
  char            name[64];
  bool            is_open;
  const size_t    max_size;
  size_t          size;
  void**          objects;
  void**          reader;
  void**          writer;
  void**          end;
  // Lock
  pthread_mutex_t queue_lock;
  // Conditions
  pthread_cond_t  pop_cond;
  pthread_cond_t  push_cond;
  Private(size_t n) : is_open(false), max_size(n), size(0) {
    objects = static_cast<void**>(malloc(n * sizeof(void*)));
    end = &objects[max_size];
  }
  ~Private() {
    free(objects);
  }
  void open() {
    reader = &objects[0];
    writer = &objects[0];
    is_open = true;
  }
  void close() {
    is_open = false;
  }
  // Must not be full!
  void push(void* d) {
    *reader++ = d;
    ++size;
    if (reader == end) {
      reader = &objects[0];
    }
  }
  // Must not be empty!
  void* pop() {
    void* rc = *writer++;
    --size;
    if (writer == end) {
      writer = &objects[0];
    }
    return rc;
  }
};

Queue::Queue(const char* name, size_t max_size) :_d(new Private(max_size)) {
  strncpy(_d->name, name, sizeof(_d->name));
  _d->name[sizeof(_d->name) - 1] = '\0';
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
  _d->open();
}

void Queue::close() {
  hlog_regression("%s.%s enter", _d->name, __FUNCTION__);
  _d->close();
  pthread_cond_broadcast(&_d->pop_cond);
  hlog_regression("%s.%s exit", _d->name, __FUNCTION__);
}

void Queue::wait() {
  hlog_regression("%s.%s enter", _d->name, __FUNCTION__);
  pthread_mutex_lock(&_d->queue_lock);
  while (_d->size > 0) {
    hlog_regression("%s.%s wait for queue to empty", _d->name, __FUNCTION__);
    pthread_cond_wait(&_d->push_cond, &_d->queue_lock);
  }
  pthread_mutex_unlock(&_d->queue_lock);
  hlog_regression("%s.%s exit", _d->name, __FUNCTION__);
}

bool Queue::empty() const {
  return _d->size == 0;
}

size_t Queue::size() const {
  return _d->size;
}

int Queue::push(void* data) {
  hlog_regression("%s.%s enter", _d->name, __FUNCTION__);
  pthread_mutex_lock(&_d->queue_lock);
  // Wait for some space
  while ((_d->size == _d->max_size) && _d->is_open) {
    hlog_regression("%s.%s wait for queue to empty some", _d->name, __FUNCTION__);
    pthread_cond_wait(&_d->push_cond, &_d->queue_lock);
  }
  // Check status
  if (_d->is_open) {
    // Get data
    _d->push(data);
    // Signal not empty
    pthread_cond_broadcast(&_d->pop_cond);
  }
  pthread_mutex_unlock(&_d->queue_lock);
  int rc = _d->is_open ? 0 : 1;
  hlog_regression("%s.%s exit: rc = %d", _d->name, __FUNCTION__, rc);
  return rc;
}

int Queue::pop(void** data) {
  hlog_regression("%s.%s enter", _d->name, __FUNCTION__);
  bool queue_flushed = false;
  pthread_mutex_lock(&_d->queue_lock);
  // Wait for some data
  while ((_d->size == 0) && _d->is_open) {
    hlog_regression("%s.%s wait for queue to fill up some", _d->name, __FUNCTION__);
    pthread_cond_wait(&_d->pop_cond, &_d->queue_lock);
  }
  // Check status
  if (_d->size > 0) {
    *data = _d->pop();
    pthread_cond_broadcast(&_d->push_cond);
  } else
  if (! _d->is_open) {
    queue_flushed = true;
    pthread_cond_broadcast(&_d->push_cond);
  }
  pthread_mutex_unlock(&_d->queue_lock);
  int rc = queue_flushed ? 1 : 0;
  hlog_regression("%s.%s exit: rc = %d", _d->name, __FUNCTION__, rc);
  return rc;
}
