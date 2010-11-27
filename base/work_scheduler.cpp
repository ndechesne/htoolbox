#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <list>
#include <queue>

using namespace std;

#include <hreport.h>
#include <job_queue.h>
#include <work_scheduler.h>

using namespace htools;

enum {
  NAME_SIZE = 64
};

struct WorkSchedulerData;

struct WorkerThreadData {
  WorkSchedulerData&        parent;
  pthread_t                 tid;
  char                      name[NAME_SIZE];
  JobQueue                  q_in;
  bool                      busy;
  WorkerThreadData(WorkSchedulerData& p, const char* n)
  : parent(p), q_in(n) {
    strcpy(name, n);
    q_in.open();
  }
};

struct WorkSchedulerData {
  // Parameters
  JobQueue&                 q_in;
  JobQueue&                 q_out;
  WorkScheduler::routine_f  routine;
  void*                     user;
  size_t                    min_threads;
  size_t                    max_threads;
  // Local data
  pthread_mutex_t           threads_list_lock;
  list<WorkerThreadData*>   threads;
  WorkSchedulerData(JobQueue& in, JobQueue& out) : q_in(in), q_out(out) {
    pthread_mutex_init(&threads_list_lock, NULL);
  }
  ~WorkSchedulerData() {
    pthread_mutex_destroy(&threads_list_lock);
  }
};

struct WorkScheduler::Private {
  bool                      started;
  WorkSchedulerData         data;
  pthread_t                 monitor_tid;
  Private(JobQueue& in, JobQueue& out) : started(false), data(in, out) {}
};

static void* worker_thread(void* data) {
  WorkerThreadData* d = static_cast<WorkerThreadData*>(data);
  // Loop
  while (true) {
    if (hlog_is_worth(regression)) {
      usleep(200000);
      hlog_regression("%s.loop enter", d->name);
    }
    void* data_in;
    if (d->q_in.pop(&data_in) == 0) {
      // Work
      void* data_out = d->parent.routine(data_in, d->parent.user);
      if (data_out != NULL) {
        d->parent.q_out.push(data_out);
      }
      // Put idle thread at the front of the list
      pthread_mutex_lock(&d->parent.threads_list_lock);
      d->parent.threads.remove(d);
      d->parent.threads.push_front(d);
      pthread_mutex_unlock(&d->parent.threads_list_lock);
      d->busy = false;
    } else {
      // Exit loop
      hlog_regression("%s.loop exit", d->name);
      break;
    }
  }
  // Exit
  return NULL;
}

static void* monitor_thread(void* data) {
  WorkSchedulerData* d = static_cast<WorkSchedulerData*>(data);
  size_t order = 0;
  // Loop
  while (true) {
    WorkerThreadData* wtd;
    hlog_regression("monitor.loop enter");
    void* data_in;
    if (d->q_in.pop(&data_in) == 0) {
      pthread_mutex_lock(&d->threads_list_lock);
      if (! d->threads.empty()) {
        wtd = d->threads.front();
      } else {
        wtd = NULL;
      }
      // Create new thread if needed and possible
      if ((wtd == NULL) ||
          (wtd->busy && ((d->max_threads == 0) ||
                         (d->threads.size() < d->max_threads)))) {
        // Create worker thread
        char name[NAME_SIZE];
        snprintf(name, NAME_SIZE, "worker #%zu", ++order);
        wtd = new WorkerThreadData(*d, name);
        if (pthread_create(&wtd->tid, NULL, worker_thread, wtd) == 0) {
          d->threads.push_back(wtd);
          hlog_regression("%s.thread created", wtd->name);
        } else {
          // TODO Report error
          delete wtd;
        }
      }
      // Put data in thread queue
      wtd->busy = true;
      wtd->q_in.push(data_in);
      pthread_mutex_unlock(&d->threads_list_lock);
    } else {
      hlog_regression("monitor.queue closed");
      pthread_mutex_lock(&d->threads_list_lock);
      // Stop all threads
      for (list<WorkerThreadData*>::iterator it = d->threads.begin();
           it != d->threads.end(); ++it) {
        (*it)->q_in.close();
        hlog_regression("%s.queue closed", (*it)->name);
      }
      pthread_mutex_unlock(&d->threads_list_lock);
      hlog_regression("all %zu worker queue(s) closed", d->threads.size());
      // Wait for all to have stopped
      wtd = NULL;
      while (true) {
        pthread_mutex_lock(&d->threads_list_lock);
        list<WorkerThreadData*>::iterator it = d->threads.begin();
        if (it == d->threads.end()) {
          wtd = NULL;
        } else {
          wtd = *it;
        }
        pthread_mutex_unlock(&d->threads_list_lock);
        if (wtd == NULL) {
          break;
        }
        pthread_join(wtd->tid, NULL);
        pthread_mutex_lock(&d->threads_list_lock);
        d->threads.remove(wtd);
        pthread_mutex_unlock(&d->threads_list_lock);
        if (hlog_is_worth(regression)) {
          usleep(100000);
          hlog_regression("%s.thread joined", wtd->name);
        }
      }
      hlog_regression("all worker thread(s) joined");
      // Exit loop
      hlog_regression("monitor.loop exit");
      break;
    }
  }
  // Exit
  return NULL;
}

WorkScheduler::WorkScheduler(
    JobQueue& in, JobQueue& out, routine_f routine, void* user)
  : _d(new Private(in, out)) {
  _d->data.routine = routine;
  _d->data.user = user;
}

WorkScheduler::~WorkScheduler() {
  stop();
  delete _d;
}

int WorkScheduler::start(size_t max_threads, size_t min_threads) {
  if (_d->started) return -1;
  _d->data.min_threads = min_threads;
  _d->data.max_threads = max_threads;
  // Start monitoring thread
  int rc = pthread_create(&_d->monitor_tid, NULL, monitor_thread, &_d->data);
  if (rc == 0) {
    _d->started = true;
    hlog_regression("monitor.thread created");
  }
  return rc;
}

int WorkScheduler::stop() {
  if (! _d->started) return -1;

  // Stop monitoring thread
  _d->data.q_in.close();
  pthread_join(_d->monitor_tid, NULL);
  hlog_regression("monitor.thread joined");
  _d->started = false;

  return 0;
}

size_t WorkScheduler::threads() const {
  return _d->data.threads.size();
}
