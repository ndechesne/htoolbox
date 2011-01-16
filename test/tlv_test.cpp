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
  do {
    uint8_t tag;
    size_t  len;
    char    val[65535];
    type = receiver.receive(&tag, &len, val);
    hlog_info("receive: type=%d tag=%d, len=%d, value='%s'",
      type, tag, len, ((len > 0) || (tag == 0)) ? val : "");
  } while (type >= Receiver::END);

  sock.close();

  pthread_mutex_unlock(&mutex);

  return NULL;
}

int main(void) {
  report.setLevel(verbose);
  pthread_mutex_lock(&mutex);

  tl_report = &report;
  hlog_info("initial (global) log level is %s", Report::levelString(tl_report->level()));

  pthread_t thread;
  int rc = pthread_create(&thread, NULL, receiver, NULL);
  if (rc < 0) {
    hlog_error("%s, failed to create thread1", strerror(-rc));
    return 0;
  }

  pthread_mutex_lock(&mutex);
  hlog_info("sender (global) log level is %s", Report::levelString(tl_report->level()));

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
  if (sender.write(0x12, "I am not a stupid protocol!") < 0) {
    hlog_error("%s writing to socket (string)", strerror(errno));
    return 0;
  }
  if (sender.end() < 0) {
    hlog_error("%s writing to socket (end)", strerror(errno));
    return 0;
  }


  hlog_info("log test");
  report.stopConsoleLog();
  Report::TlvOutput o(sender, 11);
  report.add(&o);
  if (sender.start() < 0) {
    hlog_error("%s writing to socket (start)", strerror(errno));
    return 0;
  }
  hlog_info("this log should be send over the socket (%d)", 9);
  tl_report->log("file", 12345, warning, true, 3, "this is some text with a number %d", 17);
  if (sender.end() < 0) {
    hlog_error("%s writing to socket (end)", strerror(errno));
    return 0;
  }
  

  sock.close();

  pthread_mutex_lock(&mutex);

  return 0;
}
