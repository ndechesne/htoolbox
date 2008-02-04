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

#include "files.h"
#include "list.h"
#include "data.h"
#include "hbackup.h"

using namespace hbackup;

// Tests status:
//   getDir:          tested
//   organise:        tested
//   open:            tested
//   close:           tested
//   read:            tested
//   write:           tested
//   check:           tested in db through crawl/scan
//   remove:          tested in db
//   crawl:           tested in db
//   parseChecksums:  NOT IMPLEMENTED

class DataTest : public Data {
public:
  int getDir(
      const string&   checksum,
      string&         path,
      bool            create) {
    return Data::getDir(checksum, path, create);
  }
  int  organise(
      const string&   path,
      int             number) {
    return Data::organise(path, number);
  }
};

static int verbose = 4;

int hbackup::verbosity(void) {
  return verbose;
}

int hbackup::terminating(const char* unused) {
  return 0;
}

time_t time(time_t *t) {
  static time_t my_time = 0;
  return ++my_time;
}

int main(void) {
  string            checksum;
  string            zchecksum;
  int               status;
  DataTest          db;

  /* Test database */
  Directory("test_db").create();
  status = db.open("test_db/data", true);
  if (status) {
    cout << "db::open status " << status << endl;
    if (status < 0) {
      return 0;
    }
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


  cout << endl << "Test: read-only mode" << endl;
  if ((status = db.open("test_db/data"))) {
    cout << "db::open error status " << status << endl;
    if (status == 2) {
      return 0;
    }
  }
  db.close();


  cout << endl << "Test: do nothing" << endl;
  if ((status = db.open("test_db/data", true))) {
    cout << "db::open error status " << status << endl;
    if (status == 2) {
      return 0;
    }
  }
  db.close();

  return 0;
}