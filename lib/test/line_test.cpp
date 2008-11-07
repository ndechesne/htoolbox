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

static void show(const LineBuffer& line) {
  cout
    << " size = " << line.size()
    << " cap. = " << line.capacity()
    << endl;
}

int main(void) {
#ifdef _DEBUG
  _Line_debug = true;
#endif
  {
    Line line0;
    cout << "(0) default c'tor:";
    show(line0);
    Line line1 = line0;
    cout << "(1) copy c'tor:";
    show(line1);
    Line line2("a");
    cout << "(2) const char* constructor:";
    show(line2);
    Line line3 = "def";
    cout << "(3) const char* constructor:";
    show(line3);
    Line line4 = line2;
    cout << "(4) copy constructor:";
    show(line4);
    line4 = line3;
    cout << "copy: " << line4;
    show(line4);
    line4 = "g";
    cout << "copy: " << line4;
    show(line4);
    line4 = "hijklmn";
    cout << "copy: " << line4;
    show(line4);
    cout << "operator[0]: " << &line4[0] << endl;
    cout << "operator[3]: " << &line4[3] << endl;
    cout << "erase(5): " << line4.erase(5);
    show(line4);
    cout << "find('j'): " << line4.find('j') << ", size = " << line4.size()
      << endl;
    cout << "find('z'): " << line4.find('z') << ", size = " << line4.size()
      << endl;
    cout << "find('j', 1): " << line4.find('j', 1) << ", size = "
      << line4.size() << endl;
    cout << "find('j', 4): " << line4.find('j', 4) << ", size = "
      << line4.size() << endl;
    cout << "append(\"op\"): " << line4.append("op");
    show(line4);
    cout << "append(\"qr\", 3): " << line4.append("qr", 3);
    show(line4);
    cout << "append(\"stuv\", 4): " << line4.append("stuv", 4);
    show(line4);
    cout << "append(\"wxy\", 6): " << line4.append("wxy", 6);
    show(line4);
    cout << "replace end of line" << endl;
    line0 = "\t0\tnew line";
    line1 = "\t12345\told line";
    cout << "(0) new line: " << line0;
    show(line0);
    cout << "(1) old line: " << line1 << ", size = " << line1.size() << endl;
    int pos = line1.find('\t', 1);
    cout << "second tab: " << pos << endl;
    cout << "end line: " << line1.append(&line0[2], pos);
    show(line1);

    Line line5;
    cout << "(5) default c'tor";
    show(line5);
    cout << line5 << endl;
    cout << "+=";
    line5 += "123456789";
    show(line5);
    cout << line5 << endl;
    cout << "+=";
    line5 += "1234567890";
    show(line5);
    cout << line5 << endl;
    cout << "+=";
    line5 += "123456789";
    show(line5);
    cout << line5 << endl;
    cout << "+=";
    line5 += "123456789";
    show(line5);
    cout << line5 << endl;
  }

  {
    cout << endl << "No-copy" << endl;

    cout << "Construct line from string" << endl;
    Line* line;
    line = new Line("abcdef");
    cout << "line (" << line->instances() << "): " << *line << endl;

    cout << "Modify line" << endl;
    *line = "defghi";
    cout << "line (" << line->instances() << "): " << *line << endl;

    cout << "Append to line" << endl;
    *line += "jklmno";
    cout << "line (" << line->instances() << "): " << *line << endl;

    cout << "Copy-construct line2" << endl;
    Line* line2;
    line2 = new Line(*line);
    cout << "line (" << line->instances() << "): " << *line << endl;
    cout << "line2 (" << line2->instances() << "): " << *line2 << endl;

    cout << "Modify line" << endl;
    *line = "stuvwx";
    cout << "line (" << line->instances() << "): " << *line << endl;
    cout << "line2 (" << line2->instances() << "): " << *line2 << endl;

    cout << "Delete line" << endl;
    delete line;
    cout << "line2 (" << line2->instances() << "): " << *line2 << endl;

    cout << "Default-construct line" << endl;
    line = new Line;
    cout << "line (" << line->instances() << "): " << *line << endl;
    cout << "line2 (" << line2->instances() << "): " << *line2 << endl;

    cout << "Copy line2 into line" << endl;
    *line = *line2;
    cout << "line (" << line->instances() << "): " << *line << endl;
    cout << "line2 (" << line2->instances() << "): " << *line2 << endl;

    cout << "Append to line" << endl;
    *line += "yz";
    cout << "line (" << line->instances() << "): " << *line << endl;
    cout << "line2 (" << line2->instances() << "): " << *line2 << endl;

    cout << "Append line to line2" << endl;
    *line2 += *line;
    cout << "line (" << line->instances() << "): " << *line << endl;
    cout << "line2 (" << line2->instances() << "): " << *line2 << endl;

    cout << "Copy line into line2" << endl;
    *line2 = *line;
    cout << "line (" << line->instances() << "): " << *line << endl;
    cout << "line2 (" << line2->instances() << "): " << *line2 << endl;

    cout << "Erase from line2" << endl;
    line2->erase(4);
    cout << "line (" << line->instances() << "): " << *line << endl;
    cout << "line2 (" << line2->instances() << "): " << *line2 << endl;

    cout << "Delete line2" << endl;
    delete line2;
    cout << "line (" << line->instances() << "): " << *line << endl;

    cout << "Copy-construct line2" << endl;
    line2 = new Line(*line);
    cout << "line (" << line->instances() << "): " << *line << endl;
    cout << "line2 (" << line2->instances() << "): " << *line2 << endl;

    cout << "Erase from line2" << endl;
    line2->erase(4);
    cout << "line (" << line->instances() << "): " << *line << endl;
    cout << "line2 (" << line2->instances() << "): " << *line2 << endl;

    cout << "Delete line" << endl;
    delete line;
  }

  cout << endl << "end of tests" << endl;

  return 0;
}
