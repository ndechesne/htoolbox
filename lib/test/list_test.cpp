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

#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "report.h"
#include "list.h"

using namespace hbackup;

static time_t my_time = 0;
time_t time(time_t *t) {
  return my_time;
}

static void showLine(time_t timestamp, char* path, Node* node) {
  printf("[%2ld] %-16s", timestamp, path);
  if (node != NULL) {
    printf(" %c %5llu %03o", node->type(),
    (node->type() != 'd') ? node->size() : 0, node->mode());
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
  char*   path   = NULL;
  Node*   node   = NULL;
  time_t  ts;
  int     rc     = 0;

  cout << "Test: DB lists" << endl;
  mkdir("test_db", 0755);
  setVerbosityLevel(debug);

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

  // No checksum
  node = new Stream("test1/test space");
  journal.addPath("file sp");
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  }
  journal.addData(time(NULL), node);
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  }
  free(node);

  journal.addPath("file_gone");
  journal.addData(time(NULL));

  node = new Stream("test1/testfile");
  ((Stream*) node)->open("r");
  ((Stream*) node)->computeChecksum();
  ((Stream*) node)->close();
  journal.addPath("file_new");
  journal.addData(time(NULL), node);
  free(node);

  node = new Link("test1/testlink");
  journal.addPath("link");
  journal.addData(time(NULL), node);
  free(node);

  node = new Directory("test1/testdir");
  journal.addPath("path");
  journal.addData(time(NULL), node);
  free(node);

  journal.close();
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  }


  cout << endl << "Test: journal read" << endl;
  my_time++;

  node = NULL;
  if (journal.open("r") < 0) {
    cerr << "Failed to open journal: " << strerror(errno) << endl;
  } else {
    if (journal.isEmpty()) {
      cout << "Journal is empty" << endl;
    } else
    while ((rc = journal.getEntry(&ts, &path, &node)) > 0) {
      showLine(ts, path, node);
    }
    journal.close();
    if (rc < 0) {
      cerr << "Failed to read journal" << endl;
    } else
    // Check that we obtain the end of file again
    if (rc == 0) {
      if (journal.getEntry(&ts, &path, &node) != 0) {
        cerr << "Error reading end of journal again" << endl;
      }
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
  if (merge.merge(dblist, journal) < 0) {
    cerr << "Failed to merge" << endl;
    return 0;
  }
  if (merge.isEmpty()) {
    cout << "Merge is empty" << endl;
  }
  merge.close();
  journal.close();
  dblist.close();


  cout << endl << "Test: merge read" << endl;
  my_time++;

  merge.show();

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

  node = new Stream("test1/test space");
  ((Stream*) node)->open("r");
  ((Stream*) node)->computeChecksum();
  ((Stream*) node)->close();
  journal.addPath("file sp");
  journal.addData(0, node);
  free(node);

  node = new Stream("test1/testfile");
  ((Stream*) node)->open("r");
  ((Stream*) node)->computeChecksum();
  ((Stream*) node)->close();
  journal.addPath("file_new");
  journal.addData(time(NULL), node);
  free(node);

  journal.close();


  cout << endl << "Test: journal read" << endl;
  my_time++;

  node = NULL;
  journal.show();


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
  if (merge.merge(dblist, journal) < 0) {
    cerr << "Failed to merge" << endl;
    return 0;
  }
  merge.close();
  journal.close();
  dblist.close();


  cout << endl << "Test: merge read" << endl;
  my_time++;

  merge.show();

  if (rename("test_db/merge", "test_db/list")) {
    cerr << "Failed to rename merge into list" << endl;
  }


  cout << endl << "Test: get all paths" << endl;
  my_time++;
  if (dblist.open("r")) {
    cerr << "Failed to open list" << endl;
    return 0;
  }
  while (dblist.search() == 2) {
    char *path   = NULL;
    dblist.getEntry(NULL, &path, NULL, -2);
    cout << path << endl;
    // Should give the same result twice, then get type
    dblist.getEntry(NULL, &path, NULL, -2);
    cout << path << ": " << dblist.getLineType() << endl;
    free(path);
  }
  dblist.close();


  cout << endl << "Test: journal path out of order" << endl;
  my_time++;

  if (journal.open("w")) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  system("echo \"this is my new test\" > test1/testfile");
  node = new Stream("test1/testfile");
  ((Stream*) node)->open("r");
  ((Stream*) node)->computeChecksum();
  ((Stream*) node)->close();
  journal.addPath("file_new");
  journal.addData(time(NULL), node);
  journal.addPath("file_gone");
  journal.addData(time(NULL), node);
  free(node);
  journal.close();


  cout << endl << "Test: journal read" << endl;
  my_time++;

  node = NULL;
  journal.show();


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
  if (merge.merge(dblist, journal) < 0) {
    cerr << "Failed to merge" << endl;
//     return 0;
  }
  merge.close();
  journal.close();
  dblist.close();


  cout << endl << "Test: merge read" << endl;
  my_time++;

  merge.show();


  cout << endl << "Test: expire" << endl;

  // Show current list
  cout << "List:" << endl;
  dblist.show();

  list<string>::iterator i;
  list<string>::iterator j;

  cout << "Date: 4" << endl;
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
  if (dblist.search("", &merge, 4) < 0) {
    cerr << "Failed to copy: " << strerror(errno) << endl;
    return 0;
  }
  merge.close();
  dblist.close();

  merge.show();

  // All but last
  cout << "Date: 0" << endl;
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
  if (dblist.search("", &merge, 0) < 0) {
    cerr << "Failed to copy: " << strerror(errno) << endl;
    return 0;
  }
  merge.close();
  dblist.close();

  merge.show();


  cout << endl << "Test: copy all data" << endl;

  cout << "List:" << endl;
  dblist.show();
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
  if (dblist.search("", &merge) < 0) {
    cerr << "Failed to copy: " << strerror(errno) << endl;
    return 0;
  }
  merge.close();
  dblist.close();

  cout << "Merge:" << endl;
  merge.show();
  cout << "Merge tail:" << endl;
  system("tail -2 test_db/merge | grep -v 1");
  rename("test_db/merge", "test_db/list");


  cout << endl << "Test: get latest entry only" << endl;

  // Add new entry in journal
  if (journal.open("w")) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  system("echo \"this is my other test\" > test1/testfile");
  node = new Stream("test1/testfile");
  ((Stream*) node)->open("r");
  ((Stream*) node)->computeChecksum();
  ((Stream*) node)->close();
  journal.addPath("file_new");
  journal.addData(time(NULL), node);
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
  if (merge.merge(dblist, journal) < 0) {
    cerr << "Failed to merge" << endl;
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
  cout << "List:" << endl;
  dblist.show();


  // Show list with new functionality
  cout << endl << "Test: last entries data" << endl;
  cout << "List:" << endl;
  dblist.show(0);


  // Show list with new functionality
  cout << endl << "Test: entries for given date" << endl;
  cout << "Date: 5" << endl;
  cout << "List:" << endl;
  dblist.show(5);


  // Recovery
  cout << endl << "Test: journal write" << endl;
  my_time++;

  if (journal.open("w", -1)) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }

  // No checksum
  node = new Stream("test1/test space");
  journal.addPath("file sp");
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  }
  journal.addData(time(NULL), node);
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  }
  free(node);

  journal.addPath("file_gone");
  journal.addData(time(NULL));

  node = new Stream("test1/testfile");
  ((Stream*) node)->open("r");
  ((Stream*) node)->computeChecksum();
  ((Stream*) node)->close();
  journal.addPath("file_new");
  journal.addData(time(NULL), node);
  free(node);

  node = new Link("test1/testlink");
  journal.addPath("link2");
  journal.addData(time(NULL), node);
  free(node);

  node = new Directory("test1/testdir");
  journal.addPath("path");
  journal.addData(time(NULL), node);
  free(node);

  journal.close();
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  }
  system("head -c 190 test_db/journal > test_db/journal.1");
  system("cp -f test_db/journal.1 test_db/journal");


  cout << endl << "Test: journal read" << endl;
  my_time++;

  node = NULL;
  if (journal.open("r") < 0) {
    cerr << "Failed to open journal: " << strerror(errno) << endl;
  } else {
    if (journal.isEmpty()) {
      cout << "Journal is empty" << endl;
    } else
    while ((rc = journal.getEntry(&ts, &path, &node)) > 0) {
      showLine(ts, path, node);
    }
    journal.close();
    if (rc < 0) {
      cerr << "Failed to read journal" << endl;
    } else
    // Check that we obtain the end of file again
    if (rc == 0) {
      if (journal.getEntry(&ts, &path, &node) != 0) {
        cerr << "Error reading end of journal again" << endl;
      }
    }
  }


  cout << endl << "Test: broken journal merge" << endl;
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
  if (merge.merge(dblist, journal) < 0) {
    cerr << "Failed to merge" << endl;
    return 0;
  }
  if (merge.isEmpty()) {
    cout << "Merge is empty" << endl;
  }
  merge.close();
  journal.close();
  dblist.close();


  cout << endl << "Test: merge read" << endl;
  my_time++;

  merge.show();

  if (rename("test_db/merge", "test_db/list")) {
    cerr << "Failed to rename merge into list" << endl;
  }

  return 0;
}
