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

#include "line.h"

using namespace hbackup;

int main(void) {
  Line line0;
  Line line1 = line0;
  Line line2("abc");
  cout << "const char* constructor: " << line2 << endl;
  Line line3 = "def";
  cout << "const char* constructor: " << line3 << endl;
  Line line4 = line2;
  cout << "copy constructor: " << line4 << endl;
  line4 = line3;
  cout << "copy: " << line4 << endl;
  line4 = "g";
  cout << "copy: " << line4 << endl;
  line4 = "hijklmn";
  cout << "copy: " << line4 << endl;
  cout << "operator[0]: " << &line4[0] << endl;
  cout << "operator[3]: " << &line4[3] << endl;
  cout << "erase(5): " << line4.erase(5) << endl;
  cout << "find('j'): " << line4.find('j') << endl;
  cout << "find('z'): " << line4.find('z') << endl;
  cout << "find('j', 1): " << line4.find('j', 1) << endl;
  cout << "find('j', 4): " << line4.find('j', 4) << endl;
  cout << "append(\"op\"): " << line4.append("op") << endl;
  cout << "append(\"qr\", 3): " << line4.append("qr", 3) << endl;
  cout << "append(\"stuv\", 4): " << line4.append("stuv", 4) << endl;
  cout << "append(\"wxyz\", 6, 3): " << line4.append("wxyz", 6, 3) << endl;
  cout << "replace end of line" << endl;
  line0 = "\t0\tnew line";
  line1 = "\t12345\told line";
  cout << "new line: " << line0 << endl;
  cout << "old line: " << line1 << endl;
  int pos = line1.find('\t', 1);
  cout << "second tab: " << pos << endl;
  cout << "end line: " << line1.append(&line0[2], pos) << endl;

  cout << "end of tests" << endl;

  return 0;
}
