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

#ifndef _PROCESS_MUTEX_H
#define _PROCESS_MUTEX_H

namespace htoolbox {

class ProcessMutex {
  struct         Private;
  Private* const _d;
  ProcessMutex(const Socket&);
  const ProcessMutex& operator=(const ProcessMutex&);
public:
  ProcessMutex(const char* lock_name);
  ~ProcessMutex();
  int lock();
  int unlock();
};

};

#endif // _PROCESS_MUTEX_H
