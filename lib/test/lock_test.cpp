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

int main(void) {
  cout << "create locked lock" << endl;
  Lock l1(true);

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

  cout << "unlock" << endl;
  switch (l1.unlock()) {
  case 0:
    cout << "  ok" << endl;
    break;
  default:
    cout << "  error" << endl;
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

  cout << "unlock" << endl;
  switch (l1.unlock()) {
  case 0:
    cout << "  ok" << endl;
    break;
  default:
    cout << "  error" << endl;
  }

  cout << "lock" << endl;
  switch (l1.lock()) {
  case 0:
    cout << "  ok" << endl;
    break;
  default:
    cout << "  error" << endl;
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

  cout << "unlock" << endl;
  switch (l1.unlock()) {
  case 0:
    cout << "  ok" << endl;
    break;
  default:
    cout << "  error" << endl;
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

  return 0;
}
