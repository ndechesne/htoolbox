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

#include <list>
#include <queue>

using namespace std;

#include <report.h>
#include <queue.h>
#include <threads_manager.h>

using namespace htoolbox;

enum {
  NAME_SIZE = 1024
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
  // Local data
  pthread_mutex_t           threads_list_lock;
  pthread_cond_t            idle_cond;
  list<WorkerThreadData*>   busy_threads;
  list<WorkerThreadData*>   idle_threads;
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

struct WorkerThreadData {
  ThreadsManagerData&       parent;
  pthread_t                 tid;
  char                      name[NAME_SIZE];
  time_t                    last_run;
  pthread_mutex_t           mutex;
  void*                     data;
  WorkerThreadData(ThreadsManagerData& p, const char* n): parent(p) {
    strcpy(name, n);
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
  void stop() {
    pthread_mutex_unlock(&mutex);
    hlog_regression("%s.%s stopping", parent.name, name);
  }
};

struct ThreadsManager::Private {
  ThreadsManagerData         data;
  pthread_t                 monitor_tid;
  Private(Queue* in, Queue* out) : data(in, out) {}
};

static void worker_thread_activity_callback(
    WorkerThreadData* d,
    bool              idle,
    size_t            idle_threads,
    size_t            busy_threads) {
  ThreadsManagerData& data = d->parent;
  if (idle) {
    hlog_regression("%s.%s.activity_callback: idle, system: %zu busy/%zu total",
      d->parent.name, d->name, busy_threads, idle_threads + busy_threads);
  } else {
    hlog_regression("%s.%s.activity_callback: busy", d->parent.name, d->name);
  }
  if (data.callback != NULL) {
    if (idle) {
      if (busy_threads == 0) {
        data.activityCallback(idle);
      }
    } else {
      if (data.callback_called_idle) {
        data.activityCallback(idle);
      }
    }
  }
}

static void* worker_thread(void* data) {
  WorkerThreadData* d = static_cast<WorkerThreadData*>(data);
  // Loop
  while (true) {
    if (hlog_is_worth(regression)) {
      usleep(200000);
      hlog_regression("%s.%s.loop enter", d->parent.name, d->name);
    }
    pthread_mutex_lock(&d->mutex);
    if (d->data != d) {
      // Work
      worker_thread_activity_callback(d, false, 0, 1);
      void* data_out = d->parent.routine(d->data, d->parent.user);
      if ((d->parent.q_out != NULL) && (data_out != NULL)) {
        d->parent.q_out->push(data_out);
      }
      // Lock before checking that we're still running
      pthread_mutex_lock(&d->parent.threads_list_lock);
      d->parent.busy_threads.remove(d);
      // Put idle thread at the back of the list, if not shutting down
      d->parent.idle_threads.push_back(d);
      pthread_cond_signal(&d->parent.idle_cond);
      size_t idle_threads = d->parent.idle_threads.size();
      size_t busy_threads = d->parent.busy_threads.size();
      d->last_run = time(NULL);
      d->data = d;
      pthread_mutex_unlock(&d->parent.threads_list_lock);
      // Slack
      worker_thread_activity_callback(d, true, idle_threads, busy_threads);
    } else {
      // Exit loop
      hlog_regression("%s.%s.loop exit", d->parent.name, d->name);
      break;
    }
  }
  // Exit
  return NULL;
}

static void* monitor_thread(void* data) {
  ThreadsManagerData* d = static_cast<ThreadsManagerData*>(data);
  size_t order = 0;
  // Loop
  while (true) {
    hlog_regression("%s.loop enter", d->name);
    void* data_in;
    if (d->q_in->pop(&data_in) == 0) {
      pthread_mutex_lock(&d->threads_list_lock);
      // Get first idle thread
      WorkerThreadData* wtd = NULL;
      hlog_regression("%s.loop %zu busy %zu idle %zu total", d->name,
        d->busy_threads.size(), d->idle_threads.size(), d->threads);
      bool wait_for_thread = false;
      if (! d->idle_threads.empty()) {
        wtd = d->idle_threads.back();
        d->idle_threads.pop_back();
        // Always push to back so longest running threads are at the front
        d->busy_threads.push_back(wtd);
        // Get rid of oldest idle thread if too old
        list<WorkerThreadData*>::iterator it = d->idle_threads.begin();
        if (it != d->idle_threads.end()) {
          hlog_regression("%s.%s age %ld, t-o %ld", d->name, (*it)->name,
            time(NULL) - (*it)->last_run, d->time_out);
          if ((time(NULL) - (*it)->last_run) > d->time_out) {
            (*it)->stop();
            pthread_join((*it)->tid, NULL);
            hlog_regression("%s.%s joined", d->name, (*it)->name);
            delete *it;
            d->idle_threads.erase(it);
            --d->threads;
          }
        }
      } else
      // Create new thread if possible
      if (((d->max_threads == 0) || (d->busy_threads.size() < d->max_threads))) {
        // Create worker thread
        char name[NAME_SIZE];
        snprintf(name, NAME_SIZE, "worker%zu", ++order);
        wtd = new WorkerThreadData(*d, name);
        int rc = pthread_create(&wtd->tid, NULL, worker_thread, wtd);
        if (rc == 0) {
          d->busy_threads.push_back(wtd);
          ++d->threads;
          hlog_regression("%s.%s.thread created", d->name, wtd->name);
        } else {
          hlog_error("%s creating thread '%s'", strerror(-rc), d->name);
          delete wtd;
          wait_for_thread = true;
        }
      } else
      {
        wait_for_thread = true;
      }
      if (wait_for_thread) {
        hlog_regression("%s.loop wait for thread to get idle", d->name);
        pthread_cond_wait(&d->idle_cond, &d->threads_list_lock);
        /*if (d->run) */{
          wtd = d->idle_threads.back();
          d->idle_threads.pop_back();
          // Always push to back so longest running threads are at the front
          d->busy_threads.push_back(wtd);
        }
      }
      pthread_mutex_unlock(&d->threads_list_lock);
      // Start worker thread
      wtd->push(data_in);
    } else {
      hlog_regression("%s.queue closed", d->name);
      // Stop all threads
      pthread_mutex_lock(&d->threads_list_lock);
      while (! d->idle_threads.empty() || ! d->busy_threads.empty()) {
        list<WorkerThreadData*>::iterator it = d->idle_threads.begin();
        if (it != d->idle_threads.end()) {
          (*it)->stop();
          pthread_join((*it)->tid, NULL);
          hlog_regression("%s.%s joined", d->name, (*it)->name);
          delete *it;
          it = d->idle_threads.erase(it);
        } else {
          hlog_regression("%s wait for %zu busy worker(s)", d->name,
            d->busy_threads.size());
          pthread_cond_wait(&d->idle_cond, &d->threads_list_lock);
        }
      }
      pthread_mutex_unlock(&d->threads_list_lock);
      hlog_regression("%s all worker thread(s) joined", d->name);
      // Exit loop
      hlog_regression("%s.loop exit", d->name);
      break;
    }
  }
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

int ThreadsManager::stop() {
  if (! _d->data.running) return -1;
  // Stop monitoring thread
  _d->data.q_in->close();
  pthread_join(_d->monitor_tid, NULL);
  hlog_regression("%s.thread joined", _d->data.name);
  _d->data.running = false;

  return 0;
}

size_t ThreadsManager::threads() const {
  return _d->data.threads;
}
