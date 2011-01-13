/*
     Copyright (C) 2011  Herve Fache

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

#include <stdio.h>

#include <string>

using namespace std;

#include <observer.h>

using namespace htoolbox;

class Bottom : public Observee {
  string _name;
public:
  Bottom(const char* name) : _name(name) {}
};

class Middle : public Observer, public Observee {
  string _name;
public:
  Middle(const char* name) : _name(name) {}
  void notify() {
    printf("%s: %s called\n", _name.c_str(), __PRETTY_FUNCTION__);
    notifyObservers();
  }
};

class Top : public Observer {
  string _name;
public:
  Top(const char* name) : _name(name) {}
  void notify() {
    printf("%s: %s called\n", _name.c_str(), __PRETTY_FUNCTION__);
  }
};

int main() {
  Middle m1("m1");
  Middle m2("m2");
  Top t1("t1");
  Top t2("t2");
  Bottom b1("b1");
  Bottom b2("b2");

  // b1   -> | m1 | -> t1
  // b2 | -> |    | -> t2
  //    | -> m2

  printf("register m1 to b1\n");
  b1.registerObserver(&m1);
  printf("register m1 to b2\n");
  b2.registerObserver(&m1);
  printf("register m2 to b2\n");
  b2.registerObserver(&m2);
  printf("register t1 to m1\n");
  m1.registerObserver(&t1);
  printf("register t2 to m1\n");
  m1.registerObserver(&t2);

  printf("call t1 notify()\n");
  t1.notify();

  printf("call t2 notify()\n");
  t2.notify();

  printf("call m1 notifyObservers()\n");
  m1.notifyObservers();

  printf("call m2 notifyObservers()\n");
  m2.notifyObservers();

  printf("call b1 notifyObservers()\n");
  b1.notifyObservers();

  printf("call b2 notifyObservers()\n");
  b2.notifyObservers();

  printf("exit\n");
  return 0;
}
