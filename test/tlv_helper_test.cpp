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
  static bool   first_time = true;
  const char* cuser = static_cast<char*>(user);
  *buffer = &tx_buffer[sent];
  ssize_t rc = size;
  if (size >= (sizeof(tx_buffer) - sent)) {
    size = sizeof(tx_buffer) - sent;
    if (first_time) {
      rc = size;
    } else {
      rc = -103587;
    }
  }
  sent += size;
  hlog_verbose("tx callback called for %s, sent = %zu, rc = %zd",
    cuser, sent, rc);
  if (size == 0) {
    sent = 0;
    first_time = false;
  }
  return rc;
}

static char rx_buffer[102400];
int rx_callback(const char* buffer, size_t size, void* user) {
  static size_t received = 0;
  const char* cuser = static_cast<char*>(user);
  memcpy(&rx_buffer[received], buffer, size);
  received += size;
  hlog_verbose("rx callback called for %s, received = %zu, rc = %zd",
    cuser, received, size);
  if (size == 0) {
    received = 0;
  }
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
  char rx_buffer21[102400];
  r.add(21, rx_buffer21, sizeof(rx_buffer21));
  // Blob
  char rx_buffer22[102400];
  r.add(22, rx_buffer22, sizeof(rx_buffer22));
  // Blob
  char rx_buffer23[102400];
  r.add(23, rx_buffer23, sizeof(rx_buffer23));
  // Blob
  char rx_buffer24[102400];
  r.add(24, rx_buffer24, sizeof(rx_buffer24));
  // Blob
  char rx_buffer25[102400];
  r.add(25, rx_buffer25, sizeof(rx_buffer25));
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
    hlog_info("receive: type=%d tag=%u, len=%zu %s", type, tag, len,
      tag < 65530 ? "" : val);
    if (type == Receiver::DATA) {
      if (r.submit(tag, len, val) < 0) {
        hlog_error("no receiver for tag %d", tag);
      }
    }
  } while (type >= Receiver::END);

  pthread_mutex_lock(&thread_mutex);

  hlog_info("values:");
  hlog_info("b1 = %d", b1);
  hlog_info("b2 = %d", b2);

  hlog_info("21 = %d", memcmp(tx_buffer, rx_buffer21, BUFFER_MAX));
  hlog_info("22 = %d", memcmp(tx_buffer, rx_buffer22, 30));
  hlog_info("23 = %d", memcmp(tx_buffer, rx_buffer23, 30));

  hlog_info("25 = %d", memcmp(tx_buffer, rx_buffer25, sizeof(rx_buffer25)));
  for (size_t i = 0; i < sizeof(tx_buffer); ++i) {
    if (tx_buffer[i] != rx_buffer25[i]) {
      hlog_error("tx_buffer[%zu]=%d and rx_buffer25[%zu]=%d differ",
        i, tx_buffer[i], i, rx_buffer25[i]);
      break;
    }
  }

  hlog_info("bb = %d", memcmp(tx_buffer, rx_buffer, sizeof(rx_buffer)));
  for (size_t i = 0; i < sizeof(tx_buffer); ++i) {
    if (tx_buffer[i] != rx_buffer[i]) {
      hlog_error("tx_buffer[%zu]=%d and rx_buffer[%zu]=%d differ",
        i, tx_buffer[i], i, rx_buffer[i]);
      break;
    }
  }
  hlog_info("i = %d", i);
  hlog_info("s = %s", s.c_str());

  sock.close();

  pthread_mutex_unlock(&main_mutex);

  return NULL;
}

int main(void) {
  report.setLevel(regression);
  pthread_mutex_lock(&main_mutex);

  for (size_t i = 0; i < sizeof(tx_buffer); ++i) {
    tx_buffer[i] = static_cast<char>((rand() + 0x32) & 0x7f);
  }

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
  t.add(21, tx_buffer, BUFFER_MAX);
  // Blob
  t.add(22, tx_buffer, 30);
  // Blob
  t.add(23, tx_buffer, 30);
  // Blob
  t.add(24, tx_buffer, 0);
  // Blob
  t.add(25, tx_buffer, sizeof(tx_buffer));
  // BigBlob
  char bigblob[] = "BigBlobT";
  t.add(3, tx_callback, bigblob);
  // Int
  t.add(4, 12);
  // String
  std::string s = "Goodbye cruel world";
  t.add(5, s);
  // BigBlob (fails)
  t.add(3, tx_callback, bigblob);

  t.send(sender);

  pthread_mutex_unlock(&thread_mutex);


  sock.close();

  pthread_mutex_lock(&main_mutex);
  pthread_join(thread, NULL);

  return 0;
}
