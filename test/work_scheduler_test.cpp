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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <report.h>
#include <queue.h>
#include <work_scheduler.h>

using namespace htools;

static void* task(void* data, void* user) {
  char* cuser = static_cast<char*>(user);
  char* cdata = static_cast<char*>(data);
  hlog_regression("task enter: data = '%s' user = '%s'", cdata, cuser);
  cdata[0] = 't';
  sleep(1);
  hlog_regression("task exit: data = '%s'", cdata);
  return data;
}

int main(void) {
  report.setLevel(regression);
  report.addRegressionCondition("work_scheduler.cpp");
  report.addRegressionCondition("work_scheduler_test.cpp");

  Queue q_in("in");
  Queue q_out("out");
  char user[32] = "";
  WorkScheduler ws("sched", q_in, q_out, task, user);

  hlog_regression("no thread limit");
  q_in.open();
  q_out.open(20);
  strcpy(user, "user1");
  if (ws.start() != 0) {
    hlog_error("start failed");
  } else {
    usleep(50000);
    char data1[32] = "data1";
    q_in.push(data1);
    usleep(250000);
    char data2[32] = "data2";
    q_in.push(data2);
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
  q_in.open();
  q_out.open(20);
  strcpy(user, "user2");
  if (ws.start(1) != 0) {
    hlog_error("start failed");
  } else {
    usleep(100000);
    char data1[32] = "data1";
    q_in.push(data1);
    usleep(100000);
    char data2[32] = "data2";
    q_in.push(data2);
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
  q_in.open();
  q_out.open(20);
  strcpy(user, "user3");
  if (ws.start() != 0) {
    hlog_error("start failed");
  } else {
    usleep(50000);
    char data1[32] = "data1";
    q_in.push(data1);
    usleep(250000);
    char data2[32] = "data2";
    q_in.push(data2);
    usleep(250000);
    char data3[32] = "data3";
    q_in.push(data3);
    usleep(250000);
    char data4[32] = "data4";
    q_in.push(data4);
    usleep(250000);
    char data5[32] = "data5";
    q_in.push(data5);
    usleep(250000);
    char data6[32] = "data6";
    q_in.push(data6);
    usleep(250000);
    char data7[32] = "data7";
    q_in.push(data7);
    usleep(250000);
    char data8[32] = "data8";
    q_in.push(data8);
    usleep(250000);
    char data9[32] = "data9";
    q_in.push(data9);
    usleep(250000);
    char data10[32] = "data10";
    q_in.push(data10);
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
