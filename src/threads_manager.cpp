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

struct ThreadsManagerData;

struct WorkerThreadData {
  ThreadsManagerData&        parent;
  pthread_t                 tid;
  char                      name[NAME_SIZE];
  Queue                     q_in;
  time_t                    last_run;
  WorkerThreadData(ThreadsManagerData& p, const char* n)
  : parent(p), q_in(n) {
    strcpy(name, n);
    q_in.open();
  }
};

struct ThreadsManagerData {
  // Parameters
  char                      name[NAME_SIZE];
  Queue*                    q_in;
  Queue*                    q_out;
  ThreadsManager::routine_f  routine;
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
  // Local data
  pthread_mutex_t           threads_list_lock;
  list<WorkerThreadData*>   busy_threads;
  list<WorkerThreadData*>   idle_threads;
  ThreadsManagerData(Queue* in, Queue* out)
  : q_in(in), q_out(out), time_out(600), running(false), callback(NULL) {
    pthread_mutex_init(&threads_list_lock, NULL);
  }
  ~ThreadsManagerData() {
    pthread_mutex_destroy(&threads_list_lock);
  }
};

struct ThreadsManager::Private {
  ThreadsManagerData         data;
  pthread_t                 monitor_tid;
  Private(Queue* in, Queue* out) : data(in, out) {}
};

static void worker_thread_activity_callback(WorkerThreadData* d, bool idle) {
  ThreadsManagerData& data = d->parent;
  hlog_regression("%s.%s.activity_callback: %s (%zu/%zu busy)",
    d->parent.name, d->name, idle ? "idle" : "busy",
    d->parent.busy_threads.size(),
    d->parent.busy_threads.size() + d->parent.idle_threads.size());
  if (data.callback != NULL) {
    if (idle) {
      if (data.busy_threads.empty()) {
        data.callback_called_idle = true;
        data.callback(true, data.callback_user);
      }
    } else {
      if (data.callback_called_idle) {
        data.callback_called_idle = false;
        data.callback(false, data.callback_user);
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
    void* data_in;
    if (d->q_in.pop(&data_in) == 0) {
      // Work
      worker_thread_activity_callback(d, false);
      void* data_out = d->parent.routine(data_in, d->parent.user);
      if ((d->parent.q_out != NULL) && (data_out != NULL)) {
        d->parent.q_out->push(data_out);
      }
      // Lock before checking that we're still running
      pthread_mutex_lock(&d->parent.threads_list_lock);
      if (d->parent.running) {
        d->parent.busy_threads.remove(d);
        // Put idle thread at the back of the list, if not shutting down
        d->parent.idle_threads.push_back(d);
      }
      pthread_mutex_unlock(&d->parent.threads_list_lock);
      // Slack
      d->last_run = time(NULL);
      worker_thread_activity_callback(d, true);
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
      hlog_debug("%s.loop %zu busy %zu idle %zu total", d->name,
        d->busy_threads.size(), d->idle_threads.size(), d->threads);
      if (! d->idle_threads.empty()) {
        wtd = d->idle_threads.back();
        d->idle_threads.pop_back();
        // Always push to back so longest running threads are at the front
        d->busy_threads.push_back(wtd);
        // Get rid of oldest idle thread if too old
        list<WorkerThreadData*>::iterator it = d->idle_threads.begin();
        if (it != d->idle_threads.end()) {
          hlog_debug("%s.%s age %ld, t-o %ld", d->name, (*it)->name,
            time(NULL) - (*it)->last_run, d->time_out);
          if ((time(NULL) - (*it)->last_run) > d->time_out) {
            hlog_verbose("%s.%s.thread destroyed", d->name, (*it)->name);
            (*it)->q_in.close();
            pthread_join((*it)->tid, NULL);
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
          hlog_verbose("%s.%s.thread created", d->name, wtd->name);
        } else {
          hlog_error("%s creating thread '%s'", strerror(-rc), d->name);
          delete wtd;
          wtd = d->busy_threads.front();
          d->busy_threads.pop_front();
          d->busy_threads.push_back(wtd);
        }
      } else
      {
        // Give longest running thread, and push it back
        wtd = d->busy_threads.front();
        d->busy_threads.pop_front();
        d->busy_threads.push_back(wtd);
      }
      // Put data in thread queue
      wtd->q_in.push(data_in);
      pthread_mutex_unlock(&d->threads_list_lock);
    } else {
      hlog_regression("%s.queue closed", d->name);
      pthread_mutex_lock(&d->threads_list_lock);
      d->running = false;
      // Stop all idle threads
      list<WorkerThreadData*>::iterator it = d->idle_threads.begin();
      while (it != d->idle_threads.end()) {
        (*it)->q_in.close();
        hlog_regression("%s.%s.queue closed", d->name, (*it)->name);
        pthread_join((*it)->tid, NULL);
        hlog_regression("%s.%s.thread joined", d->name, (*it)->name);
        delete *it;
        it = d->idle_threads.erase(it);
      }
      hlog_regression("%s.all idle worker thread(s) destroyed", d->name);
      // Close all busy threads queues
      for (list<WorkerThreadData*>::iterator it = d->busy_threads.begin();
           it != d->busy_threads.end(); ++it) {
        (*it)->q_in.close();
        hlog_regression("%s.%s.queue closed", d->name, (*it)->name);
      }
      hlog_regression("%s all %zu busy worker queue(s) closed", d->name,
        d->busy_threads.size());
      // Wait for all to have stopped (not put in idle list when shutting down)
      it = d->busy_threads.begin();
      while (it != d->busy_threads.end()) {
        // Unlock so threads can finish
        pthread_mutex_unlock(&d->threads_list_lock);
        pthread_join((*it)->tid, NULL);
        if (hlog_is_worth(regression)) {
          usleep(100000);
          hlog_regression("%s.%s.thread joined", d->name, (*it)->name);
        }
        delete *it;
        pthread_mutex_lock(&d->threads_list_lock);
        it = d->busy_threads.erase(it);
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

ThreadsManager::ThreadsManager(const char* name, routine_f routine, void* user,
    Queue* out)
  : in(name), _d(new Private(&in, out)) {
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
    _d->data.running = true;
    _d->data.threads = 0;
    hlog_regression("%s.thread created", _d->data.name);
  }
  if (_d->data.callback != NULL) {
    _d->data.callback_called_idle = true;
    _d->data.callback(true, _d->data.callback_user);
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
