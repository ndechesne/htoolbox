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

// Sub-filter not tested here (should not be tested before filter is!)
// TODO test file type filter

#include <iostream>
#include <sys/stat.h>

using namespace std;

#include "test.h"

#include "files.h"
#include "conditions.h"
#include "filters.h"

int main(void) {
  Condition*  condition;
  Node*       node;

  report.setLevel(debug);

  cout << "Conditions test" << endl;
  node = new Node("this is/a path/to a file.txt", 'f', 0, 0, 0, 0, 0);

  cout << "Condition::name check" << endl;
  condition = new Condition(Condition::name, "to a file.txt", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name, "to a file.tx", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name, "o a file.txt", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name, "TO A FILE.TXT", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name, "TO A FILE.TX", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name, "O A FILE.TXT", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;

  cout << "Condition::name (no case) check" << endl;
  condition = new Condition(Condition::name, "to a file.txt", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name, "to a file.tx", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name, "o a file.txt", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name, "TO A FILE.TXT", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name, "TO A FILE.TX", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name, "O A FILE.TXT", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;

  cout << "Condition::name_start check" << endl;
  condition = new Condition(Condition::name_start, "to a file.txt", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_start, "to a file", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_start, "o a file", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_start, "TO A FILE.TXT", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_start, "TO A FILE", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_start, "O A FILE", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;

  cout << "Condition::name_start (no case) check" << endl;
  condition = new Condition(Condition::name_start, "to a file.txt", false,
    false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_start, "to a file", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_start, "o a file", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_start, "TO A FILE.TXT", false,
    false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_start, "TO A FILE", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_start, "O A FILE", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;

  cout << "Condition::name_end check" << endl;
  condition = new Condition(Condition::name_end, ".txt", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_end, ".tst", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_end, "and to a file.txt", false,
    true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_end, ".TXT", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_end, ".TST", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_end, "AND TO A FILE.TXT", false,
    true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;

  cout << "Condition::name_end (no case) check" << endl;
  condition = new Condition(Condition::name_end, ".txt", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_end, ".tst", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_end, "and to a file.txt", false,
    false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_end, ".TXT", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_end, ".TST", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_end, "AND TO A FILE.TXT", false,
    false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;

  cout << "Condition::name_regex check" << endl;
  condition = new Condition(Condition::name_regex, "^.*\\.txt", false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::name_regex, "^a.*\\.txt", false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;

  cout << "Condition::path check" << endl;
  condition = new Condition(Condition::path, "this is/a path/to a file.txt",
    false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path, "his is/a path/to a file.txt",
    false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path, "this is/a path/to a file.tx",
    false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path, "THIS IS/A PATH/TO A FILE.TXT",
    false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path, "HIS IS/A PATH/TO A FILE.TXT",
    false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path, "THIS IS/A PATH/TO A FILE.TX",
    false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;

  cout << "Condition::path (no case) check" << endl;
  condition = new Condition(Condition::path, "this is/a path/to a file.txt",
    false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path, "his is/a path/to a file.txt",
    false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path, "this is/a path/to a file.tx",
    false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path, "THIS IS/A PATH/TO A FILE.TXT",
    false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path, "HIS IS/A PATH/TO A FILE.TXT",
    false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path, "THIS IS/A PATH/TO A FILE.TX",
    false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;

  cout << "Condition::path_start check" << endl;
  condition = new Condition(Condition::path_start, "this is/a", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_start, "this was/a", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_start, "his is/a", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_start, "THIS IS/A", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_start, "THIS WAS/A", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_start, "HIS IS/A", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;

  cout << "Condition::path_start (no case) check" << endl;
  condition = new Condition(Condition::path_start, "this is/a", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_start, "this was/a", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_start, "his is/a", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_start, "THIS IS/A", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_start, "THIS WAS/A", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_start, "HIS IS/A", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;

  cout << "Condition::path_end check" << endl;
  condition = new Condition(Condition::path_end, ".txt", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_end, ".tst", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_end,
    "and this is/a path/to a file.txt", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_end, ".TXT", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_end, ".TST", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_end,
    "AND THIS IS/A PATH/TO A FILE.TXT", false, true);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;

  cout << "Condition::path_end (no case) check" << endl;
  condition = new Condition(Condition::path_end, ".txt", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_end, ".tst", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_end,
    "and this is/a path/to a file.txt", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_end, ".TXT", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_end, ".TST", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_end,
    "AND THIS IS/A PATH/TO A FILE.TXT", false, false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;

  cout << "Condition::path_regex check" << endl;
  condition = new Condition(Condition::path_regex, "^this.*path/.*\\.txt",
    false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;
  condition = new Condition(Condition::path_regex, "^this.*path/a.*\\.txt",
    false);
  if (condition->match(*node)) {
    condition->show();
  }
  delete condition;

  delete node;

  cout << "Condition::size_ge check" << endl;
  condition = new Condition(Condition::size_ge, 5000LL, false);
  node = new Node("", 'f', 0, 4000, 0, 0, 0);
  if (! condition->match(*node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 4999, 0, 0, 0);
  if (! condition->match(*node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 5000, 0, 0, 0);
  if (! condition->match(*node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 5001, 0, 0, 0);
  if (! condition->match(*node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 6000, 0, 0, 0);
  if (! condition->match(*node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  delete condition;

  cout << "Condition::size_gt check" << endl;
  condition = new Condition(Condition::size_gt, 5000LL, false);
  node = new Node("", 'f', 0, 4000, 0, 0, 0);
  if (! condition->match(*node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 4999, 0, 0, 0);
  if (! condition->match(*node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 5000, 0, 0, 0);
  if (! condition->match(*node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 5001, 0, 0, 0);
  if (! condition->match(*node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 6000, 0, 0, 0);
  if (! condition->match(*node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  delete condition;

  cout << "Condition::size_le check" << endl;
  condition = new Condition(Condition::size_le, 5000LL, false);
  node = new Node("", 'f', 0, 4000, 0, 0, 0);
  if (! condition->match(*node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 4999, 0, 0, 0);
  if (! condition->match(*node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 5000, 0, 0, 0);
  if (! condition->match(*node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 5001, 0, 0, 0);
  if (! condition->match(*node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 6000, 0, 0, 0);
  if (! condition->match(*node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  delete condition;

  cout << "Condition::size_lt check" << endl;
  condition = new Condition(Condition::size_lt, 5000LL, false);
  node = new Node("", 'f', 0, 4000, 0, 0, 0);
  if (! condition->match(*node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 4999, 0, 0, 0);
  if (! condition->match(*node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 5000, 0, 0, 0);
  if (! condition->match(*node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 5001, 0, 0, 0);
  if (! condition->match(*node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 6000, 0, 0, 0);
  if (! condition->match(*node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  delete condition;

  cout << "Condition::mode_and check" << endl;
  condition = new Condition(Condition::mode_and, 0111LL, false);
  node = new Node("", 'f', 0, 4000, 0, 0, 0777);
  if (! condition->match(*node)) {
    cout << "Not matching " << oct << node->mode() << dec << "" << endl;
  } else {
    cout << "Matching " << oct << node->mode() << dec << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 4999, 0, 0, 0666);
  if (! condition->match(*node)) {
    cout << "Not matching " << oct << node->mode() << dec << "" << endl;
  } else {
    cout << "Matching " << oct << node->mode() << dec << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 4999, 0, 0, 0111);
  if (! condition->match(*node)) {
    cout << "Not matching " << oct << node->mode() << dec << "" << endl;
  } else {
    cout << "Matching " << oct << node->mode() << dec << "" << endl;
  }
  delete node;
  delete condition;

  cout << "Condition::mode_eq check" << endl;
  condition = new Condition(Condition::mode_eq, 0111LL, false);
  node = new Node("", 'f', 0, 4000, 0, 0, 0777);
  if (! condition->match(*node)) {
    cout << "Not matching " << oct << node->mode() << dec << "" << endl;
  } else {
    cout << "Matching " << oct << node->mode() << dec << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 4999, 0, 0, 0666);
  if (! condition->match(*node)) {
    cout << "Not matching " << oct << node->mode() << dec << "" << endl;
  } else {
    cout << "Matching " << oct << node->mode() << dec << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 4999, 0, 0, 0111);
  if (! condition->match(*node)) {
    cout << "Not matching " << oct << node->mode() << dec << "" << endl;
  } else {
    cout << "Matching " << oct << node->mode() << dec << "" << endl;
  }
  delete node;
  delete condition;

  cout << "negation check (using Condition::mode_eq)" << endl;
  condition = new Condition(Condition::mode_eq, 0111LL, true);
  node = new Node("", 'f', 0, 4000, 0, 0, 0777);
  if (! condition->match(*node)) {
    cout << "Not matching " << oct << node->mode() << dec << "" << endl;
  } else {
    cout << "Matching " << oct << node->mode() << dec << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 4999, 0, 0, 0666);
  if (! condition->match(*node)) {
    cout << "Not matching " << oct << node->mode() << dec << "" << endl;
  } else {
    cout << "Matching " << oct << node->mode() << dec << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 4999, 0, 0, 0111);
  if (! condition->match(*node)) {
    cout << "Not matching " << oct << node->mode() << dec << "" << endl;
  } else {
    cout << "Matching " << oct << node->mode() << dec << "" << endl;
  }
  delete node;
  delete condition;

  return 0;
}
