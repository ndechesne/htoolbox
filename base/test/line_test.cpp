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

class LineDebug : public Line {
public:
  LineDebug(const char* line = "") : Line(line) {}
  LineDebug& operator=(const Line& line) {
    return static_cast<LineDebug&>(Line::operator=(line));
  }
  LineDebug& operator=(const char* line) {
    return static_cast<LineDebug&>(Line::operator=(line));
  }
  void show(const LineBuffer* line = NULL) {
    if (line == NULL) {
      line = _line;
    }
    std::cout
      << " bf " << line->_address
      << " in " << line->_instances
      << " cp " << line->capacity()
      << " sz " << line->size()
      << " '" << &(*line)[0] << "'"
      << std::endl;
  }
};

int main(void) {
#ifdef _DEBUG
//   __Line_debug = true;
#endif
  {
    LineDebug line0;
    cout << "(0) default c'tor:";
    line0.show();
    LineDebug line1 = line0;
    cout << "(1) copy c'tor:";
    line1.show();
    LineDebug line2("a");
    cout << "(2) const char* constructor:";
    line2.show();
    LineDebug line3 = "def";
    cout << "(3) const char* constructor:";
    line3.show();
    LineDebug line4(line2);
    cout << "(4) copy constructor:";
    line4.show();
    line4 = line3;
    cout << "copy: ";
    line4.show();
    line4 = "g";
    cout << "copy: ";
    line4.show();
    line4 = "hijklmn";
    cout << "copy: ";
    line4.show();
    cout << "operator[0]: " << &line4[0] << endl;
    cout << "operator[3]: " << &line4[3] << endl;
    cout << "erase(5): " << line4.erase(5);
    line4.show();
    cout << "find('j'): " << line4.find('j') << ", size = " << line4.size()
      << endl;
    cout << "find('z'): " << line4.find('z') << ", size = " << line4.size()
      << endl;
    cout << "find('j', 1): " << line4.find('j', 1) << ", size = "
      << line4.size() << endl;
    cout << "find('j', 4): " << line4.find('j', 4) << ", size = "
      << line4.size() << endl;
    cout << "append(\"op\"): " << line4.append("op");
    line4.show();
    cout << "append(\"qr\", 3): " << line4.append("qr", 3);
    line4.show();
    cout << "append(\"stuv\", 4): " << line4.append("stuv", 4);
    line4.show();
    cout << "append(\"wxy\", 6): " << line4.append("wxy", 6);
    line4.show();
    cout << "replace end of line" << endl;
    line0 = "\t0\tnew line";
    line1 = "\t12345\told line";
    cout << "(0) new line:";
    line0.show();
    cout << "(1) old line:";
    line1.show();
    int pos = line1.find('\t', 1);
    cout << "second tab: " << pos << endl;
    cout << "end line: " << line1.append(&line0[2], pos);
    line1.show();

    LineDebug line5;
    cout << "(5) default c'tor";
    line5.show();
    line5 += "123456789";
    cout << "+=";
    line5.show();
    line5 += "1234567890";
    cout << "+=";
    line5.show();
    line5 += "123456789";
    cout << "+=";
    line5.show();
    line5 += "123456789";
    cout << "+=";
    line5.show();
  }

  {
    cout << endl << "No-copy" << endl;

    cout << "Construct line from string" << endl;
    LineDebug* line;
    line = new LineDebug("abcdef");
    cout << "line"; line->show();

    cout << "Modify line" << endl;
    *line = "defghi";
    cout << "line"; line->show();

    cout << "Append to line" << endl;
    *line += "jklmno";
    cout << "line"; line->show();

    cout << endl;
    cout << "Copy-construct line2" << endl;
    LineDebug* line2;
    line2 = new LineDebug(*line);
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Modify line" << endl;
    *line = "stuvwx";
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Delete line" << endl;
    delete line;
    cout << "line2"; line2->show();

    cout << "Default-construct line" << endl;
    line = new LineDebug;
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Copy line2 into line" << endl;
    *line = *line2;
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Append to line" << endl;
    *line += "yz";
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Append line to line2" << endl;
    *line2 += *line;
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Copy line into line2" << endl;
    *line2 = *line;
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Erase from line2" << endl;
    line2->erase(4);
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Delete line2" << endl;
    delete line2;
    cout << "line"; line->show();

    cout << endl << "Erase" << endl;
    cout << "Copy-construct line2" << endl;
    line2 = new LineDebug(*line);
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Erase from line2" << endl;
    line2->erase(4);
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Swap" << endl;
    line2->swap(*line);
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Swap back" << endl;
    line->swap(*line2);
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Delete line2" << endl;
    delete line2;
    cout << "line"; line->show();

    cout << endl << "Append" << endl;
    cout << "Copy-construct line2" << endl;
    line2 = new LineDebug(*line);
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Append to line2" << endl;
    *line2 += Line("ABCD");
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Delete line2" << endl;
    delete line2;
    cout << "line"; line->show();

    cout << endl << "Get instance" << endl;
    cout << "Copy-construct line2" << endl;
    line2 = new LineDebug(*line);
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Get first character of line2" << endl;
    if ((*line2)[0] != 'd') {
      cout << "Wrong!" << endl;
    }
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Set first character of line2" << endl;
    line2->buffer()[0] = 'H';
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Delete line2" << endl;
    delete line2;
    cout << "line"; line->show();

    cout << endl << "Replace extra buffer with bigger one" << endl;
    cout << "Copy-construct line2" << endl;
    line2 = new LineDebug(*line);
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Set value to line2" << endl;
    *line2 = "this is a line ";
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Increase line2 to more than 128" << endl;
    while (line2->size() <= 128) {
      *line2 += "this is a increased line ";
    }
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Another buffer for line" << endl;
    *line = Line("this is another line");
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Another buffer for line2" << endl;
    *line2 = Line("this is another line 2");
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Yet another buffer for line2 (=> delete main buffer)" << endl;
    *line2 = Line("this is yet another line ");
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Copy line2 into line (=> same buffer)" << endl;
    *line = *line2;
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Modify line2 (=> re-use big buffer)" << endl;
    *line2 = "abc";
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Copy line2 into line (=> give big buffer)" << endl;
    *line = *line2;
    cout << "line"; line->show();
    cout << "line2"; line2->show();

    cout << "Delete line2" << endl;
    delete line2;
    cout << "line"; line->show();

    cout << "Another buffer for line (=> replace extra buffer)" << endl;
    *line = Line("this is another line");
    cout << "line"; line->show();

    cout << "Delete line (=> two buffers deleted)" << endl;
    delete line;
  }

  cout << endl << "end of tests" << endl;

  return 0;
}
