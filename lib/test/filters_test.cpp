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
#include "filters.h"
#include "hbackup.h"

using namespace hbackup;

int hbackup::verbosity(void) {
  return 0;
}

int hbackup::terminating(void) {
  return 0;
}

void filter_show(const Filters& filters) {
  /* Read through list of filters */
  Filters::const_iterator rule;
  for (rule = filters.begin(); rule != filters.end(); rule++) {
    cout << "-> List " << rule->size() << " condition(s)\n";
    /* Read through list of conditions in filter */
    Filter::const_iterator condition;
    for (condition = rule->begin(); condition != rule->end(); condition++) {
      condition->show();
    }
  }
}

/* TODO Test file type check */
int main(void) {
  Filters   *filter = NULL;
  Filters   *filter2 = NULL;
  Condition *condition;
  Node*     node;

  cout << "Conditions test\n";
  node = new Node("this is/a path/to a file.txt", 'f', 0, 0, 0, 0, 0);

  cout << "filter_name check\n";
  condition = new Condition(filter_name, "to a file.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 2.1\n";
  }
  delete condition;
  condition = new Condition(filter_name, "to a file.tx");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 2.2\n";
  }
  delete condition;
  condition = new Condition(filter_name, "o a file.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 2.3\n";
  }
  delete condition;

  cout << "filter_name_start check\n";
  condition = new Condition(filter_name_start, "to a file.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 3.1\n";
  }
  delete condition;
  condition = new Condition(filter_name_start, "to a file");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 3.2\n";
  }
  delete condition;
  condition = new Condition(filter_name_start, "o a file");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 3.3\n";
  }
  delete condition;

  cout << "filter_name_end check\n";
  condition = new Condition(filter_name_end, ".txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 4.1\n";
  }
  delete condition;
  condition = new Condition(filter_name_end, ".tst");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 4.2\n";
  }
  delete condition;
  condition = new Condition(filter_name_end, "and to a file.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 4.3\n";
  }
  delete condition;

  cout << "filter_name_regex check\n";
  condition = new Condition(filter_name_regex, "^.*\\.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 5.1\n";
  }
  delete condition;
  condition = new Condition(filter_name_regex, "^a.*\\.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 5.2\n";
  }
  delete condition;

  cout << "filter_path check\n";
  condition = new Condition(filter_path, "this is/a path/to a file.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 6.1\n";
  }
  delete condition;
  condition = new Condition(filter_path, "his is/a path/to a file.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 6.2\n";
  }
  delete condition;
  condition = new Condition(filter_path, "this is/a path/to a file.tx");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 6.3\n";
  }
  delete condition;

  cout << "filter_path_start check\n";
  condition = new Condition(filter_path_start, "this is/a");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 7.1\n";
  }
  delete condition;
  condition = new Condition(filter_path_start, "this was/a");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 7.2\n";
  }
  delete condition;
  condition = new Condition(filter_path_start, "his is/a");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 7.3\n";
  }
  delete condition;

  cout << "filter_path_end check\n";
  condition = new Condition(filter_path_end, ".txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 8.1\n";
  }
  delete condition;
  condition = new Condition(filter_path_end, ".tst");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 8.2\n";
  }
  delete condition;
  condition = new Condition(filter_path_end, "and this is/a path/to a file.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 8.3\n";
  }
  delete condition;

  cout << "filter_path_regex check\n";
  condition = new Condition(filter_path_regex, "^this.*path/.*\\.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 9.1\n";
  }
  delete condition;
  condition = new Condition(filter_path_regex, "^this.*path/a.*\\.txt");
  if (condition->match("this is/a path/", *node)) {
    cout << "match 9.2\n";
  }
  delete condition;

  delete node;


  cout << "\nMatch function test\n";
  condition = new Condition(filter_size_below, (off_t) 5000);
  node = new Node("", 'f', 0, 4000, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "\n";
  } else {
    cout << "Matching " << node->size() << "\n";
  }
  delete node;
  node = new Node("", 'f', 0, 6000, 0, 0, 0);
  if (! condition->match("this is/a path/", *node)) {
    cout << "Not matching " << node->size() << "\n";
  } else {
    cout << "Matching " << node->size() << "\n";
  }
  delete node;
  delete condition;


  cout << "\nSimple rules test\n";
  filter = new Filters;
  filter->push_back(Filter(Condition(filter_path_regex, "^/to a.*\\.txt")));
  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

  filter->push_back(Filter(Condition(filter_path_regex, "^/to a.*\\.t.t")));
  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

  node = new Node("to a file.txt", 'f', 0, 0, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching 1\n";
  }
  delete node;
  node = new Node("to a file.tst", 'f', 0, 0, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching 2\n";
  }
  delete node;
  node = new Node("to a file.tsu", 'f', 0, 0, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching 3\n";
  }
  delete node;

  filter2 = new Filters;
  node = new Node("to a file.txt", 'f', 0, 0, 0, 0, 0);
  if (! filter2->match("this is/a path/", *node)) {
    cout << "Not matching +1\n";
  }
  delete node;
  node = new Node("to a file.tst", 'f', 0, 0, 0, 0, 0);
  if (! filter2->match("this is/a path/", *node)) {
    cout << "Not matching +2\n";
  }
  delete node;
  node = new Node("to a file.tsu", 'f', 0, 0, 0, 0, 0);
  if (! filter2->match("this is/a path/", *node)) {
    cout << "Not matching +3\n";
  }
  delete node;


  filter2->push_back(Filter(Condition(filter_path_regex, "^/to a.*\\.txt")));
  cout << ">List " << filter2->size() << " rule(s):\n";
  filter_show(*filter2);

  filter2->push_back(Filter(Condition(filter_path_regex, "^/to a.*\\.t.t")));
  cout << ">List " << filter2->size() << " rule(s):\n";
  filter_show(*filter2);

  node = new Node("to a file.txt", 'f', 0, 0, 0, 0, 0);
  if (! filter2->match("/", *node)) {
    cout << "Not matching +1\n";
  }
  delete node;
  node = new Node("to a file.tst", 'f', 0, 0, 0, 0, 0);
  if (! filter2->match("/", *node)) {
    cout << "Not matching +2\n";
  }
  delete node;
  node = new Node("to a file.tsu", 'f', 0, 0, 0, 0, 0);
  if (! filter2->match("/", *node)) {
    cout << "Not matching +3\n";
  }
  delete node;
  delete filter2;

  node = new Node("to a file.txt", 'f', 0, 0, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching 1\n";
  }
  delete node;
  node = new Node("to a file.tst", 'f', 0, 0, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching 2\n";
  }
  delete node;
  node = new Node("to a file.tsu", 'f', 0, 0, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching 3\n";
  }
  delete node;
  delete filter;

  filter = new Filters;
  node = new Node("", 'f', 0, 0, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "\n";
  } else {
    cout << "Matching " << node->size() << "\n";
  }
  delete node;
  node = new Node("", 'f', 0, 1000, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "\n";
  } else {
    cout << "Matching " << node->size() << "\n";
  }
  delete node;
  node = new Node("", 'f', 0, 1000000, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "\n";
  } else {
    cout << "Matching " << node->size() << "\n";
  }
  delete node;

  /* File type is always 'f' */
  filter->push_back(Filter(Condition(filter_size_below, (off_t) 500)));
  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

  node = new Node("", 'f', 0, 0, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "\n";
  } else {
    cout << "Matching " << node->size() << "\n";
  }
  delete node;
  node = new Node("", 'f', 0, 1000, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "\n";
  } else {
    cout << "Matching " << node->size() << "\n";
  }
  delete node;
  node = new Node("", 'f', 0, 1000000, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "\n";
  } else {
    cout << "Matching " << node->size() << "\n";
  }
  delete node;

  /* File type is always 'f' */
  filter->push_back(Filter(Condition(filter_size_above, (off_t) 5000)));
  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

  node = new Node("", 'f', 0, 0, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "\n";
  } else {
    cout << "Matching " << node->size() << "\n";
  }
  delete node;
  node = new Node("", 'f', 0, 1000, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "\n";
  } else {
    cout << "Matching " << node->size() << "\n";
  }
  delete node;
  node = new Node("", 'f', 0, 1000000, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "\n";
  } else {
    cout << "Matching " << node->size() << "\n";
  }
  delete node;

  delete filter;

  /* Test complex rules */
  cout << "\nComplex rules test\n";
  filter = new Filters;

  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

  filter->push_back(Filter());
  Filter* rule = &filter->back();

  rule->push_back(Condition(filter_size_below, (off_t) 500));
  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

  rule->push_back(Condition(filter_size_above, (off_t) 400));
  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

  node = new Node("", 'f', 0, 600, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "\n";
  } else {
    cout << "Matching " << node->size() << "\n";
  }
  delete node;
  node = new Node("", 'f', 0, 500, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "\n";
  } else {
    cout << "Matching " << node->size() << "\n";
  }
  delete node;
  node = new Node("", 'f', 0, 450, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "\n";
  } else {
    cout << "Matching " << node->size() << "\n";
  }
  delete node;
  node = new Node("", 'f', 0, 400, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "\n";
  } else {
    cout << "Matching " << node->size() << "\n";
  }
  delete node;
  node = new Node("", 'f', 0, 300, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "\n";
  } else {
    cout << "Matching " << node->size() << "\n";
  }
  delete node;

  delete filter;

  return 0;
}
