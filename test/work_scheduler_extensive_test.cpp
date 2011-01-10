/*
     Copyright (C) 2010-2011  Herve Fache

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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <report.h>
#include <queue.h>
#include <work_scheduler.h>

using namespace htoolbox;

static void* task1(void* data, void* user) {
  char* cuser = static_cast<char*>(user);
  char* cdata = static_cast<char*>(data);
  hlog_regression("%s enter: data = '%s' user = '%s'",
    __FUNCTION__, cdata, cuser);
  cdata[0] = 'b';
  usleep(100000);
  hlog_regression("%s exit: data = '%s'", __FUNCTION__, cdata);
  return data;
}

static void* task2(void* data, void* user) {
  char* cuser = static_cast<char*>(user);
  char* cdata = static_cast<char*>(data);
  hlog_regression("%s enter: data = '%s' user = '%s'",
    __FUNCTION__, cdata, cuser);
  cdata[0] = 'c';
  usleep(300000);
  hlog_regression("%s exit: data = '%s'", __FUNCTION__, cdata);
  return data;
}

static void* task3(void* data, void* user) {
  char* cuser = static_cast<char*>(user);
  char* cdata = static_cast<char*>(data);
  hlog_regression("%s enter: data = '%s' user = '%s'",
    __FUNCTION__, cdata, cuser);
  cdata[0] = 'd';
  usleep(200000);
  hlog_regression("%s exit: data = '%s'", __FUNCTION__, cdata);
  return data;
}

enum {
  MAX_LOOPS = 2100
};

int main(void) {
  report.setLevel(regression);
//   report.addRegressionCondition("work_scheduler.cpp");
  report.addRegressionCondition("work_scheduler_extensive_test.cpp");

  Queue q_in("in");
  Queue q_int1("int1");
  Queue q_int2("int2");
  char user1[32] = "user1";
  char user2[32] = "user2";
  char user3[32] = "user3";
  WorkScheduler ws1("sched1", task1, user1, &q_in, &q_int1);
  WorkScheduler ws2("sched2", task2, user2, &q_int1, &q_int2);
  WorkScheduler ws3("sched3", task3, user3, &q_int2, NULL);

  q_in.open();
  q_int1.open();
  q_int2.open();
  if ((ws1.start(0, 0, 2) != 0) || (ws2.start(0, 0, 2) != 0) || (ws3.start(0, 0, 2) != 0)) {
    hlog_error("start failed");
  } else {
    char data[MAX_LOOPS][32];
    for (size_t i = 0; i < MAX_LOOPS; ++i) {
      sprintf(data[i], "a%03zu", i);
      hlog_regression("push %s", data[i]);
      q_in.push(data[i]);
      hlog_regression("ws1 %zu thread(s)", ws1.threads());
      hlog_regression("ws2 %zu thread(s)", ws2.threads());
      hlog_regression("ws3 %zu thread(s)", ws3.threads());
      if (i < (MAX_LOOPS / 3)) {
        usleep(10000);
      } else
      if (i > (MAX_LOOPS / 3)){
        usleep(20000);
      } else
      {
        hlog_regression("slacking");
        usleep(100000);
      }
    }
    ws1.stop();
    hlog_regression("ws1 stopped");
    hlog_regression("ws2 %zu thread(s)", ws2.threads());
    hlog_regression("ws3 %zu thread(s)", ws3.threads());
    ws2.stop();
    hlog_regression("ws2 stopped");
    hlog_regression("ws3 %zu thread(s)", ws3.threads());
    ws3.stop();
    hlog_regression("ws3 stopped");
  }

  return 0;
}
