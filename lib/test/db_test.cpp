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
#include <string>
#include <list>
#include <sys/stat.h>
#include <errno.h>

using namespace std;

#include "strings.h"
#include "files.h"
#include "list.h"
#include "db.h"
#include "hbackup.h"

using namespace hbackup;

// Tests status:
//   isOpen:      tested
//   isWriteable: tested
//   lock:        tested
//   unlock:      tested
//   merge:       tested in paths
//   getDir:      tested
//   organise:    tested
//   write:       tested
//   crawl:       FIXME not tested
//   path:        tested in paths
//   open:        tested in paths
//   open(ro):    tested
//   close:       tested in paths
//   close(ro):   tested
//   getList:     tested in paths
//   read:        tested
//   scan:        FIXME broken, not tested
//   add:         tested in paths
//   remove:      tested in paths

class DbTest : public Database {
public:
  DbTest(const string& path) :
    Database::Database(path) {}
  bool isOpen() const {
    return Database::isOpen();
  }
  bool isWriteable() const {
    return Database::isWriteable();
  }
  int getDir(
      const string&   checksum,
      string&         path,
      bool            create) {
    return Database::getDir(checksum, path, create);
  }
  int  organise(
      const string&   path,
      int             number) {
    return Database::organise(path, number);
  }
  int  write(
      const string&   path,
      char**          checksum,
      int             compress = 0) {
    return Database::write(path, checksum, compress);
  }
  int  crawl(Directory &dir, string path, bool check) const {
    return Database::crawl(dir, path, check);
  }
};

static int verbose = 4;

int hbackup::verbosity(void) {
  return verbose;
}

int hbackup::terminating(const char* unused) {
  return 0;
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

time_t time(time_t *t) {
  static time_t my_time = 0;
  return ++my_time;
}

int main(void) {
  string            checksum;
  string            zchecksum;
  int               status;
  List    dblist("test_db/list");
  char*   prefix = NULL;
  char*   path   = NULL;
  Node*   node   = NULL;
  time_t  ts;

  DbTest db("test_db");

  if (db.isOpen()) {
    cerr << "db is open when it should not be" << endl;
    return 0;
  }

  if (db.isWriteable()) {
    cerr << "db is writeable when it should not be" << endl;
    return 0;
  }

  /* Test database */
  if ((status = db.open())) {
    cout << "db::open error status " << status << endl;
    if (status == 2) {
      return 0;
    }
  }

  if (! db.isOpen()) {
    cerr << "db is not open when it should be" << endl;
    return 0;
  }

  if (! db.isWriteable()) {
    cerr << "db is not writeable when it should be" << endl;
    return 0;
  }


  cout << endl << "Test: getdir" << endl;
  cout << "Check test_db/data dir: " << ! Directory("test_db/data").isValid() << endl;
  File("test_db/data/.nofiles").create();
  Directory("test_db/data/fe").create();
  File("test_db/data/fe/.nofiles").create();
  File("test_db/data/fe/test4").create();
  Directory("test_db/data/fe/dc").create();
  File("test_db/data/fe/dc/.nofiles").create();
  Directory("test_db/data/fe/ba").create();
  Directory("test_db/data/fe/ba/test1").create();
  Directory("test_db/data/fe/98").create();
  Directory("test_db/data/fe/98/test2").create();
  string  getdir_path;
  cout << "febatest1 status: " << db.getDir("febatest1", getdir_path, true)
    << ", getdir_path: " << getdir_path << endl;
  cout << "fe98test2 status: " << db.getDir("fe98test2", getdir_path, true)
    << ", getdir_path: " << getdir_path << endl;
  cout << "fe98test3 status: " << db.getDir("fe98test3", getdir_path, true)
    << ", getdir_path: " << getdir_path << endl;
  cout << "fetest4 status: " << db.getDir("fetest4", getdir_path, true)
    << ", getdir_path: " << getdir_path << endl;
  cout << "fedc76test5 status: " << db.getDir("fedc76test5", getdir_path, true)
    << ", getdir_path: " << getdir_path << endl;
  Directory("test_db/data/fe/dc/76").create();
  cout << "fedc76test6 status: " << db.getDir("fedc76test6", getdir_path, true)
    << ", getdir_path: " << getdir_path << endl;
  mkdir("test_db/data/fe/dc/76/test6", 0755);
  cout << "fedc76test6 status: " << db.getDir("fedc76test6", getdir_path, true)
    << ", getdir_path: " << getdir_path << endl;


  cout << endl << "Test: write and read back" << endl;
  /* Write */
  char* chksm = NULL;
  if ((status = db.write("test1/testfile", &chksm))) {
    printf("db.write error status %d\n", status);
    db.close();
    return 0;
  }
  if (chksm == NULL) {
    printf("db.write returned unexpected null checksum\n");
    db.close();
    return 0;
  }
  cout << chksm << "  test1/testfile" << endl;
  /* Read */
  if ((status = db.read("test_db/blah", chksm))) {
    printf("db.read error status %d\n", status);
    db.close();
    return 0;
  }
  /* Write again */
  chksm = NULL;
  if ((status = db.write("test_db/blah", &chksm))) {
    printf("db.write error status %d\n", status);
    db.close();
    return 0;
  }
  if (chksm == NULL) {
    printf("db.write returned unexpected null checksum\n");
    db.close();
    return 0;
  }
  cout << chksm << "  test_db/blah" << endl;

  cout << endl << "Test: write and read back with compression" << endl;
  /* Write */
  chksm = NULL;
  if ((status = db.write("test1/testfile", &chksm, 5))) {
    printf("db.write error status %d\n", status);
    db.close();
    return 0;
  }
  if (chksm == NULL) {
    printf("db.write returned unexpected null checksum\n");
    db.close();
    return 0;
  }
  cout << chksm << "  test1/testfile" << endl;
  /* Read */
  if ((status = db.read("test_db/blah", chksm))) {
    printf("db.read error status %d\n", status);
    db.close();
    return 0;
  }
  /* Write again */
  chksm = NULL;
  if ((status = db.write("test_db/blah", &chksm))) {
    printf("db.write error status %d\n", status);
    db.close();
    return 0;
  }
  if (chksm == NULL) {
    printf("db.write returned unexpected null checksum\n");
    db.close();
    return 0;
  }
  cout << chksm << "  test_db/blah" << endl;


  cout << endl << "Test: check" << endl;
  if (db.scan("49ca0efa9f5633cb0371bbc0355678d8-0")) {
    printf("db.check: %s\n", strerror(errno));
  }
  if (db.scan("59ca0efa9f5633cb0371bbc0355678d8-0")) {
    printf("db.check: %s\n", strerror(errno));
  }
  if (db.scan("59ca0efa9f5633cb0371bbc0355478d8")) {
    printf("db.check: %s\n", strerror(errno));
  }
  if (db.scan("59ca0efa9f5633cb0371bbc0355478d8-0")) {
    printf("db.check: %s\n", strerror(errno));
  }
  if (db.scan()) {
    printf("db.check: %s\n", strerror(errno));
  }
  remove("test_db/data/59/ca0efa9f5633cb0371bbc0355478d8-0/data");
  if (db.scan("59ca0efa9f5633cb0371bbc0355478d8-0")) {
    printf("db.check: %s\n", strerror(errno));
    fflush(stdout);
  }
  File("test_db/data/59/ca0efa9f5633cb0371bbc0355478d8-0/data").create();
  if (db.scan("59ca0efa9f5633cb0371bbc0355478d8-0")) {
    printf("db.check: %s\n", strerror(errno));
  }


  cout << endl << "Test: organise" << endl;
  mkdir("test_db/data/zz", 0755);
  mkdir("test_db/data/zz/000001", 0755);
  cout << "Lesser:" << endl;
  db.organise("test_db/data/zz", 2);
  system("LANG=en_US.UTF-8 ls -AR test_db/data/zz");
  mkdir("test_db/data/zz/000002", 0755);
  cout << "Equal:" << endl;
  db.organise("test_db/data/zz", 2);
  system("LANG=en_US.UTF-8 ls -AR test_db/data/zz");
  mkdir("test_db/data/zz/00/0003", 0755);
  cout << "Greater:" << endl;
  db.organise("test_db/data/zz/00", 2);
  system("LANG=en_US.UTF-8 ls -AR test_db/data/zz");

  db.close();


  cout << endl << "Test: lock" << endl;
  if (! db.open()) {
    db.close();
  }
  system("echo 100000 > test_db/lock");
  if (! db.open()) {
    db.close();
  }
  system("echo 1 > test_db/lock");
  if (! db.open()) {
    db.close();
  }
  system("echo 0 > test_db/lock");
  if (! db.open()) {
    db.close();
  }
  system("touch test_db/lock");
  if (! db.open()) {
    db.close();
  }
  remove("test_db/lock");


  cout << endl << "Test: read-only mode" << endl;
  if ((status = db.open(true))) {
    cout << "db::open error status " << status << endl;
    if (status == 2) {
      return 0;
    }
  }

  if (! db.isOpen()) {
    cerr << "db is not open when it should be" << endl;
    return 0;
  }

  if (db.isWriteable()) {
    cerr << "db is writeable when it should not be" << endl;
    return 0;
  }

  db.close();

  if (db.isOpen()) {
    cerr << "db is open when it should not be" << endl;
    return 0;
  }

  if (db.isWriteable()) {
    cerr << "db is writeable when it should not be" << endl;
    return 0;
  }


  cout << endl << "Test: fill in DB" << endl;
  if ((status = db.open())) {
    cout << "db::open error status " << status << endl;
    if (status == 2) {
      return 0;
    }
  }

  db.setPrefix("prot://client");

  File* f;
  f = new File("test1/test space");
  db.add("/client_path/test space", f);
  delete f;
  f = new File("test1/testfile");
  db.add("/client_path/testfile", f);
  delete f;
  Directory* d;
  d = new Directory("test1/subdir");
  db.add("/client_path/subdir", d);
  delete d;
  f = new File("test1/subdir/testfile");
  db.add("/client_path/subdir/testfile", f);
  delete f;
  f = new File("test1/subdir/testfile2");
  db.add("/client_path/subdir/testfile2", f);
  delete f;

  f = new File("test2/testfile");
  db.add("other_path/testfile", f);
  delete f;
  d = new Directory("test1/testdir");
  db.add("other_path/testdir", d);
  delete d;
  
  db.close();
  rename("test_db/journal~", "test_db/list");

  dblist.open("r");
  if (dblist.isEmpty()) {
    cout << "Merge is empty" << endl;
  } else
  while ((status = dblist.getEntry(&ts, &prefix, &path, &node)) > 0) {
    showLine(ts, prefix, path, node);
  }
  dblist.close();
  if (status < 0) {
    cerr << "Failed to read list" << endl;
  }

  cout << endl << "Test: do nothing" << endl;
  if ((status = db.open())) {
    cout << "db::open error status " << status << endl;
    if (status == 2) {
      return 0;
    }
  }
  db.close();

  dblist.open("r");
  if (dblist.isEmpty()) {
    cout << "Merge is empty" << endl;
  } else
  while ((status = dblist.getEntry(&ts, &prefix, &path, &node)) > 0) {
    showLine(ts, prefix, path, node);
  }
  dblist.close();
  if (status < 0) {
    cerr << "Failed to read list" << endl;
  }


  list<string> records;
  cout << endl << "Test: read prefixes" << endl;
  if ((status = db.open(true))) {
    cout << "db::open error status " << status << endl;
    if (status == 2) {
      return 0;
    }
  }
  db.getRecords(records);
  db.close();
  cout << "List of prefixes: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: read paths in prot://client" << endl;
  if ((status = db.open(true))) {
    cout << "db::open error status " << status << endl;
    if (status == 2) {
      return 0;
    }
  }
  db.getRecords(records, "prot://client");
  db.close();
  cout << "List of paths: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: read paths in prot://client below /client_path"
    << endl;
  if ((status = db.open(true))) {
    cout << "db::open error status " << status << endl;
    if (status == 2) {
      return 0;
    }
  }
  db.getRecords(records, "prot://client", "/client_path");
  db.close();
  cout << "List of paths: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: read paths in prot://client below other_path"
    << endl;
  if ((status = db.open(true))) {
    cout << "db::open error status " << status << endl;
    if (status == 2) {
      return 0;
    }
  }
  db.getRecords(records, "prot://client", "other_path");
  db.close();
  cout << "List of paths: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: read paths in prot://client below /client_path/subdir"
    << endl;
  if ((status = db.open(true))) {
    cout << "db::open error status " << status << endl;
    if (status == 2) {
      return 0;
    }
  }
  db.getRecords(records, "prot://client", "/client_path/subdir");
  db.close();
  cout << "List of paths: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: crawl" << endl;
  if ((status = db.open())) {
    cout << "db::open error status " << status << endl;
    if (status == 2) {
      return 0;
    }
  }
  d = new Directory("test_db/data");
  if (db.crawl(*d, "", true) != 0) {
    cout << "db::crawl failed" << endl;
  }
  delete d;
  db.close();

  return 0;
}
