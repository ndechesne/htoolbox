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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <report.h>
#include <queue.h>
#include <threads_manager.h>

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
  report.setLevel(debug);
  report.consoleFilter().addCondition(Report::Filter::force,
    "threads_manager.cpp", 0, 0, regression);
  report.consoleFilter().addCondition(Report::Filter::force,
    __FILE__, regression);

  char user1[32] = "user1";
  char user2[32] = "user2";
  char user3[32] = "user3";
  ThreadsManager ws3("sched3", task3, user3, NULL);
  ThreadsManager ws2("sched2", task2, user2, &ws3.inputQueue());
  ThreadsManager ws1("sched1", task1, user1, &ws2.inputQueue());

  if ((ws3.start(0, 0, 2) != 0) || (ws2.start(0, 0, 2) != 0) || (ws1.start(0, 0, 2) != 0)) {
    hlog_error("start failed");
  } else {
    char data[MAX_LOOPS][32];
    for (size_t i = 0; i < MAX_LOOPS; ++i) {
      sprintf(data[i], "a%03zu", i);
      hlog_regression("push %s", data[i]);
      ws1.push(data[i]);
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
