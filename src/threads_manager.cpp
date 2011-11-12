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
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include <report.h>
#include <queue.h>
#include <threads_manager.h>

using namespace htoolbox;

enum {
  NAME_SIZE = 1024
};

template <class T>
class Stack {
  T*      _head;
  T*      _tail;
  size_t  _size;
public:
  Stack() : _head(NULL), _tail(NULL), _size(0) {}
  size_t size() const { return _size; }
  bool empty() const { return _size == 0; }
  void push(T* t) {
    t->previous = NULL;
    t->next = _head;
    if (_head != NULL) {
      _head->previous = t;
    } else {
      _tail = t;
    }
    _head = t;
    ++_size;
  }
  T* pop() {
    if (_head == NULL) {
      return NULL;
    }
    T* t = _head;
    _head = t->next;
    if (_head != NULL) {
      _head->previous = NULL;
    } else {
      _tail = NULL;
    }
    --_size;
    return t;
  }
  void remove(T* t) {
    if (t->previous != NULL) {
      t->previous->next = t->next;
    } else {
      _head = t->next;
    }
    if (t->next != NULL) {
      t->next->previous = t->previous;
    } else {
      _tail = t->previous;
    }
    --_size;
  }
  T* bottom() { return _tail; }
};

struct WorkerThreadData;

struct ThreadsManagerData {
  // Parameters
  char                      name[NAME_SIZE];
  Queue*                    q_in;
  Queue*                    q_out;
  ThreadsManager::routine_f routine;
  void*                     user;
  size_t                    min_threads;
  size_t                    max_threads;
  size_t                    threads;
  time_t                    time_out;
  bool                      running;
  // Callback
  ThreadsManager::callback_f callback;
  void*                     callback_user;
  bool                      callback_called_idle;
  pthread_mutex_t           callback_lock;
  // Thread IDs base
  int                       thread_id_base;
  // Local data
  pthread_mutex_t           threads_list_lock;
  pthread_cond_t            idle_cond;
  Stack<WorkerThreadData>   busy_threads;
  Stack<WorkerThreadData>   idle_threads;
  ThreadsManagerData(Queue* in, Queue* out)
  : q_in(in), q_out(out), time_out(600), running(false), callback(NULL) {
    pthread_mutex_init(&callback_lock, NULL);
    pthread_mutex_init(&threads_list_lock, NULL);
    pthread_cond_init(&idle_cond, NULL);
  }
  ~ThreadsManagerData() {
    pthread_cond_destroy(&idle_cond);
    pthread_mutex_destroy(&threads_list_lock);
    pthread_mutex_destroy(&callback_lock);
  }
  void activityCallback(bool idle) {
    pthread_mutex_lock(&callback_lock);
    callback_called_idle = idle;
    callback(idle, callback_user);
    pthread_mutex_unlock(&callback_lock);
  }
};

static void* worker_thread(void* data);

struct WorkerThreadData {
  WorkerThreadData*         previous;
  WorkerThreadData*         next;
  ThreadsManagerData&       parent;
  pthread_t                 tid;
  int                       number;
  time_t                    last_run;
  pthread_mutex_t           mutex;
  void*                     data;
  WorkerThreadData(ThreadsManagerData& p, int n): parent(p), number(n) {
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_lock(&mutex);
  }
  ~WorkerThreadData() {
    pthread_mutex_destroy(&mutex);
  }
  void push(void* d) {
    data = d;
    pthread_mutex_unlock(&mutex);
  }
  int start() {
    hlog_regression("%s.%d starting", parent.name, number);
    return pthread_create(&tid, NULL, worker_thread, this);
  }
  void stop() {
    hlog_regression("%s.%d stopping", parent.name, number);
    pthread_mutex_unlock(&mutex);
    pthread_join(tid, NULL);
    hlog_regression("%s.%d joined", parent.name, number);
  }
};

struct ThreadsManager::Private {
  ThreadsManagerData         data;
  pthread_t                 monitor_tid;
  Private(Queue* in, Queue* out) : data(in, out) {}
};

static void* worker_thread(void* data) {
  WorkerThreadData* d = static_cast<WorkerThreadData*>(data);
  if (d->parent.thread_id_base >= 0) {
    htoolbox::tl_thread_id = d->parent.thread_id_base + d->number;
  }
  // Loop
  while (true) {
    hlog_regression("%s.%d.loop enter", d->parent.name, d->number);
    pthread_mutex_lock(&d->mutex);
    if (d->data != d) {
      hlog_regression("%s.%d.loop has data", d->parent.name, d->number);
      // Work
      void* data_out = d->parent.routine(d->data, d->parent.user);
      if (d->parent.q_out != NULL) {
        d->parent.q_out->push(data_out);
      }
      // Lock before checking that we're still running
      pthread_mutex_lock(&d->parent.threads_list_lock);
      d->parent.busy_threads.remove(d);
      d->parent.idle_threads.push(d);
      pthread_cond_signal(&d->parent.idle_cond);
      d->last_run = time(NULL);
      // Prepare to die
      d->data = d;
      // Slacking
      d->parent.q_in->signal();
      pthread_mutex_unlock(&d->parent.threads_list_lock);
    } else {
      // Exit loop
      hlog_regression("%s.%d.loop exit", d->parent.name, d->number);
      break;
    }
  }
  // Exit
  return NULL;
}

static void* monitor_thread(void* data) {
  ThreadsManagerData* d = static_cast<ThreadsManagerData*>(data);
  if (d->thread_id_base >= 0) {
    htoolbox::tl_thread_id = d->thread_id_base;
  }
  int order = 0;
  bool busy = false;
  // Loop
  bool run = true;
  do {
    hlog_regression("%s.loop enter", d->name);
    void* data_in;
    int q_rc = d->q_in->pop(&data_in);
    pthread_mutex_lock(&d->threads_list_lock);
    if (q_rc == 1) {
      // Activity report
      if (busy) {
        size_t busy_threads = d->busy_threads.size();
        if (busy_threads == 0) {
          hlog_regression("%s.loop: signalled and idle (%zu/%zu)",
            d->name, busy_threads, d->idle_threads.size() + busy_threads);
          busy = false;
          if (d->callback != NULL) {
            d->activityCallback(true);
          }
        } else {
          hlog_regression("%s.loop: signalled but busy (%zu/%zu)",
            d->name, busy_threads, d->idle_threads.size() + busy_threads);
        }
      }
    } else
    if (q_rc == 0) {
      // Get first idle thread
      WorkerThreadData* wtd = NULL;
      hlog_regression("%s.queue has data (%zu/%zu)", d->name,
        d->busy_threads.size(), d->threads);
      bool wait_for_thread = false;
      if (! d->idle_threads.empty()) {
        wtd = d->idle_threads.pop();
        // Always push to back so longest running threads are at the front
        d->busy_threads.push(wtd);
        // Get rid of oldest idle thread if too old
        WorkerThreadData* bottom = d->idle_threads.bottom();
        if (bottom != NULL) {
          hlog_regression("%s.%d age %ld, t-o %ld", d->name, bottom->number,
            time(NULL) - bottom->last_run, d->time_out);
          if ((time(NULL) - bottom->last_run) > d->time_out) {
            d->idle_threads.remove(bottom);
            bottom->stop();
            delete bottom;
            --d->threads;
          }
        }
      } else
      // Create new thread if possible
      if (((d->max_threads == 0) || (d->threads < d->max_threads))) {
        // Create worker thread
        wtd = new WorkerThreadData(*d, ++order);
        int rc = wtd->start();
        if (rc == 0) {
          d->busy_threads.push(wtd);
          ++d->threads;
          hlog_regression("%s.loop thread %d created", d->name, wtd->number);
        } else {
          hlog_error("%s.loop %s creating thread %d", d->name, strerror(-rc),
            wtd->number);
          delete wtd;
          wait_for_thread = true;
        }
      } else
      {
        wait_for_thread = true;
      }
      if (wait_for_thread) {
        hlog_regression("%s.loop wait for a thread to get idle", d->name);
        pthread_cond_wait(&d->idle_cond, &d->threads_list_lock);
        /*if (d->run) */{
          wtd = d->idle_threads.pop();
          d->busy_threads.push(wtd);
        }
      }
      // Start worker thread
      wtd->push(data_in);
      // Activity report
      hlog_regression("%s.loop: %s busy (%zu/%zu)",
        d->name, busy ? "remaining" : "becoming",
        d->busy_threads.size(), d->idle_threads.size() + d->busy_threads.size());
      if (! busy) {
        busy = true;
        if (d->callback != NULL) {
          d->activityCallback(false);
        }
      }
    } else
    {
      hlog_regression("%s.loop queue closed", d->name);
      // Stop all threads
      while (d->threads > 0) {
        WorkerThreadData* bottom = d->idle_threads.bottom();
        if (bottom != NULL) {
          hlog_regression("%s.loop stop idle thread %d (%zu/%zu)",
            d->name, bottom->number, d->busy_threads.size(), d->threads);
          d->idle_threads.remove(bottom);
          bottom->stop();
          delete bottom;
          --d->threads;
        } else {
          hlog_regression("%s.loop wait for busy thread(s) (%zu/%zu)", d->name,
            d->busy_threads.size(), d->threads);
          pthread_cond_wait(&d->idle_cond, &d->threads_list_lock);
          // Activity report
          if (busy) {
            size_t busy_threads = d->busy_threads.size();
            if (busy_threads == 0) {
              hlog_regression("%s.loop: closing and idle (%zu/%zu)",
                d->name, busy_threads, d->idle_threads.size() + busy_threads);
              busy = false;
              if (d->callback != NULL) {
                d->activityCallback(true);
              }
            } else {
              hlog_regression("%s.loop: closing but busy (%zu/%zu)",
                d->name, busy_threads, d->idle_threads.size() + busy_threads);
            }
          }
        }
      }
      // Exit loop
      hlog_regression("%s.loop exit", d->name);
      run = false;
    }
    pthread_mutex_unlock(&d->threads_list_lock);
  } while (run);
  // Exit
  return NULL;
}

ThreadsManager::ThreadsManager(
    const char* name,
    routine_f   routine,
    void*       user,
    size_t      q_in_size,
    Queue*      q_out) : in(name, q_in_size), _d(new Private(&in, q_out)) {
  strncpy(_d->data.name, name, NAME_SIZE);
  _d->data.name[NAME_SIZE - 1] ='\0';
  _d->data.routine = routine;
  _d->data.user = user;
  _d->data.thread_id_base = -1;
}

ThreadsManager::~ThreadsManager() {
  stop();
  delete _d;
}

void ThreadsManager::setActivityCallback(callback_f callback, void* user) {
  _d->data.callback = callback;
  _d->data.callback_user = user;
  _d->data.callback_called_idle = false;
}

void ThreadsManager::setThreadIdBase(int thread_id_base) {
  _d->data.thread_id_base = thread_id_base;
}

int ThreadsManager::start(size_t max_threads, size_t min_threads, time_t time_out) {
  if (_d->data.running) return -1;
  _d->data.q_in->open();
  _d->data.min_threads = min_threads;
  _d->data.max_threads = max_threads;
  _d->data.time_out = time_out;
  // Start monitoring thread
  int rc = pthread_create(&_d->monitor_tid, NULL, monitor_thread, &_d->data);
  if (rc == 0) {
    _d->data.threads = 0;
    if (_d->data.callback != NULL) {
      _d->data.activityCallback(true);
    }
    _d->data.running = true;
    hlog_regression("%s.thread created", _d->data.name);
  }
  return rc;
}

int ThreadsManager::stop(bool urgent) {
  if (! _d->data.running) return -1;
  // Stop monitoring thread
  _d->data.q_in->close(urgent);
  pthread_join(_d->monitor_tid, NULL);
  hlog_regression("%s.thread joined", _d->data.name);
  _d->data.running = false;

  return 0;
}

size_t ThreadsManager::threads() const {
  return _d->data.threads;
}
