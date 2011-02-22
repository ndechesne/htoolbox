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
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include <report.h>
#include <socket.h>
#include <tlv.h>

using namespace htoolbox;
using namespace tlv;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* receiver(void*) {
  // Report test!
  Report my_report;
  tl_report = &my_report;
  hlog_info("receiver (local) log level is %s", Report::levelString(tl_report->level()));

  Socket sock("socket");
  Receiver receiver(sock);

  if (sock.listen(5) < 0) {
    hlog_error("%s listening", strerror(errno));
    return NULL;
  }
  pthread_mutex_unlock(&mutex);
  if (sock.open() < 0) {
    hlog_error("%s opening socket", strerror(errno));
    return NULL;
  }

  Receiver::Type type;
  Report::TlvManager log_manager;
  do {
    uint16_t    tag;
    size_t      len;
    char        val[65536];
    type = receiver.receive(&tag, &len, val);
    hlog_info("receive: type=%d tag=%u, len=%zu, value='%s'",
      type, tag, len, ((len > 0) || (tag == 0)) ? val : "");
    // The code below would be fun, but report would see a line number within
    // range (the original one) and try to send it over the socket...
/*    if ((tag >= tlv::log_start_tag) && (tag < tlv::log_start_tag + 10) &&
        (log_manager.submit(tag, len, val) < 0)) {
      hlog_error("%s logging", strerror(errno));
    }*/
  } while (type >= Receiver::END);

  sock.close();

  pthread_mutex_unlock(&mutex);

  return NULL;
}

int main(void) {
  report.setLevel(verbose);
  pthread_mutex_lock(&mutex);

  hlog_info("initial (global) log level is %s", Report::levelString(report.level()));

  pthread_t thread;
  int rc = pthread_create(&thread, NULL, receiver, NULL);
  if (rc < 0) {
    hlog_error("%s, failed to create thread1", strerror(-rc));
    return 0;
  }

  pthread_mutex_lock(&mutex);
  hlog_info("sender (global) log level is %s", Report::levelString(report.level()));

  Socket sock("socket");
  if (sock.open() < 0) {
    hlog_error("%s opening socket", strerror(errno));
    return 0;
  }


  hlog_info("basic test");
  Sender sender(sock);
  if (sender.start() < 0) {
    hlog_error("%s writing to socket (start)", strerror(errno));
    return 0;
  }
  if (sender.write(1, NULL, 0) < 0) {
    hlog_error("%s writing to socket (empty)", strerror(errno));
    return 0;
  }
  if (sender.check() < 0) {
    hlog_error("%s writing to socket (check)", strerror(errno));
    return 0;
  }
  const char message[] = "I am not a stupid protocol!";
  if (sender.write(0x12, message, strlen(message)) < 0) {
    hlog_error("%s writing to socket (string)", strerror(errno));
    return 0;
  }
  if (sender.end() < 0) {
    hlog_error("%s writing to socket (end)", strerror(errno));
    return 0;
  }


  hlog_info("log test");
  // The global report shall log to the socket, but filter out the thread
  report.stopConsoleLog();
  Report::TlvOutput o(sender);
  Report::Filter f("sender", &o, false);
  f.addCondition(false, "tlv_test.cpp", 37, 76);
  if (sender.start() < 0) {
    hlog_error("%s writing to socket (start)", strerror(errno));
    return 0;
  }
  report.add(&f);
  hlog_info("this log should be send over the socket (%d)", 9);
  report.log("file", 12345, warning, true, 3, "this is some text with a number %d", 17);
  tl_report = &report;
  hlog_info("message sent twice to socket");
  report.remove(&f);
  if (sender.end() < 0) {
    hlog_error("%s writing to socket (end)", strerror(errno));
    return 0;
  }


  sock.close();

  pthread_mutex_lock(&mutex);

  return 0;
}
