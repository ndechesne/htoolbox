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
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <report.h>
#include "socket.h"

using namespace htoolbox;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* server_thread(void* user) {
  Socket& master_server = *static_cast<Socket*>(user);
  Socket server(master_server);
  hlog_info("server going to accept connections");
  pthread_mutex_unlock(&mutex);
  int sock = server.open();
  if (sock < 0) {
    hlog_error("%s, server failed to open", strerror(errno));
  } else {
    hlog_info("client connection socket #%d", sock);
  }
  hlog_info("client connected");
  ssize_t rc;
  do {
    char buffer[65536];
    rc = server.stream(buffer, sizeof(buffer));
    if (rc > 0) {
      hlog_info("received '%s'", buffer);
    } else {
      break;
    }
    strcat(buffer, ". server is alive and kicking.");
    rc = server.write(buffer, strlen(buffer) + 1);
  } while (rc > 0);
  hlog_info("client disconnected");
  if (server.close() < 0) {
    hlog_error("%s, server failed to close", strerror(errno));
  }
  return NULL;
}

int main() {
  report.setLevel(verbose);
  pthread_mutex_lock(&mutex);

  Socket server(@@FIRST@@);
  hlog_info("server path = '%s'", server.path());
  int sock = server.listen(2);
  if (sock < 0) {
    hlog_error("%s, server failed to listen", strerror(errno));
    return 0;
  } else {
    hlog_info("server listening socket #%d", sock);
  }

  int rc;
  pthread_t thread1;
  rc = pthread_create(&thread1, NULL, server_thread, &server);
  if (rc < 0) {
    hlog_error("%s, failed to create thread1", strerror(-rc));
    return 0;
  }
  pthread_t thread2;
  rc = pthread_create(&thread2, NULL, server_thread, &server);
  if (rc < 0) {
    hlog_error("%s, failed to create thread2", strerror(-rc));
    return 0;
  }

  pthread_mutex_lock(&mutex);
  pthread_mutex_lock(&mutex);

  Socket client1(@@SECOND@@);
  hlog_info("client1 path = '%s'", client1.path());
  if (client1.open() < 0) {
    hlog_error("%s, client1 failed to connect", strerror(-rc));
  }

  usleep(1000);

  Socket client2(@@THIRD@@);
  hlog_info("client2 path = '%s'", client2.path());
  if (client2.open() < 0) {
    hlog_error("%s, client2 failed to connect", strerror(-rc));
  }

  usleep(1000);

  char buffer[65536];
  strcpy(buffer, "This is my message");
  if (client1.write(buffer, strlen(buffer) + 1) < 0) {
    hlog_error("%s, client1 failed to write", strerror(-rc));
  }

  usleep(1000);

  strcpy(buffer, "Let us have another message");
  if (client2.write(buffer, strlen(buffer) + 1) < 0) {
    hlog_error("%s, client2 failed to write", strerror(-rc));
  }

  usleep(1000);

  if (client1.stream(buffer, sizeof(buffer)) < 0) {
    hlog_error("%s, client1 failed to write", strerror(-rc));
  } else {
    hlog_info("message from server to client1: '%s'", buffer);
  }

  usleep(1000);

  if (client2.stream(buffer, sizeof(buffer)) < 0) {
    hlog_error("%s, client2 failed to write", strerror(-rc));
  } else {
    hlog_info("message from server to client2: '%s'", buffer);
  }

  if (client1.close() < 0) {
    hlog_error("%s, client1 failed to disconnect", strerror(-rc));
  }

  if (client2.close() < 0) {
    hlog_error("%s, client2 failed to disconnect", strerror(-rc));
  }

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  if (server.release() < 0) {
    hlog_error("%s, failed to release listening socket", strerror(rc));
  }

  return 0;
}
