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

class ListWriterJournal : public ListWriter {
public:
  ListWriterJournal(const Path& path) : ListWriter(path, true) {}
  void add(
      const char*     path,
      time_t          epoch,
      const Node*     node = NULL) {
    putPath(path);
    if (node != NULL) {
      char line[64];
      size_t sep_offset;
      List::encode(*node, line, &sep_offset);
      const char* extra = NULL;
      if (node->type() == 'f') {
        extra = static_cast<const File*>(node)->checksum();
      } else
      if (node->type() == 'l') {
        extra = static_cast<const Link*>(node)->link();
      }
      putData(epoch, line, extra);
    } else {
      putData(epoch, "-", NULL);
    }
  }
};

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

static void showList(ListReader& list) {
  if (list.open() < 0) {
    cerr << "Failed to open " << list.path() << ": " << strerror(errno) << endl;
  } else {
    if (list.end()) {
      cout << list.path() << " is empty" << endl;
    } else {
      ListReader::Status rc;
      Path path;
      while ((rc = list.fetchLine()) > 0) {
        if (rc == ListReader::got_path) {
          path = list.getPath();
          list.resetStatus();
        } else {
          time_t ts;
          Node*  node;
          List::decodeLine(list.getData(), &ts, &node, path);
          showLine(ts, path, node);
          list.resetStatus();
        }
      }
      if (rc == ListReader::failed) {
        cerr << "Failed to read " << list.path() << endl;
      } else
      if (rc == ListReader::eof) {
        cerr << "Unexpected end of " << list.path() << endl;
      }
    }
    list.close();
  }
}

int main(void) {
  ListWriter dblist("test_db/list");
  ListWriterJournal journal("test_db/journal");
  ListWriter merge("test_db/merge");
  ListReader dblist_reader("test_db/list");
  ListReader journal_reader("test_db/journal");
  ListReader merge_reader("test_db/merge");
  Node* node   = NULL;
  int sys_rc;

  cout << "Test: DB lists" << endl;
  mkdir("test_db", 0755);
  report.setLevel(debug);


  cout << endl << "Test: list creation" << endl;

  if (dblist.open()) {
    cerr << strerror(errno) << " opening list!" << endl;
    return 0;
  }
  dblist.close();

  dblist_reader.show();


  cout << endl << "Test: journal write" << endl;
  my_time++;

  if (journal.open()) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }

  // No checksum
  node = new Stream("test1/test space");
  journal.add("file sp", time(NULL), node);
  delete node;

  journal.add("file_gone", time(NULL));

  node = new Stream("test1/testfile");
  static_cast<Stream*>(node)->open(O_RDONLY);
  static_cast<Stream*>(node)->computeChecksum();
  static_cast<Stream*>(node)->close();
  journal.add("file_new", time(NULL), node);
  delete node;

  Link* link = new Link("test1/testlink");
  char max_link[PATH_MAX];
  memset(max_link, '?', sizeof(max_link));
  max_link[sizeof(max_link) - 1] = '\0';
  link->setLink(max_link);
  node = link;
  journal.add("link", time(NULL), node);
  delete node;

  node = new Link("test1/longlink");
  journal.add("longlink", time(NULL), node);
  delete node;

  node = new Directory("test1/testdir");
  node->setSize(0);
  journal.add("path", time(NULL), node);
  delete node;

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
  if (merge.open()) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (ListWriter::merge(merge, dblist_reader, journal_reader) < 0) {
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

  if (journal.open()) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  sys_rc = system("echo \"this is my new test\" > test1/testfile");

  node = new Link("test1/testlink");
  journal.add("CR/x", time(NULL), node);
  delete node;

  node = new Link("test1/testlink");
  journal.add("CR\r", time(NULL), node);
  delete node;

  node = new Stream("test1/test space");
  static_cast<Stream*>(node)->open(O_RDONLY);
  static_cast<Stream*>(node)->computeChecksum();
  static_cast<Stream*>(node)->close();
  journal.add("file sp", time(NULL), node);
  delete node;

  node = new Stream("test1/testfile");
  static_cast<Stream*>(node)->open(O_RDONLY);
  static_cast<Stream*>(node)->computeChecksum();
  static_cast<Stream*>(node)->close();
  journal.add("file_new", time(NULL), node);
  delete node;

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
  if (merge.open()) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (ListWriter::merge(merge, dblist_reader, journal_reader) < 0) {
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
  ListReader::Status status;
  while ((status = dblist_reader.fetchLine()) > 0) {
    if (status == ListReader::got_path) {
      cout << dblist_reader.getPath() << endl;
    }
    dblist_reader.resetStatus();
  }
  dblist_reader.close();


  cout << endl << "Test: journal path out of order" << endl;
  my_time++;

  if (journal.open()) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  sys_rc = system("echo \"this is my new test\" > test1/testfile");
  node = new Stream("test1/testfile");
  static_cast<Stream*>(node)->open(O_RDONLY);
  static_cast<Stream*>(node)->computeChecksum();
  static_cast<Stream*>(node)->close();
  journal.add("file_new", time(NULL), node);
  journal.add("file_gone", time(NULL), node);
  delete node;
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
  if (merge.open()) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (ListWriter::merge(merge, dblist_reader, journal_reader) < 0) {
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

  time_t remove;
  remove = 5;
  cout << "Date: " << remove << endl;
  if (dblist_reader.open()) {
    cerr << strerror(errno) << " opening list!" << endl;
    return 0;
  }
  if (dblist_reader.end()) {
    cout << "Register is empty" << endl;
  }
  if (merge.open()) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (merge.search(&dblist_reader, "", remove, 0) < 0) {
    cerr << "Failed to copy: " << strerror(errno) << endl;
    return 0;
  }
  merge.close();
  dblist_reader.close();

  showList(merge_reader);

  // All but last
  remove = 0;
  cout << "Date: " << remove << endl;
  if (dblist_reader.open()) {
    cerr << strerror(errno) << " opening list!" << endl;
    return 0;
  }
  if (dblist_reader.end()) {
    cout << "Register is empty" << endl;
  }
  if (merge.open()) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (merge.search(&dblist_reader, "", remove, 0) < 0) {
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
  if (merge.open()) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (merge.search(&dblist_reader, "", -1, 0) < 0) {
    cerr << "Failed to copy: " << strerror(errno) << endl;
    return 0;
  }
  merge.close();
  dblist_reader.close();

  cout << "Merge:" << endl;
  showList(merge_reader);
  cout << "Merge tail:" << endl;
  sys_rc = system("sed \"s/.$//\" test_db/merge | tail -2 | grep -v 1");
  rename("test_db/merge", "test_db/list");


  cout << endl << "Test: get latest entry only" << endl;

  // Add new entry in journal
  if (journal.open()) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  sys_rc = system("echo \"this is my other test\" > test1/testfile");
  node = new Stream("test1/testfile");
  static_cast<Stream*>(node)->open(O_RDONLY);
  static_cast<Stream*>(node)->computeChecksum();
  static_cast<Stream*>(node)->close();
  journal.add("file_new", time(NULL), node);
  delete node;
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
  if (merge.open()) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (ListWriter::merge(merge, dblist_reader, journal_reader) < 0) {
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

  if (journal.open()) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }

  // No checksum
  node = new Stream("test1/test space");
  journal.add("file sp", time(NULL), node);
  delete node;

  journal.add("file_gone", time(NULL));

  node = new Stream("test1/testfile");
  static_cast<Stream*>(node)->open(O_RDONLY);
  static_cast<Stream*>(node)->computeChecksum();
  static_cast<Stream*>(node)->close();
  journal.add("file_new", time(NULL), node);
  delete node;

  node = new Link("test1/testlink");
  journal.add("link2", time(NULL), node);
  delete node;

  node = new Directory("test1/testdir");
  journal.add("path", time(NULL), node);
  delete node;

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
  if (merge.open()) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (ListWriter::merge(merge, dblist_reader, journal_reader) < 0) {
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


  cout << endl << "Test: read version 4 list" << endl;
  my_time++;

  ListReader old_list("../../../test_tools/list.v4");
  showList(old_list);

  return 0;
}
