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

#ifndef _THREADS_MANAGER_H
#define _THREADS_MANAGER_H

#include <queue.h>

namespace htoolbox {

class ThreadsManager {
  Queue          in;
  struct         Private;
  Private* const _d;
  ThreadsManager(const htoolbox::ThreadsManager&);
public:
  typedef void* (*routine_f)(void* data, void* user);
  ThreadsManager(
    const char* name,
    routine_f   routine,
    void*       user,
    size_t      q_in_size = 1,
    Queue*      q_out = NULL);
  ~ThreadsManager();
  Queue& inputQueue() { return in; }
  int push(void* data) { return in.push(data); }
  typedef void (*callback_f)(bool idle, void* user);
  void setActivityCallback(callback_f callback, void* user);
  //! Set base value for thread IDs, which starts their reporting in logs
  /*!
   * The base value will be added to the thread ID set by the thread manager.
   * IDs should be in the range of 0 to 9999999, so a base value should ideally
   * be comprised between 1000000 and 9000000, depending how many threads are
   * going to be created...
   * This number will then appear in logs.
   *
   * \param base        second delimiter to use
  */
  void setThreadIdBase(int base);
  int start(size_t max_threads = 0, size_t min_threads = 0, time_t time_out = 600);
  int stop(bool urgent = false);
  size_t threads() const;
};

}

#endif // _THREADS_MANAGER_H
