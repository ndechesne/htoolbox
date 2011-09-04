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

  Queue q1("q1", 1);
  q1.open();
  int i = 1;
  q1.push(&i);
  void* p;
  q1.pop(&p);
  hlog_info("i = %d", *static_cast<int*>(p));
  q1.close();
  return 0;
}
