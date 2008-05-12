/*
     Copyright (C) 2008  Herve Fache

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

#include <iostream>

using namespace std;

#include "lock.h"

using namespace hbackup;

void* child(void* data) {
  Lock* l = static_cast<Lock*>(data);

  cout << "unlock" << endl;
  if (l->isLocked()) {
    cout << "-> locked" << endl;
  } else {
    cout << "-> not locked" << endl;
  }
  switch (l->release()) {
  case 0:
    cout << "  ok" << endl;
    break;
  default:
    cout << "  error" << endl;
  }
  if (l->isLocked()) {
    cout << "-> locked" << endl << endl;
  } else {
    cout << "-> not locked" << endl << endl;
  }
  pthread_exit(NULL);
}

int main(void) {
  cout << "create locked lock" << endl;
  Lock l1(true);
  if (l1.isLocked()) {
    cout << "-> locked" << endl << endl;
  } else {
    cout << "-> not locked" << endl << endl;
  }

  cout << "trylock" << endl;
  switch (l1.trylock()) {
  case 0:
    cout << "  ok" << endl;
    break;
  case 1:
    cout << "  busy" << endl;
    break;
  default:
    cout << "  error" << endl;
  }
  if (l1.isLocked()) {
    cout << "-> locked" << endl << endl;
  } else {
    cout << "-> not locked" << endl << endl;
  }

  cout << "unlock" << endl;
  switch (l1.release()) {
  case 0:
    cout << "  ok" << endl;
    break;
  default:
    cout << "  error" << endl;
  }
  if (l1.isLocked()) {
    cout << "-> locked" << endl << endl;
  } else {
    cout << "-> not locked" << endl << endl;
  }

  cout << "trylock" << endl;
  switch (l1.trylock()) {
  case 0:
    cout << "  ok" << endl;
    break;
  case 1:
    cout << "  busy" << endl;
    break;
  default:
    cout << "  error" << endl;
  }
  if (l1.isLocked()) {
    cout << "-> locked" << endl << endl;
  } else {
    cout << "-> not locked" << endl << endl;
  }

  cout << "trylock" << endl;
  switch (l1.trylock()) {
  case 0:
    cout << "  ok" << endl;
    break;
  case 1:
    cout << "  busy" << endl;
    break;
  default:
    cout << "  error" << endl;
  }
  if (l1.isLocked()) {
    cout << "-> locked" << endl << endl;
  } else {
    cout << "-> not locked" << endl << endl;
  }

  cout << "unlock" << endl;
  switch (l1.release()) {
  case 0:
    cout << "  ok" << endl;
    break;
  default:
    cout << "  error" << endl;
  }
  if (l1.isLocked()) {
    cout << "-> locked" << endl << endl;
  } else {
    cout << "-> not locked" << endl << endl;
  }

  cout << "lock" << endl;
  switch (l1.lock()) {
  case 0:
    cout << "  ok" << endl;
    break;
  default:
    cout << "  error" << endl;
  }
  if (l1.isLocked()) {
    cout << "-> locked" << endl << endl;
  } else {
    cout << "-> not locked" << endl << endl;
  }

  cout << "timed lock" << endl;
  switch (l1.lock(1)) {
  case 0:
    cout << "  ok" << endl;
    break;
  case 1:
    cout << "  busy" << endl;
    break;
  default:
    cout << "  error" << endl;
  }
  if (l1.isLocked()) {
    cout << "-> locked" << endl << endl;
  } else {
    cout << "-> not locked" << endl << endl;
  }

  cout << "unlock" << endl;
  switch (l1.release()) {
  case 0:
    cout << "  ok" << endl;
    break;
  default:
    cout << "  error" << endl;
  }
  if (l1.isLocked()) {
    cout << "-> locked" << endl << endl;
  } else {
    cout << "-> not locked" << endl << endl;
  }

  cout << "timed lock" << endl;
  switch (l1.lock(1)) {
  case 0:
    cout << "  ok" << endl;
    break;
  case 1:
    cout << "  busy" << endl;
    break;
  default:
    cout << "  error" << endl;
  }
  if (l1.isLocked()) {
    cout << "-> locked" << endl << endl;
  } else {
    cout << "-> not locked" << endl << endl;
  }

  cout << "unlock" << endl;
  switch (l1.release()) {
  case 0:
    cout << "  ok" << endl;
    break;
  default:
    cout << "  error" << endl;
  }
  if (l1.isLocked()) {
    cout << "-> locked" << endl << endl;
  } else {
    cout << "-> not locked" << endl << endl;
  }

  cout << "double unlock" << endl;
  switch (l1.release()) {
  case 0:
    cout << "  ok" << endl;
    break;
  default:
    cout << "  error" << endl;
  }
  if (l1.isLocked()) {
    cout << "-> locked" << endl << endl;
  } else {
    cout << "-> not locked" << endl << endl;
  }

  cout << "timed lock" << endl;
  switch (l1.lock(1)) {
  case 0:
    cout << "  ok" << endl;
    break;
  case 1:
    cout << "  busy" << endl;
    break;
  default:
    cout << "  error" << endl;
  }
  if (l1.isLocked()) {
    cout << "-> locked" << endl << endl;
  } else {
    cout << "-> not locked" << endl << endl;
  }

  cout << "timed lock" << endl;
  switch (l1.lock(1)) {
  case 0:
    cout << "  ok" << endl;
    break;
  case 1:
    cout << "  busy" << endl;
    break;
  default:
    cout << "  error" << endl;
  }
  if (l1.isLocked()) {
    cout << "-> locked" << endl << endl;
  } else {
    cout << "-> not locked" << endl << endl;
  }

  cout << "(multithreaded)" << endl << endl;
  pthread_t child_task;
  if (pthread_create(&child_task, NULL, child, &l1)) {
    cout << "thread create failed " << endl;
  }
  sleep(1);

  cout << "timed lock" << endl;
  switch (l1.lock(1)) {
  case 0:
    cout << "  ok" << endl;
    break;
  case 1:
    cout << "  busy" << endl;
    break;
  default:
    cout << "  error" << endl;
  }
  if (l1.isLocked()) {
    cout << "-> locked" << endl << endl;
  } else {
    cout << "-> not locked" << endl << endl;
  }

  return 0;
}
