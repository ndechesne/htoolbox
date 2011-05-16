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
#include <unistd.h>
#include <string.h>

#include <report.h>
#include <queue.h>
#include <threads_manager.h>

using namespace htoolbox;

static void* task(void* data, void* user) {
  char* cuser = static_cast<char*>(user);
  char* cdata = static_cast<char*>(data);
  hlog_regression("task enter: data = '%s' user = '%s'", cdata, cuser);
  cdata[0] = 't';
  sleep(1);
  hlog_regression("task exit: data = '%s'", cdata);
  return data;
}

void activity_callback(bool idle, void* user) {
  char* cuser = static_cast<char*>(user);
  hlog_regression("activity callback(%s): user = '%s'",
    idle ? "idle" : "busy", cuser);
}

int main(void) {
  report.setLevel(debug);
  report.consoleFilter().addCondition(true, "threads_manager.cpp", 0, 0, regression);
  report.consoleFilter().addCondition(true, __FILE__, 0, 0, regression);

  Queue q_out("out", 20);
  char user[32] = "";
  ThreadsManager ws("sched", task, user, &q_out);

  char cuser[] = "user_string";
  ws.setActivityCallback(activity_callback, cuser);

  hlog_regression("no thread limit");
  q_out.open();
  strcpy(user, "user1");
  if (ws.start() != 0) {
    hlog_error("start failed");
  } else {
    usleep(50000);
    char data1[32] = "data1";
    ws.push(data1);
    usleep(250000);
    char data2[32] = "data2";
    ws.push(data2);
    usleep(150000);
    hlog_regression("%zu thread(s)", ws.threads());
    ws.stop();
    usleep(50000);
    char* data_out;
    hlog_regression("out queue:");
    q_out.close();
    while (q_out.pop(reinterpret_cast<void**>(&data_out)) == 0) {
      hlog_regression("  '%s'", data_out);
    }
    q_out.wait();
  }

  hlog_regression("thread limit = 1");
  q_out.open();
  strcpy(user, "user2");
  if (ws.start(1) != 0) {
    hlog_error("start failed");
  } else {
    usleep(100000);
    char data1[32] = "data1";
    ws.push(data1);
    usleep(100000);
    char data2[32] = "data2";
    ws.push(data2);
    usleep(100000);
    hlog_regression("%zu thread(s)", ws.threads());
    ws.stop();
    usleep(100000);
    char* data_out;
    hlog_regression("out queue:");
    q_out.close();
    while (q_out.pop(reinterpret_cast<void**>(&data_out)) == 0) {
      hlog_regression("  '%s'", data_out);
    }
    q_out.wait();
  }

  hlog_regression("no thread limit, 10 objects");
  q_out.open();
  strcpy(user, "user3");
  if (ws.start() != 0) {
    hlog_error("start failed");
  } else {
    usleep(50000);
    char data1[32] = "data1";
    ws.push(data1);
    usleep(250000);
    char data2[32] = "data2";
    ws.push(data2);
    usleep(250000);
    char data3[32] = "data3";
    ws.push(data3);
    usleep(250000);
    char data4[32] = "data4";
    ws.push(data4);
    usleep(250000);
    char data5[32] = "data5";
    ws.push(data5);
    usleep(250000);
    char data6[32] = "data6";
    ws.push(data6);
    usleep(250000);
    char data7[32] = "data7";
    ws.push(data7);
    usleep(250000);
    char data8[32] = "data8";
    ws.push(data8);
    usleep(250000);
    char data9[32] = "data9";
    ws.push(data9);
    usleep(250000);
    char data10[32] = "data10";
    ws.push(data10);
    usleep(150000);
    hlog_regression("%zu thread(s)", ws.threads());
    ws.stop();
    usleep(50000);
    char* data_out;
    hlog_regression("out queue:");
    q_out.close();
    while (q_out.pop(reinterpret_cast<void**>(&data_out)) == 0) {
      hlog_regression("  '%s'", data_out);
    }
    q_out.wait();
  }

  return 0;
}
