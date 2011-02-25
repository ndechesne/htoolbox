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
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include <string.h>

#include <report.h>
#include <socket.h>
#include <tlv_helper.h>

using namespace htoolbox;
using namespace tlv;

pthread_mutex_t main_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;

static char tx_buffer[102400];
static ssize_t tx_callback(const char** buffer, size_t size, void* user) {
  static size_t sent = 0;
  static bool initialised = false;
  const char* cuser = static_cast<char*>(user);
  if (! initialised) {
    for (size_t i = 0; i < sizeof(tx_buffer); ++i) {
      tx_buffer[i] = 'A'/*static_cast<char>((rand() + 0x32) & 0x7f)*/;
    }
  }
  *buffer = &tx_buffer[sent];
  if (size < (sizeof(tx_buffer) - sent)) {
    sent += size;
  } else {
    size = sizeof(tx_buffer) - sent;
    sent = sizeof(tx_buffer);
  }
  hlog_verbose("tx callback called for %s, sent = %zu, rc = %zd",
    cuser, sent, size);
  return size;
}

static char rx_buffer[102400];
int rx_callback(const char* buffer, size_t size, void* user) {
  static size_t received = 0;
  const char* cuser = static_cast<char*>(user);
  memcpy(&rx_buffer[received], buffer, size);
  received += size;
  hlog_verbose("rx callback called for %s, received = %zu, rc = %zd",
    cuser, received, size);
  return 0;
}

void* receiver(void*) {
  pthread_mutex_lock(&thread_mutex);
  Socket sock("socket");
  Receiver receiver(sock);

  if (sock.listen(5) < 0) {
    hlog_error("%s listening", strerror(errno));
    return NULL;
  }
  pthread_mutex_unlock(&main_mutex);
  if (sock.open() < 0) {
    hlog_error("%s opening socket", strerror(errno));
    return NULL;
  }

  ReceptionManager r;
  // Bool
  bool b1 = true;
  r.add(0, &b1);
  // Bool
  bool b2 = false;
  r.add(1, &b2);
  // Blob
  char cs[BUFFER_MAX] = "";
  r.add(2, cs, BUFFER_MAX);
  // BigBlob
  char bigblob[] = "BigBlobR";
  r.add(3, rx_callback, bigblob);
  // Int
  int32_t i = 0;
  r.add(4, &i);
  // String
  std::string s;
  r.add(5, s);

  Receiver::Type type;
  do {
    uint16_t    tag;
    size_t      len;
    char        val[65536];
    type = receiver.receive(&tag, &len, val);
    usleep(1000);
    hlog_info("receive: type=%d tag=%u, len=%zu, value='%s'",
      type, tag, len, ((len > 0) || (tag == 0)) ? val : "");
    if (type == Receiver::DATA) {
      r.submit(tag, len, val);
    }
  } while (type >= Receiver::END);

  pthread_mutex_lock(&thread_mutex);

  hlog_info("values:");
  hlog_info("b1 = %d", b1);
  hlog_info("b2 = %d", b2);
  hlog_info("cs = %s", cs);
  hlog_info("cmp = %d", memcmp(tx_buffer, rx_buffer, sizeof(tx_buffer)));
  hlog_info("i = %d", i);
  hlog_info("s = %s", s.c_str());

  sock.close();

  pthread_mutex_unlock(&main_mutex);

  return NULL;
}

int main(void) {
  report.setLevel(verbose);
  pthread_mutex_lock(&main_mutex);


  pthread_t thread;
  int rc = pthread_create(&thread, NULL, receiver, NULL);
  if (rc < 0) {
    hlog_error("%s, failed to create thread1", strerror(-rc));
    return 0;
  }

  pthread_mutex_lock(&main_mutex);

  Socket sock("socket");
  if (sock.open() < 0) {
    hlog_error("%s opening socket", strerror(errno));
    return 0;
  }

  Sender sender(sock);
  TransmissionManager t;
  // Bool
  t.add(0, false);
  // Bool
  t.add(1, true);
  // Blob
  t.add(2, "Hello world!", 12);
  // BigBlob
  char bigblob[] = "BigBlobT";
  t.add(3, tx_callback, bigblob);
  // Int
  t.add(4, 12);
  // String
  std::string s = "Goodbye cruel world";
  t.add(5, s);

  t.send(sender);

  pthread_mutex_unlock(&thread_mutex);


  sock.close();

  pthread_mutex_lock(&main_mutex);

  return 0;
}
