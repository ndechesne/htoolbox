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

// Sub-filter not tested here (should not be tested before filter is!)
// TODO test file type filter

#include <iostream>
#include <sys/stat.h>

using namespace std;

#include "files.h"
#include "conditions.h"
#include "filters.h"
#include "hbackup.h"

using namespace hbackup;

int hbackup::verbosity(void) {
  return 2;
}

int hbackup::terminating(const char* function) {
  return 0;
}

int main(void) {
  Condition*  condition;
  Node*       node;

  cout << "Conditions test" << endl;
  node = new Node("this is/a path/to a file.txt", 'f', 0, 0, 0, 0, 0);

  cout << "Condition::name check" << endl;
  condition = new Condition(Condition::name, "to a file.txt", false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 2.1" << endl;
  }
  delete condition;
  condition = new Condition(Condition::name, "to a file.tx", false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 2.2" << endl;
  }
  delete condition;
  condition = new Condition(Condition::name, "o a file.txt", false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 2.3" << endl;
  }
  delete condition;

  cout << "Condition::name_start check" << endl;
  condition = new Condition(Condition::name_start, "to a file.txt", false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 3.1" << endl;
  }
  delete condition;
  condition = new Condition(Condition::name_start, "to a file", false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 3.2" << endl;
  }
  delete condition;
  condition = new Condition(Condition::name_start, "o a file", false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 3.3" << endl;
  }
  delete condition;

  cout << "Condition::name_end check" << endl;
  condition = new Condition(Condition::name_end, ".txt", false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 4.1" << endl;
  }
  delete condition;
  condition = new Condition(Condition::name_end, ".tst", false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 4.2" << endl;
  }
  delete condition;
  condition = new Condition(Condition::name_end, "and to a file.txt", false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 4.3" << endl;
  }
  delete condition;

  cout << "Condition::name_regex check" << endl;
  condition = new Condition(Condition::name_regex, "^.*\\.txt", false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 5.1" << endl;
  }
  delete condition;
  condition = new Condition(Condition::name_regex, "^a.*\\.txt", false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 5.2" << endl;
  }
  delete condition;

  cout << "Condition::path check" << endl;
  condition = new Condition(Condition::path, "this is/a path/to a file.txt",
    false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 6.1" << endl;
  }
  delete condition;
  condition = new Condition(Condition::path, "his is/a path/to a file.txt",
    false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 6.2" << endl;
  }
  delete condition;
  condition = new Condition(Condition::path, "this is/a path/to a file.tx",
    false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 6.3" << endl;
  }
  delete condition;

  cout << "Condition::path_start check" << endl;
  condition = new Condition(Condition::path_start, "this is/a", false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 7.1" << endl;
  }
  delete condition;
  condition = new Condition(Condition::path_start, "this was/a", false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 7.2" << endl;
  }
  delete condition;
  condition = new Condition(Condition::path_start, "his is/a", false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 7.3" << endl;
  }
  delete condition;

  cout << "Condition::path_end check" << endl;
  condition = new Condition(Condition::path_end, ".txt", false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 8.1" << endl;
  }
  delete condition;
  condition = new Condition(Condition::path_end, ".tst", false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 8.2" << endl;
  }
  delete condition;
  condition = new Condition(Condition::path_end,
    "and this is/a path/to a file.txt", false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 8.3" << endl;
  }
  delete condition;

  cout << "Condition::path_regex check" << endl;
  condition = new Condition(Condition::path_regex, "^this.*path/.*\\.txt",
    false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 9.1" << endl;
  }
  delete condition;
  condition = new Condition(Condition::path_regex, "^this.*path/a.*\\.txt",
    false);
  if (condition->match("this is/a path/", *node)) {
    cout << "match 9.2" << endl;
  }
  delete condition;

  delete node;

  cout << "Condition::size_ge check" << endl;
  condition = new Condition(Condition::size_ge, (off64_t) 5000, false);
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

  cout << "Condition::size_gt check" << endl;
  condition = new Condition(Condition::size_gt, (off64_t) 5000, false);
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

  cout << "Condition::size_le check" << endl;
  condition = new Condition(Condition::size_le, (off64_t) 5000, false);
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

  cout << "Condition::size_lt check" << endl;
  condition = new Condition(Condition::size_lt, (off64_t) 5000, false);
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

  cout << "Condition::mode_and check" << endl;
  condition = new Condition(Condition::mode_and, (off64_t) 0111, false);
  node = new Node("", 'f', 0, 4000, 0, 0, 0777);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << oct << node->mode() << dec << "" << endl;
  } else {
    cout << "Matching " << oct << node->mode() << dec << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 4999, 0, 0, 0666);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << oct << node->mode() << dec << "" << endl;
  } else {
    cout << "Matching " << oct << node->mode() << dec << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 4999, 0, 0, 0111);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << oct << node->mode() << dec << "" << endl;
  } else {
    cout << "Matching " << oct << node->mode() << dec << "" << endl;
  }
  delete node;
  delete condition;

  cout << "Condition::mode_eq check" << endl;
  condition = new Condition(Condition::mode_eq, (off64_t) 0111, false);
  node = new Node("", 'f', 0, 4000, 0, 0, 0777);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << oct << node->mode() << dec << "" << endl;
  } else {
    cout << "Matching " << oct << node->mode() << dec << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 4999, 0, 0, 0666);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << oct << node->mode() << dec << "" << endl;
  } else {
    cout << "Matching " << oct << node->mode() << dec << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 4999, 0, 0, 0111);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << oct << node->mode() << dec << "" << endl;
  } else {
    cout << "Matching " << oct << node->mode() << dec << "" << endl;
  }
  delete node;
  delete condition;

  cout << "negation check (using Condition::mode_eq)" << endl;
  condition = new Condition(Condition::mode_eq, (off64_t) 0111, true);
  node = new Node("", 'f', 0, 4000, 0, 0, 0777);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << oct << node->mode() << dec << "" << endl;
  } else {
    cout << "Matching " << oct << node->mode() << dec << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 4999, 0, 0, 0666);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << oct << node->mode() << dec << "" << endl;
  } else {
    cout << "Matching " << oct << node->mode() << dec << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 4999, 0, 0, 0111);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << oct << node->mode() << dec << "" << endl;
  } else {
    cout << "Matching " << oct << node->mode() << dec << "" << endl;
  }
  delete node;
  delete condition;

  return 0;
}
