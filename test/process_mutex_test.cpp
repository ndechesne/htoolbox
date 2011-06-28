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

#include <stdlib.h>

#include <report.h>
#include "process_mutex.h"

using namespace htoolbox;

int main() {
  report.setLevel(regression);

  ProcessMutex pm1("/net/lucidia/process_mutex_test");
  ProcessMutex pm2("net/lucidia/process_mutex_test");
  ProcessMutex pm3("net/lucidia/process_mutex_test");

  if (! pm1.lock()) {
    if (! pm2.lock()) {
      if (! pm3.lock()) {
        pm3.unlock();
      }
      pm2.unlock();
    }
    pm1.unlock();
  }

  return 0;
}
