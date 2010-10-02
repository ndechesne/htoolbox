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
#include <string>
#include <list>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

using namespace std;

#include "test.h"

#include "files.h"
#include "compdata.h"
#include "data.h"

// Tests status:
//   getDir:          tested
//   organise:        tested
//   open:            tested
//   read:            tested
//   write:           tested
//   check:           tested in db through crawl/scan
//   remove:          tested in db
//   crawl:           tested in db
//   parseChecksums:  NOT IMPLEMENTED

class DataTest : public Data {
public:
  DataTest(const char* path) : Data(path) {}
  int getDir(
      const char*     checksum,
      char*           path,
      bool            create) {
    return Data::getDir(checksum, path, create);
  }
  int organise(
      const char*     path,
      int             number) {
    return Data::organise(path, number);
  }
  int check(
      const char*     checksum,
      bool            thorough   = true,
      bool            repair     = false,
      long long*      size       = NULL,
      bool*           compressed = NULL) const {
    return Data::check(checksum, thorough, repair, size, compressed);
  }
};

time_t time(time_t *t) {
  (void) t;
  static time_t my_time = 0;
  return ++my_time;
}

int main(void) {
  string            checksum;
  string            zchecksum;
  int               status;
  DataTest          db("test_db/data");
  int               sys_rc;

  report.setLevel(debug);

  /* Test database */
  Directory("test_db").create();
  status = db.open(true);
  if (status) {
    cout << "db::open status " << status << endl;
    if (status < 0) {
      return 0;
    }
  }


  cout << endl << "Test: getdir" << endl;
  cout << "Check test_db/data dir: " << ! Directory("test_db/data").isValid()
    << endl;
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
  char getdir_path[PATH_MAX];
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


  char chksm[64];
  long long size;
  bool compressed;
  {
    cout << endl << "Test: write and read back with no compression (auto)" << endl;
    /* Write */
    chksm[0] = '\0';
    const char* testfile = "test1/testfile";
    int comp_level = 5;
    if ((status = db.write(testfile, chksm, &comp_level, true)) < 0) {
      printf("db.write error status %d\n", status);
      return 0;
    }
    if (chksm == NULL) {
      printf("db.write returned unexpected null checksum\n");
      return 0;
    }
    cout << chksm << "  " << testfile << endl;
    cout << "Meta file contents: " << endl;
    sys_rc = system("cat test_db/data/59/ca0efa9f5633cb0371bbc0355478d8-0/meta");
    cout << endl;
    /* Check */
    if ((status = db.check(chksm, true, true, &size, &compressed)) < 0) {
      printf("db.check error status %d\n", status);
      return 0;
    }

    /* Read */
    if ((status = db.read("test_db/blah", chksm))) {
      printf("db.read error status %d\n", status);
      return 0;
    }
    /* Write again */
    chksm[0] = '\0';
    const char* blah = "test_db/blah";
    comp_level = 0;
    if ((status = db.write(blah, chksm, &comp_level, false)) < 0) {
      printf("db.write error status %d\n", status);
      return 0;
    }
    if (chksm == NULL) {
      printf("db.write returned unexpected null checksum\n");
      return 0;
    }
    cout << chksm << "  " << blah << endl;
  }


  {
    cout << endl << "Test: write and read back with compression (auto)" << endl;
    /* Write */
    const char* testfile = "test1/big_file";
    int comp_level = 5;
    if ((status = db.write(testfile, chksm, &comp_level, true)) < 0) {
      printf("db.write error status %d\n", status);
      return 0;
    }
    if (chksm == NULL) {
      printf("db.write returned unexpected null checksum\n");
      return 0;
    }
    cout << chksm << "  " << testfile << endl;
    cout << "Meta file contents: " << endl;
    sys_rc = system("cat test_db/data/f1/c9645dbc14efddc7d8a322685f26eb-0/meta");
    cout << endl;
    /* Check */
    if ((status = db.check(chksm, true, true, &size, &compressed)) < 0) {
      printf("db.check error status %d\n", status);
      return 0;
    }

    /* Read */
    if ((status = db.read("test_db/blah", chksm))) {
      printf("db.read error status %d\n", status);
      return 0;
    }
    /* Write again */
    chksm[0] = '\0';
    const char* blah = "test_db/blah";
    comp_level = 0;
    if ((status = db.write(blah, chksm, &comp_level, false)) < 0) {
      printf("db.write error status %d\n", status);
      return 0;
    }
    if (chksm == NULL) {
      printf("db.write returned unexpected null checksum\n");
      return 0;
    }
    cout << chksm << "  " << blah << endl;
  }


  const char* testfile = "test1/testfile";
  const char* blah = "test_db/blah";
  {
    cout << endl << "Test: write and read back with forbidden compression"
      << endl;
    /* Write */
    int comp_level = -1;
    if ((status = db.write(testfile, chksm, &comp_level, true)) < 0) {
      printf("db.write error status %d\n", status);
      return 0;
    }
    if (chksm == NULL) {
      printf("db.write returned unexpected null checksum\n");
      return 0;
    }
    cout << chksm << "  " << testfile << endl;
    cout << "Meta file contents: " << endl;
    sys_rc =
      system("cat test_db/data/59/ca0efa9f5633cb0371bbc0355478d8-0/meta");
    cout << endl;
    /* Check */
    if ((status = db.check(chksm, true, true, &size, &compressed)) < 0) {
      printf("db.check error status %d\n", status);
      return 0;
    }

    /* Read */
    if ((status = db.read("test_db/blah", chksm))) {
      printf("db.read error status %d\n", status);
      return 0;
    }
    /* Write again */
    chksm[0] = '\0';
    comp_level = 0;
    if ((status = db.write(blah, chksm, &comp_level, false)) < 0) {
      printf("db.write error status %d\n", status);
      return 0;
    }
    if (chksm == NULL) {
      printf("db.write returned unexpected null checksum\n");
      return 0;
    }
    cout << chksm << "  " << blah << endl;
  }


  cout << endl << "Test: write and read back with forced compression" << endl;
  Node("test_db/data/59/ca0efa9f5633cb0371bbc0355478d8-0/data", false).remove();
  /* Write */
  chksm[0] = '\0';
  int comp_level = 5;
  if ((status = db.write(testfile, chksm, &comp_level, false)) < 0) {
    printf("db.write error status %d\n", status);
    return 0;
  }
  if (chksm == NULL) {
    printf("db.write returned unexpected null checksum\n");
    return 0;
  }
  cout << chksm << "  " << testfile << endl;
  cout << "Meta file contents: " << endl;
  sys_rc = system("cat test_db/data/59/ca0efa9f5633cb0371bbc0355478d8-0/meta");
  cout << endl;

  /* Check and repair */
  Node("test_db/data/59/ca0efa9f5633cb0371bbc0355478d8-0/meta", false).remove();
  if ((status = db.check(chksm, true, true, &size, &compressed)) < 0) {
    printf("db.check error status %d\n", status);
    return 0;
  }
  cout << "Size reported: " << size << endl;
  cout << "Meta file contents: " << endl;
  sys_rc = system("cat test_db/data/59/ca0efa9f5633cb0371bbc0355478d8-0/meta");
  cout << endl;

  /* Re-check */
  if ((status = db.check(chksm, true, true, &size, &compressed)) < 0) {
    printf("db.check error status %d\n", status);
    return 0;
  }

  /* Read */
  if ((status = db.read("test_db/blah", chksm))) {
    printf("db.read error status %d\n", status);
    return 0;
  }
  /* Write again */
  chksm[0] = '\0';
  comp_level = 0;
  if ((status = db.write(blah, chksm, &comp_level, false)) < 0) {
    printf("db.write error status %d\n", status);
    return 0;
  }
  if (chksm == NULL) {
    printf("db.write returned unexpected null checksum\n");
    return 0;
  }
  /* Write again */
  chksm[0] = '\0';
  comp_level = 0;
  if ((status = db.write(blah, chksm, &comp_level, false)) < 0) {
    printf("db.write error status %d\n", status);
    return 0;
  }
  if (chksm == NULL) {
    printf("db.write returned unexpected null checksum\n");
    return 0;
  }
  cout << chksm << "  " << blah << endl;


  cout << endl << "Test: organise" << endl;
  mkdir("test_db/data/zz", 0755);
  mkdir("test_db/data/zz/000001", 0755);
  cout << "Lesser:" << endl;
  db.organise("test_db/data/zz", 2);
  sys_rc = system("LANG=en_US.UTF-8 ls -AR test_db/data/zz");
  mkdir("test_db/data/zz/000002", 0755);
  cout << "Equal:" << endl;
  db.organise("test_db/data/zz", 2);
  sys_rc = system("LANG=en_US.UTF-8 ls -AR test_db/data/zz");
  mkdir("test_db/data/zz/00/0003", 0755);
  cout << "Greater:" << endl;
  db.organise("test_db/data/zz/00", 2);
  sys_rc = system("LANG=en_US.UTF-8 ls -AR test_db/data/zz");



  cout << endl << "Test: read-only mode" << endl;
  if ((status = db.open("test_db/data"))) {
    cout << "db::open error status " << status << endl;
    if (status < 0) {
      return 0;
    }
  }


  cout << endl << "Test: do nothing" << endl;
  if ((status = db.open(true))) {
    cout << "db::open error status " << status << endl;
    if (status < 0) {
      return 0;
    }
  }


  {
    cout << endl << "Test: scan - metadata check/correct" << endl;
    /* Open */
    if ((status = db.open(true))) {
      cout << "db::open error status " << status << endl;
      if (status < 0) {
        return 0;
      }
    }
    /* Compressed */
    strcpy(chksm, "59ca0efa9f5633cb0371bbc0355478d8-0");
    sys_rc = system("gzip "
      "test_db/data/59/ca0efa9f5633cb0371bbc0355478d8-0/data");
    /* Check */
    cout << " * missing" << endl;
    sys_rc = system("echo -n > "
      "test_db/data/59/ca0efa9f5633cb0371bbc0355478d8-0/meta");
    if ((status = db.check(chksm, false, true, &size, &compressed)) < 0) {
      printf("db.check error status %d\n", status);
    }
    cout << " * check" << endl;
    if ((status = db.check(chksm, false, true, &size, &compressed)) < 0) {
      printf("db.check error status %d\n", status);
    }
    /* Uncompressed */
    sys_rc = system("mkdir -p test_db/data/d4/1d8cd98f00b204e9800998ecf8427e");
    sys_rc = system("echo -n > "
      "test_db/data/d4/1d8cd98f00b204e9800998ecf8427e/data");
    strcpy(chksm, "d41d8cd98f00b204e9800998ecf8427e");
    /* Check */
    cout << " * missing" << endl;
    sys_rc = system("echo -n > "
      "test_db/data/d4/1d8cd98f00b204e9800998ecf8427e/meta");
    if ((status = db.check(chksm, false, true, &size, &compressed)) < 0) {
      printf("db.check error status %d\n", status);
    }
    cout << " * check" << endl;
    if ((status = db.check(chksm, false, true, &size, &compressed)) < 0) {
      printf("db.check error status %d\n", status);
    }
    /* Close */
  }


  {
    cout << endl << "Test: check - metadata check/correct" << endl;
    /* Open */
    if ((status = db.open(true))) {
      cout << "db::open error status " << status << endl;
      if (status < 0) {
        return 0;
      }
    }
    /* Compressed */
    strcpy(chksm, "59ca0efa9f5633cb0371bbc0355478d8-0");
    /* Check */
    cout << " * missing" << endl;
    sys_rc = system("echo -n > "
      "test_db/data/59/ca0efa9f5633cb0371bbc0355478d8-0/meta");
    if ((status = db.check(chksm, true, true, &size, &compressed)) < 0) {
      printf("db.check error status %d\n", status);
    }
    cout << " * wrong" << endl;
    sys_rc = system("sed -i 's/13/2/' "
      "test_db/data/59/ca0efa9f5633cb0371bbc0355478d8-0/meta");
    if ((status = db.check(chksm, true, true, &size, &compressed)) < 0) {
      printf("db.check error status %d\n", status);
    }
    cout << " * check" << endl;
    if ((status = db.check(chksm, true, true, &size, &compressed)) < 0) {
      printf("db.check error status %d\n", status);
    }
    /* Uncompressed */
    sys_rc = system("mkdir -p test_db/data/d4/1d8cd98f00b204e9800998ecf8427e");
    sys_rc = system("echo -n > "
      "test_db/data/d4/1d8cd98f00b204e9800998ecf8427e/data");
    strcpy(chksm, "d41d8cd98f00b204e9800998ecf8427e");
    /* Check */
    cout << " * missing" << endl;
    sys_rc = system("echo -n > "
      "test_db/data/d4/1d8cd98f00b204e9800998ecf8427e/meta");
    if ((status = db.check(chksm, true, true, &size, &compressed)) < 0) {
      printf("db.check error status %d\n", status);
    }
    cout << " * wrong" << endl;
    sys_rc = system("sed -i 's/0/1/' "
      "test_db/data/d4/1d8cd98f00b204e9800998ecf8427e/meta");
    if ((status = db.check(chksm, true, true, &size, &compressed)) < 0) {
      printf("db.check error status %d\n", status);
    }
    cout << " * check" << endl;
    if ((status = db.check(chksm, true, true, &size, &compressed)) < 0) {
      printf("db.check error status %d\n", status);
    }
  }


  return 0;
}
