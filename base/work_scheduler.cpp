#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
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
  time_t                    last_run;
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
  size_t                    threads;
  time_t                    time_out;
  bool                      running;
  // Local data
  pthread_mutex_t           threads_list_lock;
  list<WorkerThreadData*>   busy_threads;
  list<WorkerThreadData*>   idle_threads;
  WorkSchedulerData(JobQueue& in, JobQueue& out)
  : q_in(in), q_out(out), time_out(600), running(false) {
    pthread_mutex_init(&threads_list_lock, NULL);
  }
  ~WorkSchedulerData() {
    pthread_mutex_destroy(&threads_list_lock);
  }
};

struct WorkScheduler::Private {
  WorkSchedulerData         data;
  pthread_t                 monitor_tid;
  Private(JobQueue& in, JobQueue& out) : data(in, out) {}
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
      // Put idle thread at the front of the list, if not shutting down
      pthread_mutex_lock(&d->parent.threads_list_lock);
      if (d->parent.running) {
        d->parent.busy_threads.remove(d);
        // Always push to back so unused threads are at the front
        d->parent.idle_threads.push_back(d);
        d->last_run = time(NULL);
      }
      pthread_mutex_unlock(&d->parent.threads_list_lock);
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
    hlog_regression("monitor.loop enter");
    void* data_in;
    if (d->q_in.pop(&data_in) == 0) {
      pthread_mutex_lock(&d->threads_list_lock);
      // Get first idle thread
      WorkerThreadData* wtd = NULL;
      if (! d->idle_threads.empty()) {
        wtd = d->idle_threads.back();
        d->idle_threads.pop_back();
        // Always push to back so longest running threads are at the front
        d->busy_threads.push_back(wtd);
        // Get rid of oldest idle thread if too old
        list<WorkerThreadData*>::iterator it = d->idle_threads.begin();
        if ((it != d->idle_threads.end()) &&
            (((*it)->last_run - time(NULL)) > d->time_out)) {
          (*it)->q_in.close();
          pthread_join((*it)->tid, NULL);
          delete *it;
          d->idle_threads.erase(it);
          --d->threads;
        }
      } else
      // Create new thread if possible
      if (((d->max_threads == 0) || (d->busy_threads.size() < d->max_threads))) {
        // Create worker thread
        char name[NAME_SIZE];
        snprintf(name, NAME_SIZE, "worker #%zu", ++order);
        wtd = new WorkerThreadData(*d, name);
        if (pthread_create(&wtd->tid, NULL, worker_thread, wtd) == 0) {
          d->busy_threads.push_back(wtd);
          ++d->threads;
          hlog_regression("%s.thread created", wtd->name);
        } else {
          // FIXME Report cause of error
          hlog_error("could not create thread");
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
      hlog_regression("monitor.queue closed");
      pthread_mutex_lock(&d->threads_list_lock);
      d->running = false;
      // Stop all idle threads
      list<WorkerThreadData*>::iterator it = d->idle_threads.begin();
      while (it != d->idle_threads.end()) {
        (*it)->q_in.close();
        hlog_regression("%s.queue closed", (*it)->name);
        pthread_join((*it)->tid, NULL);
        hlog_regression("%s.thread joined", (*it)->name);
        delete *it;
        it = d->idle_threads.erase(it);
      }
      hlog_regression("all idle worker thread(s) destroyed");
      // Close all busy threads queues
      for (list<WorkerThreadData*>::iterator it = d->busy_threads.begin();
           it != d->busy_threads.end(); ++it) {
        (*it)->q_in.close();
        hlog_regression("%s.queue closed", (*it)->name);
      }
      hlog_regression("all %zu busy worker queue(s) closed",
        d->busy_threads.size());
      // Wait for all to have stopped (not put in idle list when shutting down)
      it = d->busy_threads.begin();
      while (it != d->busy_threads.end()) {
        // Unlock so threads can finish
        pthread_mutex_unlock(&d->threads_list_lock);
        pthread_join((*it)->tid, NULL);
        if (hlog_is_worth(regression)) {
          usleep(100000);
          hlog_regression("%s.thread joined", (*it)->name);
        }
        delete *it;
        pthread_mutex_lock(&d->threads_list_lock);
        it = d->busy_threads.erase(it);
      }
      pthread_mutex_unlock(&d->threads_list_lock);
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
  if (_d->data.running) return -1;
  _d->data.min_threads = min_threads;
  _d->data.max_threads = max_threads;
  // Start monitoring thread
  int rc = pthread_create(&_d->monitor_tid, NULL, monitor_thread, &_d->data);
  if (rc == 0) {
    _d->data.running = true;
    _d->data.threads = 0;
    hlog_regression("monitor.thread created");
  }
  return rc;
}

int WorkScheduler::stop() {
  if (! _d->data.running) return -1;

  // Stop monitoring thread
  _d->data.q_in.close();
  pthread_join(_d->monitor_tid, NULL);
  hlog_regression("monitor.thread joined");
  _d->data.running = false;

  return 0;
}

size_t WorkScheduler::threads() const {
  return _d->data.threads;
}
