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

#include <errno.h>
#include <string.h>
#include <pthread.h>

#include <report.h>
#include "asyncwriter.h"

using namespace htoolbox;

struct AsyncWriter::Private {
  IReaderWriter*  child;
  pthread_t       tid;
  const void*     buffer;
  size_t          size;
  bool            failed;
  bool            closing;
  pthread_mutex_t buffer_lock;
  pthread_mutex_t thread_lock;
  Private(IReaderWriter* c) : child(c) {}
};

AsyncWriter::AsyncWriter(IReaderWriter* child, bool delete_child) :
  IReaderWriter(child, delete_child), _d(new Private(child)) {}

AsyncWriter::~AsyncWriter() {
  delete _d;
}

void* AsyncWriter::_write_thread(void* data) {
  AsyncWriter::Private* d = static_cast<AsyncWriter::Private*>(data);
  do {
    /* Wait for data */
    pthread_mutex_lock(&d->thread_lock);
    if (! d->closing) {
      if (d->child->put(d->buffer, d->size) < 0) {
        /* Can't do more than report failures */
        d->failed = true;
      }
    }
    /* Allow write or close to proceed */
    pthread_mutex_unlock(&d->buffer_lock);
  } while (! d->closing);
  return NULL;
}

int AsyncWriter::open() {
  if (_d->child->open() < 0) {
    return -1;
  }
  _d->failed = false;
  _d->closing = false;
  errno = pthread_mutex_init(&_d->buffer_lock, NULL);
  if (errno != 0) {
    hlog_alert("%s initialising buffer mutex", strerror(errno));
    goto failed;
  }
  errno = pthread_mutex_init(&_d->thread_lock, NULL);
  if (errno != 0) {
    hlog_alert("%s initialising thread mutex", strerror(errno));
    goto failed;
  }
  /* The thread must wait for data */
  pthread_mutex_lock(&_d->thread_lock);
  errno = pthread_create(&_d->tid, NULL, _write_thread, _d);
  if (errno != 0) {
    hlog_alert("%s creating thread", strerror(errno));
    goto failed;
  }
  return 0;
failed:
  _d->child->close();
  return -1;
}

int AsyncWriter::close() {
  /* Wait for all data transfers to complete */
  pthread_mutex_lock(&_d->buffer_lock);
  /* Make sure everybody stops */
  _d->closing = true;
  /* Let thread see we are closing */
  pthread_mutex_unlock(&_d->thread_lock);
  /* Wait for thread to exit */
//   pthread_mutex_lock(&_d->buffer_lock);
  pthread_join(_d->tid, NULL);
  /* All done */
  pthread_mutex_destroy(&_d->buffer_lock);
  pthread_mutex_destroy(&_d->thread_lock);
  if (_d->child->close() < 0) {
    return -1;
  }
  return _d->failed ? -1 : 0;
}

ssize_t AsyncWriter::read(void*, size_t) {
  return -1;
}

ssize_t AsyncWriter::get(void*, size_t) {
  return -1;
}

ssize_t AsyncWriter::put(const void* buffer, size_t size) {
  /* Wait for thread to finish */
  pthread_mutex_lock(&_d->buffer_lock);
  if (_d->failed) {
    return -1;
  }
  if (_d->closing) {
    hlog_alert("write called while closed");
    errno = EBADF;
    return -1;
  }
  _d->buffer = buffer;
  _d->size = size;
  /* Unleash thread */
  pthread_mutex_unlock(&_d->thread_lock);
  return size;
}
