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
#include <list>
#include <string>
#include <errno.h>

using namespace std;

#include "strings.h"
#include "files.h"
#include "list.h"
#include "hbackup.h"

using namespace hbackup;

static int verbose = 4;

int hbackup::verbosity(void) {
  return verbose;
}

int hbackup::terminating(void) {
  return 0;
}

static time_t my_time = 0;
time_t time(time_t *t) {
  return my_time;
}

static void showLine(time_t timestamp, char* prefix, char* path, Node* node) {
  printf("[%2ld] %-16s %-34s", timestamp, prefix, path);
  if (node != NULL) {
    printf(" %c %5llu %03o", node->type(), node->size(), node->mode());
    if (node->type() == 'f') {
      printf(" %s", ((File*) node)->checksum());
    }
    if (node->type() == 'l') {
      printf(" %s", ((Link*) node)->link());
    }
  } else {
    printf(" [rm]");
  }
  cout << endl;
}

int main(void) {
  List    dblist("test_db/list");
  List    journal("test_db/journal");
  List    merge("test_db/merge");
  char*   prefix = NULL;
  char*   path   = NULL;
  Node*   node   = NULL;
  time_t  ts;
  int     rc     = 0;

  cout << "Test: DB lists" << endl;
  mkdir("test_db", 0755);

  if (dblist.open("w")) {
    cerr << "Failed to open list" << endl;
    return 0;
  }
  dblist.close();


  cout << endl << "Test: journal write" << endl;
  my_time++;

  if (journal.open("w", -1)) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  journal.removed("prefix", "file_gone");
  node = new Stream("test1/test space");
  // No checksum
  journal.added("prefix2", "file sp", node);
  free(node);
  node = new Stream("test1/testfile");
  ((Stream*) node)->computeChecksum();
  journal.added(NULL, "file_new", node);
  free(node);
  node = new Link("test1/testlink");
  journal.added("prefix3", "link", node);
  free(node);
  node = new Directory("test1/testdir");
  journal.added("prefix5", "path", node);
  free(node);
  journal.close();


  cout << endl << "Test: journal read" << endl;
  my_time++;

  node = NULL;
  journal.open("r");
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  } else
  while ((rc = journal.getEntry(&ts, &prefix, &path, &node)) > 0) {
    showLine(ts, prefix, path, node);
  }
  journal.close();
  if (rc < 0) {
    cerr << "Failed to read journal" << endl;
  } else
  // Check that we obtain the end of file again
  if (rc == 0) {
    if (journal.getEntry(&ts, &prefix, &path, &node) != 0) {
      cerr << "Error reading end of journal again" << endl;
    }
  }


  cout << endl << "Test: journal merge into empty list" << endl;
  my_time++;

  if (dblist.open("r")) {
    cerr << "Failed to open list" << endl;
    return 0;
  }
  if (dblist.isEmpty()) {
    cout << "List is empty" << endl;
  }
  if (journal.open("r")) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  }
  if (merge.open("w")) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (merge.merge(dblist, journal)) {
    cerr << "Failed to merge: " << strerror(errno) << endl;
    return 0;
  }
  merge.close();
  journal.close();
  dblist.close();


  cout << endl << "Test: merge read" << endl;
  my_time++;

  merge.open("r");
  if (merge.isEmpty()) {
    cout << "Merge is empty" << endl;
  } else
  while ((rc = merge.getEntry(&ts, &prefix, &path, &node)) > 0) {
    showLine(ts, prefix, path, node);
  }
  merge.close();
  if (rc < 0) {
    cerr << "Failed to read merge" << endl;
  }

  if (rename("test_db/merge", "test_db/list")) {
    cerr << "Failed to rename merge into list" << endl;
  }


  cout << endl << "Test: journal write again" << endl;
  my_time++;

  if (journal.open("w")) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  system("echo \"this is my new test\" > test1/testfile");
  node = new Stream("test1/testfile");
  ((Stream*) node)->computeChecksum();
  journal.added("prefix", "file_new", node);
  free(node);
  node = new Stream("test1/test space");
  ((Stream*) node)->computeChecksum();
  journal.added("prefix2", "file sp", node, 0);
  free(node);
  node = new Stream("test1/testfile");
  ((Stream*) node)->computeChecksum();
  journal.added(NULL, "file_new", node);
  free(node);
  node = new Stream("test1/test space");
  ((Stream*) node)->computeChecksum();
  journal.added("prefix4", "file_new", node);
  free(node);
  journal.close();


  cout << endl << "Test: journal read" << endl;
  my_time++;

  node = NULL;
  journal.open("r");
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  } else
  while ((rc = journal.getEntry(&ts, &prefix, &path, &node)) > 0) {
    showLine(ts, prefix, path, node);
  }
  journal.close();
  if (rc < 0) {
    cerr << "Failed to read journal" << endl;
  }


  cout << endl << "Test: journal merge into list" << endl;
  my_time++;

  if (dblist.open("r")) {
    cerr << "Failed to open list" << endl;
    return 0;
  }
  if (dblist.isEmpty()) {
    cout << "List is empty" << endl;
  }
  if (journal.open("r")) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  }
  if (merge.open("w")) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (merge.merge(dblist, journal)) {
    cerr << "Failed to merge: " << strerror(errno) << endl;
    return 0;
  }
  merge.close();
  journal.close();
  dblist.close();


  cout << endl << "Test: merge read" << endl;
  my_time++;

  merge.open("r");
  if (merge.isEmpty()) {
    cout << "Merge is empty" << endl;
  } else
  while ((rc = merge.getEntry(&ts, &prefix, &path, &node)) > 0) {
    showLine(ts, prefix, path, node);
  }
  merge.close();
  if (rc < 0) {
    cerr << "Failed to read merge" << endl;
  }

  if (rename("test_db/merge", "test_db/list")) {
    cerr << "Failed to rename merge into list" << endl;
  }


  cout << endl << "Test: get all prefixes" << endl;
  if (dblist.open("r")) {
    cerr << "Failed to open list" << endl;
    return 0;
  }
  while (dblist.findPrefix(NULL)) {
    char *prefix = NULL;
    dblist.getEntry(NULL, &prefix, NULL, NULL);
    cout << prefix << endl;
    free(prefix);
  }
  dblist.close();


  cout << endl << "Test: journal prefix out of order" << endl;
  my_time++;

  if (journal.open("w")) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  system("echo \"this is my new test\" > test1/testfile");
  node = new Stream("test1/testfile");
  ((Stream*) node)->computeChecksum();
  journal.added("prefix4", "file_new", node);
  journal.added("prefix2", "file_new", node);
  free(node);
  journal.close();


  cout << endl << "Test: journal read" << endl;
  my_time++;

  node = NULL;
  journal.open("r");
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  } else
  while ((rc = journal.getEntry(&ts, &prefix, &path, &node)) > 0) {
    showLine(ts, prefix, path, node);
  }
  journal.close();
  if (rc < 0) {
    cerr << "Failed to read journal" << endl;
  }


  cout << endl << "Test: journal merge into list" << endl;
  my_time++;

  if (dblist.open("r")) {
    cerr << "Failed to open list" << endl;
    return 0;
  }
  if (dblist.isEmpty()) {
    cout << "List is empty" << endl;
  }
  if (journal.open("r")) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  }
  if (merge.open("w")) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (merge.merge(dblist, journal)) {
    cerr << "Failed to merge: " << strerror(errno) << endl;
//     return 0;
  }
  merge.close();
  journal.close();
  dblist.close();


  cout << endl << "Test: merge read" << endl;
  my_time++;

  merge.open("r");
  if (merge.isEmpty()) {
    cout << "Merge is empty" << endl;
  } else
  while ((rc = merge.getEntry(&ts, &prefix, &path, &node)) > 0) {
    showLine(ts, prefix, path, node);
  }
  merge.close();
  if (rc < 0) {
    cerr << "Failed to read merge" << endl;
  }


  cout << endl << "Test: journal path out of order" << endl;
  my_time++;

  if (journal.open("w")) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  system("echo \"this is my new test\" > test1/testfile");
  node = new Stream("test1/testfile");
  ((Stream*) node)->computeChecksum();
  journal.added("prefix2", "file_new", node);
  journal.added(NULL, "file_gone", node);
  free(node);
  journal.close();


  cout << endl << "Test: journal read" << endl;
  my_time++;

  node = NULL;
  journal.open("r");
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  } else
  while ((rc = journal.getEntry(&ts, &prefix, &path, &node)) > 0) {
    showLine(ts, prefix, path, node);
  }
  journal.close();
  if (rc < 0) {
    cerr << "Failed to read journal" << endl;
  }


  cout << endl << "Test: journal merge into list" << endl;
  my_time++;

  if (dblist.open("r")) {
    cerr << "Failed to open list" << endl;
    return 0;
  }
  if (dblist.isEmpty()) {
    cout << "List is empty" << endl;
  }
  if (journal.open("r")) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  }
  if (merge.open("w")) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (merge.merge(dblist, journal)) {
    cerr << "Failed to merge: " << strerror(errno) << endl;
//     return 0;
  }
  merge.close();
  journal.close();
  dblist.close();


  cout << endl << "Test: merge read" << endl;
  my_time++;

  merge.open("r");
  if (merge.isEmpty()) {
    cout << "Merge is empty" << endl;
  } else
  while ((rc = merge.getEntry(&ts, &prefix, &path, &node)) > 0) {
    showLine(ts, prefix, path, node);
  }
  merge.close();
  if (rc < 0) {
    cerr << "Failed to read merge" << endl;
  }


  cout << endl << "Test: prefix find" << endl;
  my_time++;

  merge.open("r");
  if (merge.isEmpty()) {
    cout << "Merge is empty" << endl;
  }
  if (! merge.findPrefix("path")) {
    cout << "prefix 'path' not found" << endl;
  } else {
    cout << "prefix 'path' found" << endl;
  }
  while ((rc = merge.getEntry(&ts, &prefix, &path, &node)) > 0) {
    showLine(ts, prefix, path, node);
  }
  merge.close();
  if (rc < 0) {
    cerr << "Failed to read merge" << endl;
  }

  merge.open("r");
  if (merge.isEmpty()) {
    cout << "Merge is empty" << endl;
  }
  if (! merge.findPrefix("silly")) {
    cout << "prefix 'silly' not found" << endl;
  } else {
    cout << "prefix 'silly' found" << endl;
  }
  while ((rc = merge.getEntry(&ts, &prefix, &path, &node)) > 0) {
    showLine(ts, prefix, path, node);
  }
  merge.close();
  if (rc < 0) {
    cerr << "Failed to read merge" << endl;
  }

  merge.open("r");
  if (merge.isEmpty()) {
    cout << "Merge is empty" << endl;
  }
  if (! merge.findPrefix("prefix2")) {
    cout << "prefix 'prefix2' not found" << endl;
  } else {
    cout << "prefix 'prefix2' found" << endl;
  }
  while ((rc = merge.getEntry(&ts, &prefix, &path, &node)) > 0) {
    showLine(ts, prefix, path, node);
  }
  merge.close();
  if (rc < 0) {
    cerr << "Failed to read merge" << endl;
  }


  cout << endl << "Test: expire" << endl;

  if (dblist.open("r")) {
    cerr << "Failed to open list" << endl;
    return 0;
  }
  if (dblist.isEmpty()) {
    cout << "List is empty" << endl;
  }
  if (merge.open("w")) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  StrPath null;
  list<string> active;
  list<string> expired;
  if (dblist.search(&null, &null, &merge, 14, &active, &expired) < 0) {
    cerr << "Failed to copy: " << strerror(errno) << endl;
    return 0;
  }
  merge.close();
  dblist.close();

  merge.open("r");
  if (merge.isEmpty()) {
    cout << "Merge is empty" << endl;
  }
  while ((rc = merge.getEntry(&ts, &prefix, &path, &node)) > 0) {
    showLine(ts, prefix, path, node);
  }
  merge.close();
  if (rc < 0) {
    cerr << "Failed to read merge" << endl;
  }

  cout << "Active checksums (gross): " << active.size() << endl;
  for (list<string>::iterator i = active.begin(); i != active.end();  i++) {
    cout << " " << *i << endl;
  }
  active.sort();
  active.unique();
  cout << "Active checksums (sorted, uniqued): " << active.size() << endl;
  for (list<string>::iterator i = active.begin(); i != active.end();  i++) {
    cout << " " << *i << endl;
  }
  cout << "Expired checksums (gross): " << expired.size() << endl;
  for (list<string>::iterator i = expired.begin(); i != expired.end();  i++) {
    cout << " " << *i << endl;
  }
  expired.sort();
  expired.unique();
  cout << "Expired checksums (sorted, uniqued): " << expired.size() << endl;
  for (list<string>::iterator i = expired.begin(); i != expired.end();  i++) {
    cout << " " << *i << endl;
  }
  // Get checksums for removal
  list<string>::iterator i = expired.begin();
  list<string>::iterator j = active.begin();
  while (i != expired.end()) {
    while ((j != active.end()) && (*j < *i)) { j++; }
    if ((j != active.end()) && (*j == *i)) {
      i = expired.erase(i);
    } else {
      i++;
    }
  }
  cout << "Expired checksums: " << expired.size() << endl;
  for (list<string>::iterator i = expired.begin(); i != expired.end();  i++) {
    cout << " " << *i << endl;
  }


  cout << endl << "Test: copy prefix data" << endl;

  if (dblist.open("r")) {
    cerr << "Failed to open list" << endl;
    return 0;
  }
  if (dblist.isEmpty()) {
    cout << "List is empty" << endl;
  } else {
    cout << "List:" << endl;
  }
  while ((rc = dblist.getEntry(&ts, &prefix, &path, &node)) > 0) {
    showLine(ts, prefix, path, node);
  }
  dblist.close();
  if (rc < 0) {
    cerr << "Failed to read list" << endl;
  }
  if (dblist.open("r")) {
    cerr << "Failed to open list" << endl;
    return 0;
  }
  if (dblist.isEmpty()) {
    cout << "List is empty" << endl;
  }
  if (merge.open("w")) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  StrPath prefix3("prefix3\n");
  if (dblist.search(&prefix3, &null, &merge) < 0) {
    cerr << "Failed to copy: " << strerror(errno) << endl;
    return 0;
  }
  merge.close();
  dblist.close();

  merge.open("r");
  if (merge.isEmpty()) {
    cout << "Merge is empty" << endl;
  } else {
    cout << "Merge:" << endl;
  }
  while ((rc = merge.getEntry(&ts, &prefix, &path, &node)) > 0) {
    showLine(ts, prefix, path, node);
  }
  merge.close();
  if (rc < 0) {
    cerr << "Failed to read merge" << endl;
  }
  cout << "Merge tail:" << endl;
  system("tail -2 test_db/merge | grep -v 3");
  rename("test_db/merge", "test_db/list");


  cout << endl << "Test: get latest entry only" << endl;

  // Add new entry in journal
  if (journal.open("w")) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  system("echo \"this is my other test\" > test1/testfile");
  node = new Stream("test1/testfile");
  ((Stream*) node)->computeChecksum();
  journal.added("prefix", "file_new", node);
  free(node);
  node = NULL;
  journal.close();
  // Merge
  if (dblist.open("r")) {
    cerr << "Failed to open list" << endl;
    return 0;
  }
  if (dblist.isEmpty()) {
    cout << "List is empty" << endl;
  }
  if (journal.open("r")) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  }
  if (merge.open("w")) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (merge.merge(dblist, journal)) {
    cerr << "Failed to merge: " << strerror(errno) << endl;
    return 0;
  }
  merge.close();
  journal.close();
  dblist.close();
  // Update
  if (rename("test_db/merge", "test_db/list")) {
    cerr << "Failed to rename merge into list" << endl;
  }
  // Show list
  if (dblist.open("r")) {
    cerr << "Failed to open list" << endl;
    return 0;
  }
  if (dblist.isEmpty()) {
    cout << "List is empty" << endl;
  } else {
    cout << "List:" << endl;
  }
  while ((rc = dblist.getEntry(&ts, &prefix, &path, &node)) > 0) {
    showLine(ts, prefix, path, node);
  }
  dblist.close();
  if (rc < 0) {
    cerr << "Failed to read list" << endl;
  }


  // Show list with new functionality
  cout << endl << "Test: last entries data" << endl;
  if (dblist.open("r")) {
    cerr << "Failed to open list" << endl;
    return 0;
  }
  if (dblist.isEmpty()) {
    cout << "List is empty" << endl;
  } else {
    cout << "List:" << endl;
  }
  while (dblist.getEntry(&ts, &prefix, &path, &node, 0) > 0) {
    showLine(ts, prefix, path, node);
  }
  dblist.close();
  if (rc < 0) {
    cerr << "Failed to read list" << endl;
  }


  // Show list with new functionality
  cout << endl << "Test: entries for given date" << endl;
  if (dblist.open("r")) {
    cerr << "Failed to open list" << endl;
    return 0;
  }
  if (dblist.isEmpty()) {
    cout << "List is empty" << endl;
  } else {
    cout << "List:" << endl;
  }
  while (dblist.getEntry(&ts, &prefix, &path, &node, 5) > 0) {
    showLine(ts, prefix, path, node);
  }
  dblist.close();
  if (rc < 0) {
    cerr << "Failed to read list" << endl;
  }

  return 0;
}
