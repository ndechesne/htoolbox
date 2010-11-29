/*
     Copyright (C) 2010  Herve Fache

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

#ifndef _WORK_SCHEDULER_H
#define _WORK_SCHEDULER_H

namespace htools {

class WorkScheduler {
  struct         Private;
  Private* const _d;
  WorkScheduler(const htools::WorkScheduler&);
public:
  typedef void* (*routine_f)(void* data, void* user);
  WorkScheduler(const char* name, JobQueue& in, JobQueue& out,
    routine_f routine, void* user);
  ~WorkScheduler();
  int start(size_t max_threads = 0, size_t min_threads = 0, time_t time_out = 600);
  int stop();
  size_t threads() const;
};

};

#endif // _WORK_SCHEDULER_H
