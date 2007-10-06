/*
     Copyright (C) 2006-2007  Herve Fache

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

// TODO test file type filter

#include <iostream>
#include <sys/stat.h>

using namespace std;

#include "strings.h"
#include "files.h"
#include "conditions.h"
#include "filters.h"
#include "hbackup.h"

using namespace hbackup;

int hbackup::verbosity(void) {
  return 0;
}

int hbackup::terminating(void) {
  return 0;
}

int main(void) {
  Condition *condition;
  Node*     node;

  cout << "Conditions test" << endl;
  node = new Node("this is/a path/to a file.txt", 'f', 0, 0, 0, 0, 0);

  cout << "filter_name check" << endl;
  condition = new Condition(filter_name, "to a file.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 2.1" << endl;
  }
  delete condition;
  condition = new Condition(filter_name, "to a file.tx");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 2.2" << endl;
  }
  delete condition;
  condition = new Condition(filter_name, "o a file.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 2.3" << endl;
  }
  delete condition;

  cout << "filter_name_start check" << endl;
  condition = new Condition(filter_name_start, "to a file.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 3.1" << endl;
  }
  delete condition;
  condition = new Condition(filter_name_start, "to a file");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 3.2" << endl;
  }
  delete condition;
  condition = new Condition(filter_name_start, "o a file");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 3.3" << endl;
  }
  delete condition;

  cout << "filter_name_end check" << endl;
  condition = new Condition(filter_name_end, ".txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 4.1" << endl;
  }
  delete condition;
  condition = new Condition(filter_name_end, ".tst");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 4.2" << endl;
  }
  delete condition;
  condition = new Condition(filter_name_end, "and to a file.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 4.3" << endl;
  }
  delete condition;

  cout << "filter_name_regex check" << endl;
  condition = new Condition(filter_name_regex, "^.*\\.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 5.1" << endl;
  }
  delete condition;
  condition = new Condition(filter_name_regex, "^a.*\\.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 5.2" << endl;
  }
  delete condition;

  cout << "filter_path check" << endl;
  condition = new Condition(filter_path, "this is/a path/to a file.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 6.1" << endl;
  }
  delete condition;
  condition = new Condition(filter_path, "his is/a path/to a file.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 6.2" << endl;
  }
  delete condition;
  condition = new Condition(filter_path, "this is/a path/to a file.tx");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 6.3" << endl;
  }
  delete condition;

  cout << "filter_path_start check" << endl;
  condition = new Condition(filter_path_start, "this is/a");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 7.1" << endl;
  }
  delete condition;
  condition = new Condition(filter_path_start, "this was/a");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 7.2" << endl;
  }
  delete condition;
  condition = new Condition(filter_path_start, "his is/a");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 7.3" << endl;
  }
  delete condition;

  cout << "filter_path_end check" << endl;
  condition = new Condition(filter_path_end, ".txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 8.1" << endl;
  }
  delete condition;
  condition = new Condition(filter_path_end, ".tst");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 8.2" << endl;
  }
  delete condition;
  condition = new Condition(filter_path_end,
    "and this is/a path/to a file.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 8.3" << endl;
  }
  delete condition;

  cout << "filter_path_regex check" << endl;
  condition = new Condition(filter_path_regex, "^this.*path/.*\\.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 9.1" << endl;
  }
  delete condition;
  condition = new Condition(filter_path_regex, "^this.*path/a.*\\.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 9.2" << endl;
  }
  delete condition;

  delete node;

  cout << "filter_size_ge check" << endl;
  condition = new Condition(filter_size_ge, (off_t) 5000);
  node = new Node("", 'f', 0, 4000, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 4999, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 5000, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 5001, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 6000, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  delete condition;

  cout << "filter_size_gt check" << endl;
  condition = new Condition(filter_size_gt, (off_t) 5000);
  node = new Node("", 'f', 0, 4000, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 4999, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 5000, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 5001, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 6000, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  delete condition;

  cout << "filter_size_le check" << endl;
  condition = new Condition(filter_size_le, (off_t) 5000);
  node = new Node("", 'f', 0, 4000, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 4999, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 5000, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 5001, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 6000, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  delete condition;

  cout << "filter_size_lt check" << endl;
  condition = new Condition(filter_size_lt, (off_t) 5000);
  node = new Node("", 'f', 0, 4000, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 4999, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 5000, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 5001, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 6000, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  delete condition;

  return 0;
}