/*
    Copyright (C) 2011  Hervé Fache

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

#include <report.h>
#include <files.h>
#include "process_mutex.h"

using namespace htoolbox;

ProcessMutex::ProcessMutex(const char* lock_name) {
  locked_ = false;
  if (lock_name[0] != '/') {
    char* cwd = get_current_dir_name();
    lock_ = new Socket(Path(cwd, lock_name), true);
    free(cwd);
  } else {
    lock_ = new Socket(lock_name, true);
  }
}

ProcessMutex::~ProcessMutex() {
  delete lock_;
}

int ProcessMutex::lock() {
  if (locked_) {
    hlog_error("Trying to lock %s twice", lock_->path());
    errno = EBUSY;
    return -1;
  }
  if (lock_->listen(0) < 0) {
    hlog_error("%m locking %s", lock_->path());
    return -1;
  }
  locked_ = true;
  hlog_regression("%s locked", lock_->path());
  return 0;
}

int ProcessMutex::unlock() {
  if (! locked_) {
    hlog_error("Trying to unlock while not locked");
    errno = EBADF;
    return -1;
  }
  locked_ = false;
  hlog_regression("%s unlocked", lock_->path());
  return 0;
}
