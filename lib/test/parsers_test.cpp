/*
     Copyright (C) 2006-2010  Herve Fache

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

using namespace std;

#include <iostream>
#include <list>

#include "test.h"

#include "files.h"
#include "parsers.h"

class TestParser : public IParser {
  int           _index;
public:
  TestParser(Mode mode = master, const string& dir_path = "") :
      IParser(mode, dir_path) {
    if (dir_path == "") {
      _index = 1;
      cout << "Contructing for list, mode: " << _mode << ", no parsing: "
        << _no_parsing << endl;
    } else {
      _index = 2;
      cout << "Contructing for use, mode: " << mode << ", no parsing: "
        << _no_parsing << ", path: " << dir_path << endl;
    }
  }
  ~TestParser() {
    cout << "Destroying (index " << _index << ")" << endl;
  }
  // Just to know the parser used
  const char* name() const { return "test"; }
  const char* code() const { return "tst"; }
  // This will create an appropriate parser for the directory if relevant
  IParser* createChildIfControlled(const string& dir_path) {
    return new TestParser(_mode, dir_path);
  }
  // That tells use whether to ignore the file, i.e. not back it up
  bool ignore(const Node& node) {
    (void) node;
    return false;
  }
  // For debug purposes
  void show(int level) {
    (void) level;
    cout << "Displaying list" << endl;
  }
};

int main(void) {
  Parsers* parsers;
  IParser* parser;

  report.setLevel(debug);

  parsers = new Parsers;
  cout << "Add TestParser to list" << endl;
  parsers->push_back(new TestParser(IParser::modified));
  cout << "Add IgnoreParser to list" << endl;
  parsers->push_back(new IgnoreParser(IParser::modified));  // Whatever mode...
  cout << "Check parsers against test directory" << endl;
  parser = parsers->createParserIfControlled("test");
  if (parser != NULL) {
    parser->show();
    delete parser;
  }
  cout << "List: " << parsers->size() << " parser(s)" << endl;
  parsers->show(1);
  delete parsers;

  return 0;
}
