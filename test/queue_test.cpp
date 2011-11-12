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
#include <queue.h>

using namespace htoolbox;

int main(void) {
  report.setLevel(debug);
  report.consoleFilter().addCondition(Report::Filter::force,
    "queue.cpp", 0, 0, regression);
  report.consoleFilter().addCondition(Report::Filter::force,
    __FILE__, 0, 0, regression);

  enum { SIZE = 10 };
  Queue q1("q1", SIZE);
  q1.open();
  int i[SIZE];
  for (int k = 0; k < SIZE; ++k) {
    i[k] = k + 20;
    q1.push(&i[k]);
  }
  for (int k = 0; k < SIZE / 2; ++k) {
    int* p;
    q1.pop(reinterpret_cast<void**>(&p));
    if (p != &i[k]) {
      hlog_error("pointer mismatch");
    } else {
      hlog_info("o = %d", *p);
    }
  }
  q1.signal();
  for (int k = 0; k < SIZE / 2; ++k) {
    i[k] = k + 30;
    q1.push(&i[k]);
  }
  for (int k = 0; k < SIZE + 1; ++k) {
    int* p;
    if (q1.pop(reinterpret_cast<void**>(&p)) == 0) {
      hlog_info("o = %d", *p);
    }
  }
  q1.signal();
  q1.pop(NULL);
  q1.close();
  return 0;
}
