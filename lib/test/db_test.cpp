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
#include <string>
#include <list>
#include <sys/stat.h>
#include <errno.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "report.h"
#include "list.h"
#include "opdata.h"
#include "db.h"

using namespace hbackup;

// Tests status:
//   lock:        tested
//   unlock:      tested
//   merge:       tested in paths test
//   path:        tested in paths test
//   open:        tested in paths test, but not where DB list is gone
//   open(ro):    tested
//   close:       tested in paths test
//   close(ro):   tested
//   getRecords:  tested in paths test
//   restore:     tested in interface test
//   scan:        basically tested, tested in interface test
//   check:       basically tested, tested in interface test
//   add:         tested in paths test
//   remove:      tested in paths test

static time_t my_time = 0;
time_t time(time_t *t) {
  (void) t;
  return ++my_time;
}

int main(void) {
  string      checksum;
  string      zchecksum;
  int         status;
  List        dblist("test_db/myClient/list");
  ListReader  dblist_reader("test_db/myClient/list");
  int         sys_rc;

  setVerbosityLevel(debug);
  Database db("test_db");


  /* Test incompatible arguments, open r-o + initialize */
  status = db.open(true, true);
  if (status < 0) {
    cout << "db::open error status " << status << endl;
  } else {
    return 0;
  }


  /* Test missing database, open r-o */
  status = db.open(true);
  if (status < 0) {
    cout << "db::open error status " << status << endl;
  } else {
    return 0;
  }


  /* Test missing database, open r/w */
  status = db.open();
  if (status < 0) {
    cout << "db::open error status " << status << endl;
  } else {
    return 0;
  }


  mkdir("test_db", 0755);
  /* Test missing database contents, open r-o */
  status = db.open(true);
  if (status < 0) {
    cout << "db::open error status " << status << endl;
  } else {
    return 0;
  }


  /* Test missing database contents, open r/w */
  status = db.open(false);
  if (status < 0) {
    cout << "db::open error status " << status << endl;
  } else {
    return 0;
  }
  remove("test_db");


  /* Test database */
  if ((status = db.open(false, true))) {
    cout << "db::open error status " << status << endl;
    if (status < 0) {
      return 0;
    }
  }


  cout << endl << "Test: scan & check" << endl;
  if (db.scan()) {
    printf("db.scan: %s\n", strerror(errno));
  }
  db.close();
  if ((status = db.open())) {
    cout << "db::open error status " << status << endl;
    if (status < 0) {
      return 0;
    }
  }
  cout << "File data gone" << endl;
  Directory("test_db/.data/59ca0efa9f5633cb0371bbc0355478d8-0").create();
  if (db.scan()) {
    printf("db.scan: %s\n", strerror(errno));
  }
  db.close();
  if ((status = db.open())) {
    cout << "db::open error status " << status << endl;
    if (status < 0) {
      return 0;
    }
  }
  cout << "File data corrupted, surficial scan" << endl;
  if (Directory("test_db/.data/59ca0efa9f5633cb0371bbc0355478d8-0").create()) {
    cout << "Directory still exists!" << endl;
  }
  File("test_db/.data/59ca0efa9f5633cb0371bbc0355478d8-0/data").create();
  sys_rc = system("echo 0 > test_db/.data/59ca0efa9f5633cb0371bbc0355478d8-0/meta");
  if (db.scan()) {
    printf("db.scan: %s\n", strerror(errno));
  }
  db.close();
  if ((status = db.open())) {
    cout << "db::open error status " << status << endl;
    if (status < 0) {
      return 0;
    }
  }
  if (db.scan()) {
    printf("db.scan: %s\n", strerror(errno));
  }
  cout << "File data corrupted, thorough scan" << endl;
  if (Directory("test_db/.data/59ca0efa9f5633cb0371bbc0355478d8-0").create()) {
    cout << "Directory still exists!" << endl;
  }
  File("test_db/.data/59ca0efa9f5633cb0371bbc0355478d8-0/data").create();
  sys_rc = system("echo 0 > test_db/.data/59ca0efa9f5633cb0371bbc0355478d8-0/meta");
  db.close();

  if ((status = db.open(true))) {
    cout << "db::open error status " << status << endl;
    if (status < 0) {
      return 0;
    }
  }
  if (db.check()) {
    printf("db.check: %s\n", strerror(errno));
  }
  db.close();

  if ((status = db.open())) {
    cout << "db::open error status " << status << endl;
    if (status < 0) {
      return 0;
    }
  }
  if (db.scan()) {
    printf("db.scan: %s\n", strerror(errno));
  }
  cout << "Scan again" << endl;
  if (Directory("test_db/.data/59ca0efa9f5633cb0371bbc0355478d8-0").create()) {
    cout << "Directory still exists!" << endl;
    if (File("test_db/.data/59ca0efa9f5633cb0371bbc0355478d8-0/data").isValid())
    {
      cout << "File still exists!" << endl;
    }
  }
  db.close();


  cout << endl << "Test: lock" << endl;
  if (! db.open()) {
    db.close();
  }
  sys_rc = system("echo 100000 > test_db/.lock");
  if (! db.open()) {
    db.close();
  }
  sys_rc = system("echo 1 > test_db/.lock");
  if (! db.open()) {
    db.close();
  }
  sys_rc = system("echo 0 > test_db/.lock");
  if (! db.open()) {
    db.close();
  }
  sys_rc = system("chmod 0 test_db/.lock");
  if (! db.open()) {
    db.close();
  }
  sys_rc = system("chmod 600 test_db/.lock");
  if (! db.open()) {
    db.close();
  }
  remove("test_db/.lock");


  cout << endl << "Test: read-only mode" << endl;
  if ((status = db.open(true))) {
    cout << "db::open error status " << status << endl;
    if (status < 0) {
      return 0;
    }
  }
  db.close();


  cout << endl << "Test: fill in DB" << endl;
  if ((status = db.open())) {
    cout << "db::open error status " << status << endl;
    if (status < 0) {
      return 0;
    }
  }

  db.openClient("myClient");

  Directory* d;
  File* f;
  OpData* op;
  d = new Directory("test1/subdir");
  d->setSize(0);
  op = new OpData("/client_path/subdir", *d);
  db.add(*op);
  delete op;
  delete d;
  f = new File("test1/subdir/testfile");
  op = new OpData("/client_path/subdir/testfile", *f);
  op->setCompression(5);
  db.add(*op);
  delete op;
  delete f;
  f = new File("test1/subdir/testfile2");
  op = new OpData("/client_path/subdir/testfile2", *f);
  db.add(*op);
  delete op;
  delete f;
  f = new File("test1/test space");
  op = new OpData("/client_path/test space", *f);
  db.add(*op);
  delete op;
  delete f;
  f = new File("test1/testfile");
  op = new OpData("/client_path/testfile", *f);
  db.add(*op);
  delete op;
  delete f;
  d = new Directory("test1/testdir");
  d->setSize(0);
  op = new OpData("other_path/testdir", *d);
  db.add(*op);
  delete op;
  delete d;
  f = new File("test2/testfile");
  op = new OpData("other_path/testfile", *f);
  db.add(*op);
  delete op;
  delete f;

  db.closeClient();
  rename("test_db/myClient/journal~", "test_db/myClient/list");

  db.close();

  dblist_reader.show();

  cout << endl << "Test: do nothing" << endl;
  if ((status = db.open())) {
    cout << "db::open error status " << status << endl;
    if (status < 0) {
      return 0;
    }
  }
  db.close();

  dblist_reader.show();

  list<string> records;
  cout << endl << "Test: read clients" << endl;
  // No open needed to get clients
  if ((status = db.open(true))) {
    cout << "db::open error status " << status << endl;
    if (status < 0) {
      return 0;
    }
  }
  if (db.getClients(records) < 0) {
    cerr << "Failed to get list of clients" << endl;
  }
  db.close();
  cout << "List of clients: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: read paths in myClient" << endl;
  if ((status = db.open(true))) {
    cout << "db::open error status " << status << endl;
    if (status < 0) {
      return 0;
    }
  }
  if ((status = db.openClient("myClient"))) {
    cout << "db::open error status " << status << endl;
    if (status < 0) {
      return 0;
    }
  }
  db.getRecords(records);
  db.closeClient();
  cout << "List of paths: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: read paths in myClient below /client_path"
    << endl;
  if ((status = db.openClient("myClient"))) {
    cout << "db::open error status " << status << endl;
    if (status < 0) {
      return 0;
    }
  }
  db.getRecords(records, "/client_path");
  db.closeClient();
  cout << "List of paths: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: read paths in myClient below other_path"
    << endl;
  if ((status = db.openClient("myClient"))) {
    cout << "db::open error status " << status << endl;
    if (status < 0) {
      return 0;
    }
  }
  db.getRecords(records, "other_path");
  db.closeClient();
  cout << "List of paths: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: read paths in myClient below /client_path/subdir"
    << endl;
  if ((status = db.openClient("myClient"))) {
    cout << "db::open error status " << status << endl;
    if (status < 0) {
      return 0;
    }
  }
  db.getRecords(records, "/client_path/subdir");
  db.closeClient();
  cout << "List of paths: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  db.close();


  cout << endl << "Test: scan" << endl;
  File("test_db/.data/3d546a1ce46c6ae10ad34ab8a81c542e-0/data").remove();
  Directory("test_db/.data/6d7fce9fee471194aa8b5b6e47267f03-0").create();
  sys_rc = system("echo '3' > test_db/.data/6d7fce9fee471194aa8b5b6e47267f03-0/data");
  sys_rc = system("echo 2 > test_db/.data/6d7fce9fee471194aa8b5b6e47267f03-0/meta");
  if ((status = db.open())) {
    cout << "db::open error status " << status << endl;
    if (status < 0) {
      return 0;
    }
  }
  if (db.scan(true) != 0) {
    cout << "db::scan2 failed" << endl;
  }
  db.close();


  cout << endl << "Test: concurrent access" << endl;
  if (db.open() == 0) {
    Database db2("test_db");
    if (db2.open(true) < 0) {
      return 0;
    }
    db2.openClient("myClient");
    db.openClient("myClient");

    Link l("test1/testlink");
    OpData o("/client_path/new_link", l);
    db.add(o);
    db.closeClient();
    rename("test_db/myClient/journal~", "test_db/myClient/list");
    db2.getRecords(records, "/client_path");
    db2.closeClient();
    db2.close();
    db.close();
  }
  dblist_reader.show();
  cout << "List of paths: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end();
      i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  if (db.open(true) < 0) {
    return 0;
  }
  if (db.openClient("myClient") != 0) {
    return 0;
  }
  db.getRecords(records, "/client_path");
  db.closeClient();
  db.close();
  cout << "List of paths: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end();
      i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << "Date: " << 0 << endl;
  if (db.open(true) < 0) {
    return 0;
  }
  if (db.openClient("myClient") != 0) {
    return 0;
  }
  db.getRecords(records, "/client_path", 0);
  db.closeClient();
  db.close();
  cout << "List of paths: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end();
      i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << "Date: " << -4 << " from " << my_time << " (+1)" << endl;
  if (db.open(true) < 0) {
    return 0;
  }
  if (db.openClient("myClient") != 0) {
    return 0;
  }
  db.getRecords(records, "/client_path", -4);
  db.closeClient();
  db.close();
  cout << "List of paths: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end();
      i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << "Date: " << 11 << endl;
  if (db.open(true) < 0) {
    return 0;
  }
  if (db.openClient("myClient") != 0) {
    return 0;
  }
  db.getRecords(records, "/client_path", 11);
  db.closeClient();
  db.close();
  cout << "List of paths: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end();
      i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  return 0;
}
