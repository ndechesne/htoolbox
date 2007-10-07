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

#include <iostream>

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

void filter_show(const Filters& filters) {
  /* Read through list of filters */
  Filters::const_iterator rule;
  for (rule = filters.begin(); rule != filters.end(); rule++) {
    cout << "-> List " << rule->size() << " condition(s)" << endl;
    /* Read through list of conditions in filter */
    Filter::const_iterator condition;
    for (condition = rule->begin(); condition != rule->end(); condition++) {
      condition->show();
    }
  }
}

int main(void) {
  Filters   *filter = NULL;
  Filters   *filter2 = NULL;
  Node*     node;

  cout << "\nSimple rules test" << endl;
  filter = new Filters;
  filter->push_back(Filter(Condition(condition_path_regex, "^/to a.*\\.txt",
    false)));
  cout << ">List " << filter->size() << " rule(s):" << endl;
  filter_show(*filter);

  filter->push_back(Filter(Condition(condition_path_regex, "^/to a.*\\.t.t",
    false)));
  cout << ">List " << filter->size() << " rule(s):" << endl;
  filter_show(*filter);

  node = new Node("to a file.txt", 'f', 0, 0, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching 1" << endl;
  }
  delete node;
  node = new Node("to a file.tst", 'f', 0, 0, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching 2" << endl;
  }
  delete node;
  node = new Node("to a file.tsu", 'f', 0, 0, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching 3" << endl;
  }
  delete node;

  filter2 = new Filters;
  node = new Node("to a file.txt", 'f', 0, 0, 0, 0, 0);
  if (! filter2->match("this is/a path/", *node)) {
    cout << "Not matching +1" << endl;
  }
  delete node;
  node = new Node("to a file.tst", 'f', 0, 0, 0, 0, 0);
  if (! filter2->match("this is/a path/", *node)) {
    cout << "Not matching +2" << endl;
  }
  delete node;
  node = new Node("to a file.tsu", 'f', 0, 0, 0, 0, 0);
  if (! filter2->match("this is/a path/", *node)) {
    cout << "Not matching +3" << endl;
  }
  delete node;


  filter2->push_back(Filter(Condition(condition_path_regex, "^/to a.*\\.txt",
    false)));
  cout << ">List " << filter2->size() << " rule(s):" << endl;
  filter_show(*filter2);

  filter2->push_back(Filter(Condition(condition_path_regex, "^/to a.*\\.t.t",
    false)));
  cout << ">List " << filter2->size() << " rule(s):" << endl;
  filter_show(*filter2);

  node = new Node("to a file.txt", 'f', 0, 0, 0, 0, 0);
  if (! filter2->match("/", *node)) {
    cout << "Not matching +1" << endl;
  }
  delete node;
  node = new Node("to a file.tst", 'f', 0, 0, 0, 0, 0);
  if (! filter2->match("/", *node)) {
    cout << "Not matching +2" << endl;
  }
  delete node;
  node = new Node("to a file.tsu", 'f', 0, 0, 0, 0, 0);
  if (! filter2->match("/", *node)) {
    cout << "Not matching +3" << endl;
  }
  delete node;
  delete filter2;

  node = new Node("to a file.txt", 'f', 0, 0, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching 1" << endl;
  }
  delete node;
  node = new Node("to a file.tst", 'f', 0, 0, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching 2" << endl;
  }
  delete node;
  node = new Node("to a file.tsu", 'f', 0, 0, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching 3" << endl;
  }
  delete node;
  delete filter;

  filter = new Filters;
  node = new Node("", 'f', 0, 0, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 1000, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 1000000, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;

  /* File type is always 'f' */
  filter->push_back(Filter(Condition(condition_size_le, (long long) 500,
    false)));
  cout << ">List " << filter->size() << " rule(s):" << endl;
  filter_show(*filter);

  node = new Node("", 'f', 0, 0, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 1000, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 1000000, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;

  /* File type is always 'f' */
  filter->push_back(Filter(Condition(condition_size_ge, (long long) 5000,
    false)));
  cout << ">List " << filter->size() << " rule(s):" << endl;
  filter_show(*filter);

  node = new Node("", 'f', 0, 0, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 1000, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 1000000, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;

  delete filter;

  /* Test complex rules */
  cout << "\nComplex rules test" << endl;
  filter = new Filters;

  cout << ">List " << filter->size() << " rule(s):" << endl;
  filter_show(*filter);

  filter->push_back(Filter());
  Filter* rule = &filter->back();

  rule->push_back(Condition(condition_size_le, (long long) 500, false));
  cout << ">List " << filter->size() << " rule(s):" << endl;
  filter_show(*filter);

  rule->push_back(Condition(condition_size_ge, (long long) 400, false));
  cout << ">List " << filter->size() << " rule(s):" << endl;
  filter_show(*filter);

  node = new Node("", 'f', 0, 600, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 500, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 450, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 400, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;
  node = new Node("", 'f', 0, 300, 0, 0, 0);
  if (! filter->match("/", *node)) {
    cout << "Not matching " << node->size() << "" << endl;
  } else {
    cout << "Matching " << node->size() << "" << endl;
  }
  delete node;

  delete filter;


  cout << "OR filter test" << endl;
  Filter2* or_filter = new Filter2(filter_or, "my_filter_or");
  or_filter->add(new Condition(condition_size_le, (long long) 500, false));
  or_filter->add(new Condition(condition_path_regex, "^/to a.*\\.txt", false));
  // Show name
  cout << or_filter->name() << endl;
  // Both match
  node = new Node("to a file.txt", 'f', 0, 400, 0, 0, 0);
  if (! or_filter->match("/", *node)) {
    cout << "Not matching 1" << endl;
  }
  delete node;
  // Size match
  node = new Node("to a file.tst", 'f', 0, 400, 0, 0, 0);
  if (! or_filter->match("/", *node)) {
    cout << "Not matching 2" << endl;
  }
  delete node;
  // Name match
  node = new Node("to a file.txt", 'f', 0, 600, 0, 0, 0);
  if (! or_filter->match("/", *node)) {
    cout << "Not matching 3" << endl;
  }
  delete node;
  // None match
  node = new Node("to a file.tst", 'f', 0, 600, 0, 0, 0);
  if (! or_filter->match("/", *node)) {
    cout << "Not matching 4" << endl;
  }
  delete node;
  delete or_filter;
  cout << endl;


  cout << "AND filter test" << endl;
  Filter2* and_filter = new Filter2(filter_and, "my_filter_and");
  and_filter->add(new Condition(condition_size_le, (long long) 500, false));
  and_filter->add(new Condition(condition_path_regex, "^/to a.*\\.txt",
    false));
  // Show name
  cout << and_filter->name() << endl;
  // Both match
  node = new Node("to a file.txt", 'f', 0, 400, 0, 0, 0);
  if (! and_filter->match("/", *node)) {
    cout << "Not matching 1" << endl;
  }
  delete node;
  // Size match
  node = new Node("to a file.tst", 'f', 0, 400, 0, 0, 0);
  if (! and_filter->match("/", *node)) {
    cout << "Not matching 2" << endl;
  }
  delete node;
  // Name match
  node = new Node("to a file.txt", 'f', 0, 600, 0, 0, 0);
  if (! and_filter->match("/", *node)) {
    cout << "Not matching 3" << endl;
  }
  delete node;
  // None match
  node = new Node("to a file.tst", 'f', 0, 600, 0, 0, 0);
  if (! and_filter->match("/", *node)) {
    cout << "Not matching 4" << endl;
  }
  delete node;
  delete and_filter;
  cout << endl;


  cout << "Sub-filter test" << endl;
  Filter2* sub_filter = new Filter2(filter_and, "my_subfilter");
  sub_filter->add(new Condition(condition_size_le, (long long) 500, false));
  sub_filter->add(new Condition(condition_path_regex, "^/to a.*\\.txt",
    false));
  and_filter = new Filter2(filter_and, "my_filter_and");
  and_filter->add(new Condition(condition_type, 'f', false));
  and_filter->add(new Condition(condition_subfilter, sub_filter, true));
  // Show names
  cout << sub_filter->name() << endl;
  cout << and_filter->name() << endl;
  // Both in subfilter match, filter condition matches
  node = new Node("to a file.txt", 'f', 0, 400, 0, 0, 0);
  if (! and_filter->match("/", *node)) {
    cout << "Not matching 1" << endl;
  }
  delete node;
  // Size in subfilter match, filter condition matches
  node = new Node("to a file.tst", 'f', 0, 400, 0, 0, 0);
  if (! and_filter->match("/", *node)) {
    cout << "Not matching 2" << endl;
  }
  delete node;
  // Name in subfilter match, filter condition matches
  node = new Node("to a file.txt", 'f', 0, 600, 0, 0, 0);
  if (! and_filter->match("/", *node)) {
    cout << "Not matching 3" << endl;
  }
  delete node;
  // None in subfilter match, filter condition matches
  node = new Node("to a file.tst", 'f', 0, 600, 0, 0, 0);
  if (! and_filter->match("/", *node)) {
    cout << "Not matching 4" << endl;
  }
  delete node;
  // Both in subfilter match, filter condition does not match
  node = new Node("to a file.txt", 'l', 0, 400, 0, 0, 0);
  if (! and_filter->match("/", *node)) {
    cout << "Not matching 1" << endl;
  }
  delete node;
  // Size in subfilter match, filter condition does not match
  node = new Node("to a file.tst", 'l', 0, 400, 0, 0, 0);
  if (! and_filter->match("/", *node)) {
    cout << "Not matching 2" << endl;
  }
  delete node;
  // Name in subfilter match, filter condition does not match
  node = new Node("to a file.txt", 'l', 0, 600, 0, 0, 0);
  if (! and_filter->match("/", *node)) {
    cout << "Not matching 3" << endl;
  }
  delete node;
  // None in subfilter match, filter condition does not match
  node = new Node("to a file.tst", 'l', 0, 600, 0, 0, 0);
  if (! and_filter->match("/", *node)) {
    cout << "Not matching 4" << endl;
  }
  delete node;
  delete and_filter;

  return 0;
}
