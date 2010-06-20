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

#include <iostream>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

using namespace std;

#include "test.h"

#include "files.h"
#include "list.h"

static time_t my_time = 0;
time_t time(time_t *t) {
  (void) t;
  return my_time;
}

static void showLine(time_t timestamp, const char* path, const Node* node) {
  printf("[%2ld] %-30s", timestamp, path);
  if (node != NULL) {
    printf(" %c %8lld %03o", node->type(), node->size(), node->mode());
    if (node->type() == 'f') {
      printf(" %s", static_cast<const File*>(node)->checksum());
    }
    if (node->type() == 'l') {
      printf(" %s", static_cast<const Link*>(node)->link());
    }
  } else {
    printf(" [rm]");
  }
  cout << endl;
}

static void showList(List& list) {
  if (list.open() < 0) {
    cerr << "Failed to open " << list.path().basename() << ": "
      << strerror(errno) << endl;
  } else {
    if (list.end()) {
      cout << list.path().basename() << " is empty" << endl;
    } else {
      List::Status rc;
      Path         path;
      while ((rc = list.fetchLine()) > 0) {
        if (rc == List::got_path) {
          path = list.getPath();
          list.resetStatus();
        } else {
          time_t ts;
          Node*  node;
          List::decodeLine(list.getData(), &ts, path, &node);
          showLine(ts, path, node);
          list.resetStatus();
        }
      }
      if (rc == List::failed) {
        cerr << "Failed to read " << list.path().basename() << endl;
      } else
      if (rc == List::eof) {
        cerr << "Unexpected end of " << list.path().basename() << endl;
      }
    }
    list.close();
  }
}

static void add(
    List&     list,
    const char*     path,
    time_t          epoch,
    const Node*     node = NULL) {
  list.putLine(path);
  char* line = NULL;
  List::encodeLine(&line, epoch, node);
  list.putLine(line);
  free(line);
}

int main(void) {
  List      dblist("test_db/list");
  List      journal("test_db/journal");
  List      merge("test_db/merge");
  List      dblist_reader("test_db/list");
  List      journal_reader("test_db/journal");
  List      merge_reader("test_db/merge");
  Node*     node   = NULL;
  int       sys_rc;

  cout << "Test: DB lists" << endl;
  mkdir("test_db", 0755);
  report.setLevel(debug);


  cout << endl << "Test: list creation" << endl;

  if (dblist.create()) {
    cerr << strerror(errno) << " opening list!" << endl;
    return 0;
  }
  dblist.close();

  dblist_reader.show();


  cout << endl << "Test: journal write" << endl;
  my_time++;

  if (journal.create()) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }

  // No checksum
  node = new Stream("test1/test space");
  add(journal, "file sp", time(NULL), node);
  free(node);

  add(journal, "file_gone", time(NULL));

  node = new Stream("test1/testfile");
  static_cast<Stream*>(node)->open(O_RDONLY);
  static_cast<Stream*>(node)->computeChecksum();
  static_cast<Stream*>(node)->close();
  add(journal, "file_new", time(NULL), node);
  free(node);

  node = new Link("test1/testlink");
  add(journal, "link", time(NULL), node);
  free(node);

  node = new Directory("test1/testdir");
  node->setSize(0);
  add(journal, "path", time(NULL), node);
  free(node);

  journal.close();
  node = NULL;


  cout << endl << "Test: journal read" << endl;
  my_time++;

  showList(journal_reader);


  cout << endl << "Test: journal merge into empty list" << endl;
  my_time++;

  if (dblist_reader.open()) {
    cerr << strerror(errno) << " opening list!" << endl;
    return 0;
  }
  if (dblist_reader.end()) {
    cout << "Register is empty" << endl;
  }
  if (journal_reader.open()) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  if (journal_reader.end()) {
    cout << "Journal is empty" << endl;
  }
  if (merge.create()) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (List::merge(&dblist_reader, &merge, &journal_reader) < 0) {
    cerr << "Failed to merge" << endl;
    return 0;
  }
  merge.close();
  journal_reader.close();
  dblist_reader.close();


  cout << endl << "Test: merge read" << endl;
  my_time++;

  showList(merge_reader);

  if (rename("test_db/merge", "test_db/list")) {
    cerr << "Failed to rename merge into list" << endl;
  }


  cout << endl << "Test: journal write again" << endl;
  my_time++;

  if (journal.create()) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  sys_rc = system("echo \"this is my new test\" > test1/testfile");

  node = new Link("test1/testlink");
  add(journal, "CR/x", time(NULL), node);
  free(node);

  node = new Link("test1/testlink");
  add(journal, "CR\r", time(NULL), node);
  free(node);

  node = new Stream("test1/test space");
  static_cast<Stream*>(node)->open(O_RDONLY);
  static_cast<Stream*>(node)->computeChecksum();
  static_cast<Stream*>(node)->close();
  add(journal, "file sp", time(NULL), node);
  free(node);

  node = new Stream("test1/testfile");
  static_cast<Stream*>(node)->open(O_RDONLY);
  static_cast<Stream*>(node)->computeChecksum();
  static_cast<Stream*>(node)->close();
  add(journal, "file_new", time(NULL), node);
  free(node);

  journal.close();
  node = NULL;


  cout << endl << "Test: journal read" << endl;
  my_time++;

  showList(journal_reader);


  cout << endl << "Test: journal merge into list" << endl;
  my_time++;

  if (dblist_reader.open()) {
    cerr << strerror(errno) << " opening list!" << endl;
    return 0;
  }
  if (dblist_reader.end()) {
    cout << "Register is empty" << endl;
  }
  if (journal_reader.open()) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  if (merge.create()) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (List::merge(&dblist_reader, &merge, &journal_reader) < 0) {
    cerr << "Failed to merge" << endl;
    return 0;
  }
  merge.close();
  journal_reader.close();
  dblist_reader.close();


  cout << endl << "Test: merge read" << endl;
  my_time++;

  showList(merge_reader);

  if (rename("test_db/merge", "test_db/list")) {
    cerr << "Failed to rename merge into list" << endl;
  }


  cout << endl << "Test: get all paths" << endl;
  my_time++;
  if (dblist_reader.open()) {
    cerr << strerror(errno) << " opening list!" << endl;
    return 0;
  }
  while (List::search(&dblist_reader) == 2) {
    char *path = NULL;
    List::getEntry(&dblist_reader, NULL, &path, NULL);
    cout << path << endl;
    free(path);
  }
  dblist_reader.close();


  cout << endl << "Test: journal path out of order" << endl;
  my_time++;

  if (journal.create()) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  sys_rc = system("echo \"this is my new test\" > test1/testfile");
  node = new Stream("test1/testfile");
  static_cast<Stream*>(node)->open(O_RDONLY);
  static_cast<Stream*>(node)->computeChecksum();
  static_cast<Stream*>(node)->close();
  add(journal, "file_new", time(NULL), node);
  add(journal, "file_gone", time(NULL), node);
  free(node);
  journal.close();
  node = NULL;


  cout << endl << "Test: journal read" << endl;
  my_time++;

  showList(journal_reader);


  cout << endl << "Test: journal merge into list" << endl;
  my_time++;

  if (dblist_reader.open()) {
    cerr << strerror(errno) << " opening list!" << endl;
    return 0;
  }
  if (dblist_reader.end()) {
    cout << "Register is empty" << endl;
  }
  if (journal_reader.open()) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  if (journal_reader.end()) {
    cout << "Journal is empty" << endl;
  }
  if (merge.create()) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (List::merge(&dblist_reader, &merge, &journal_reader) < 0) {
    cerr << "Failed to merge" << endl;
//     return 0;
  }
  merge.close();
  journal_reader.close();
  dblist_reader.close();


  cout << endl << "Test: merge read" << endl;
  my_time++;

  showList(merge_reader);


  cout << endl << "Test: expire" << endl;

  // Show current list
  cout << "Register:" << endl;
  dblist_reader.show();

  list<string>::iterator i;
  list<string>::iterator j;

  cout << "Date: 4" << endl;
  if (dblist_reader.open()) {
    cerr << strerror(errno) << " opening list!" << endl;
    return 0;
  }
  if (dblist_reader.end()) {
    cout << "Register is empty" << endl;
  }
  if (merge.create()) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (List::search(&dblist_reader, "", 4, 0, &merge) < 0) {
    cerr << "Failed to copy: " << strerror(errno) << endl;
    return 0;
  }
  merge.close();
  dblist_reader.close();

  showList(merge_reader);

  // All but last
  cout << "Date: 0" << endl;
  if (dblist_reader.open()) {
    cerr << strerror(errno) << " opening list!" << endl;
    return 0;
  }
  if (dblist_reader.end()) {
    cout << "Register is empty" << endl;
  }
  if (merge.create()) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (List::search(&dblist_reader, "", 0, 0, &merge) < 0) {
    cerr << "Failed to copy: " << strerror(errno) << endl;
    return 0;
  }
  merge.close();
  dblist_reader.close();

  showList(merge_reader);


  cout << endl << "Test: copy all data" << endl;

  cout << "Register:" << endl;
  dblist_reader.show();
  if (dblist_reader.open()) {
    cerr << strerror(errno) << " opening list!" << endl;
    return 0;
  }
  if (dblist_reader.end()) {
    cout << "Register is empty" << endl;
  }
  if (merge.create()) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (List::search(&dblist_reader, "", -1, 0, &merge) < 0) {
    cerr << "Failed to copy: " << strerror(errno) << endl;
    return 0;
  }
  merge.close();
  dblist_reader.close();

  cout << "Merge:" << endl;
  showList(merge_reader);
  cout << "Merge tail:" << endl;
  sys_rc = system("tail -2 test_db/merge | grep -v 1");
  rename("test_db/merge", "test_db/list");


  cout << endl << "Test: get latest entry only" << endl;

  // Add new entry in journal
  if (journal.create()) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  sys_rc = system("echo \"this is my other test\" > test1/testfile");
  node = new Stream("test1/testfile");
  static_cast<Stream*>(node)->open(O_RDONLY);
  static_cast<Stream*>(node)->computeChecksum();
  static_cast<Stream*>(node)->close();
  add(journal, "file_new", time(NULL), node);
  free(node);
  node = NULL;
  journal.close();
  // Merge
  if (dblist_reader.open()) {
    cerr << strerror(errno) << " opening list!" << endl;
    return 0;
  }
  if (dblist_reader.end()) {
    cout << "Register is empty" << endl;
  }
  if (journal_reader.open()) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  if (journal_reader.end()) {
    cout << "Journal is empty" << endl;
  }
  if (merge.create()) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (List::merge(&dblist_reader, &merge, &journal_reader) < 0) {
    cerr << "Failed to merge" << endl;
    return 0;
  }
  merge.close();
  journal_reader.close();
  dblist_reader.close();
  // Update
  if (rename("test_db/merge", "test_db/list")) {
    cerr << "Failed to rename merge into list" << endl;
  }
  // Show list
  cout << "Register:" << endl;
  dblist_reader.show();


  // Show list with new functionality
  cout << endl << "Test: last entries data" << endl;
  cout << "Register:" << endl;
  dblist_reader.show(0);


  // Show list with new functionality
  cout << endl << "Test: entries for given date" << endl;
  cout << "Date: 5" << endl;
  cout << "Register:" << endl;
  dblist_reader.show(5);


  // Recovery
  cout << endl << "Test: journal write" << endl;
  my_time++;

  if (journal.create()) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }

  // No checksum
  node = new Stream("test1/test space");
  add(journal, "file sp", time(NULL), node);
  free(node);

  add(journal, "file_gone", time(NULL));

  node = new Stream("test1/testfile");
  static_cast<Stream*>(node)->open(O_RDONLY);
  static_cast<Stream*>(node)->computeChecksum();
  static_cast<Stream*>(node)->close();
  add(journal, "file_new", time(NULL), node);
  free(node);

  node = new Link("test1/testlink");
  add(journal, "link2", time(NULL), node);
  free(node);

  node = new Directory("test1/testdir");
  add(journal, "path", time(NULL), node);
  free(node);

  journal.close();
  sys_rc = system("head -c 190 test_db/journal > test_db/journal.1");
  sys_rc = system("cp -f test_db/journal.1 test_db/journal");


  cout << endl << "Test: journal read" << endl;
  my_time++;

  showList(journal_reader);


  cout << endl << "Test: broken journal merge" << endl;
  my_time++;

  if (dblist_reader.open()) {
    cerr << strerror(errno) << " opening list!" << endl;
    return 0;
  }
  if (dblist_reader.end()) {
    cout << "Register is empty" << endl;
  }
  if (journal_reader.open()) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  if (journal_reader.end()) {
    cout << "Journal is empty" << endl;
  }
  if (merge.create()) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (List::merge(&dblist_reader, &merge, &journal_reader) < 0) {
    cerr << "Failed to merge" << endl;
    return 0;
  }
  merge.close();
  journal_reader.close();
  dblist_reader.close();


  cout << endl << "Test: merge read" << endl;
  my_time++;

  showList(merge_reader);

  if (rename("test_db/merge", "test_db/list")) {
    cerr << "Failed to rename merge into list" << endl;
  }

  return 0;
}
