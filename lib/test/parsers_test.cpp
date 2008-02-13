/*
     Copyright (C) 2006-2008  Herve Fache

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

#include "hbackup.h"
#include "report.h"
#include "files.h"
#include "parsers.h"

using namespace hbackup;

int hbackup::verbosity(void) {
  return 2;
}

int hbackup::terminating(const char* function) {
  return 0;
}

class TestParser : public Parser {
  int           _index;
public:
  // Constructor for parsers list
  TestParser(Mode mode) : Parser(mode) {
    _index = 1;
    cout << "Contructing for list, mode: " << _mode << ", dummy: " << _dummy
      << endl;
  }
  // Constructor for directory parsing
  TestParser(Mode mode, const string& dir_path) {
    _mode = mode;
    _index = 2;
    cout << "Contructing for use, mode: " << mode << ", dummy: " << _dummy
      << ", path: " << dir_path << endl;
  }
  ~TestParser() {
    cout << "Destroying (index " << _index << ")" << endl;
  }
  // Just to know the parser used
  string name() const {
    return "test";
  }
  // This will create an appropriate parser for the directory if relevant
  Parser* isControlled(const string& dir_path) const {
    return new TestParser(_mode, dir_path);
  }
  // That tells use whether to ignore the file, i.e. not back it up
  bool ignore(const Node& node) { return false; }
  // For debug purposes
  void list() {
    cout << "Displaying list" << endl;
  }
};

int main(void) {
  Parsers*  parsers;
  Parser*   parser;

  report::setOutLevel(debug);

  parsers = new Parsers;
  cout << "Add TestParser to list" << endl;
  parsers->push_back(new TestParser(Parser::modified));
  cout << "Add IgnoreParser to list" << endl;
  parsers->push_back(new IgnoreParser(Parser::modified));  // Whatever mode...
  cout << "Check parsers against test directory" << endl;
  parser = parsers->isControlled("test");
  if (parser != NULL) {
    parser->list();
    delete parser;
  }
  parsers->list();
  delete parsers;

  return 0;
}
