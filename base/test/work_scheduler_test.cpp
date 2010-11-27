#include <stdlib.h>
#include <unistd.h>

#include <hreport.h>
#include <job_queue.h>
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

  hlog_regression("no thread limit");
  JobQueue q_in("in");
  q_in.open();
  JobQueue q_out("out");
  q_out.open(20);
  char user[32] = "user";
  WorkScheduler ws(q_in, q_out, task, user);
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
  return 0;
}
