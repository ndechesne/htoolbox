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
  Node*           node;

  cout << "OR filter test" << endl;
  Filter* or_filter = new Filter(Filter::any, "my_filter_or");
  or_filter->add(new Condition(Condition::size_le, (long long) 500, false));
  or_filter->add(new Condition(Condition::path_regex, "^/to a.*\\.txt", false));
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
  Filter* and_filter = new Filter(Filter::all, "my_filter_and");
  and_filter->add(new Condition(Condition::size_le, (long long) 500, false));
  and_filter->add(new Condition(Condition::path_regex, "^/to a.*\\.txt",
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
  Filter* sub_filter = new Filter(Filter::all, "my_subfilter");
  sub_filter->add(new Condition(Condition::size_le, (long long) 500, false));
  sub_filter->add(new Condition(Condition::path_regex, "^/to a.*\\.txt",
    false));
  and_filter = new Filter(Filter::all, "my_filter_and");
  and_filter->add(new Condition(Condition::type, 'f', false));
  and_filter->add(new Condition(Condition::subfilter, sub_filter, true));
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
