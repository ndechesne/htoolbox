/*
     Copyright (C) 2008-2010  Herve Fache

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
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

using namespace std;

#include "test.h"

#include "files.h"
#include "list.h"
#include "missing.h"
#include "compdata.h"
#include "db.h"
#include "owner.h"

// Owner:               tested
// ~Owner:              tested
// name:                tested
// path:                tested
// expiration:          tested
// open:                somewhat tested
// close:               hopefully tested
// setProgressCallback: not visibly tested
// search:              hardly tested
// add:                 tested
// getNextRecord:       not tested
// getChecksums:        tested

// Progress
static void progress(long long previous, long long current, long long total) {
  if ((current != total) || (previous != 0)) {
    hlog_verbose_temp("Done: %5.1lf%%",
      100 * static_cast<double>(current) / static_cast<double>(total));
  }
}

static time_t my_time = 10;
time_t time(time_t *t) {
  (void) t;
  return my_time;
}

static int showRegister(const char* path) {
  Stream list(path);
  if (list.open(O_RDONLY)) {
    cout << strerror(errno) << " opening list at '" << path << "'" << endl;
    return -1;
  }
  Line line;
  while (list.getLine(line)) {
    if (line[0] != '\t') {
      // Header, footer or path
      cout << line << endl;
    } else {
      time_t ts;
      Node*  node = NULL;
      List::decodeLine(line, &ts, &node);
      cout << "\t" << ts << "\t";
      if (node != NULL) {
        printf("%c\t%6lld\t%03o", node->type(), node->size(), node->mode());
      } else {
        cout << '-';
      }
      cout << endl;
      delete node;
    }
  }
  list.close();
  return 0;
}

int main(void) {
  int sys_rc;

  cout << "Owner tests" << endl;
  report.setLevel(debug);

  mkdir("test_db", 0755);
  Owner   o("test_db", "client", 10);
  ListReader owner_list_reader("test_db/client/list");
  string remote_path = "/remote/path/";
  cout << "Name: " << o.name() << endl;
  cout << "Path: " << o.path() << endl;
  o.setProgressCallback(progress);

  int     rc;
  Node*   node;
  Node*   list_node;
  Database::OpData* op;
  Missing missing;
  Path p;

  cout << endl << "First operation" << endl;
  rc = o.open(true, false);
  if (rc) {
    cout << "Failed to open: " << rc << endl;
    return 0;
  }
  node = new File(Path("test1/testfile"));
  node->stat();
  list_node = NULL;
  p = (remote_path + node->name()).c_str();
  op = new Database::OpData(p, strlen(p), *node);
  o.send(*op, missing);
  delete list_node;
  ++my_time;
  rc = o.add(*op);
  delete node;
  delete op;
  if (rc) {
    cout << "Failed to add: " << rc << endl;
    return 0;
  }
  rc = o.close(false);
  if (rc) {
    cout << "Failed to close: " << rc << endl;
    return 0;
  }
  cout << "Register:" << endl;
  owner_list_reader.show();
  showRegister(owner_list_reader.path());
  cout << "Dir contents:" << endl;
  sys_rc = system("ls -R test_db");


  cout << endl << "Update" << endl;
  rc = o.open(true, false);
  if (rc) {
    cout << "Failed to open: " << rc << endl;
    return 0;
  }
  node = new File(Path("test2/testfile"));
  node->stat();
  list_node = NULL;
  p = (remote_path + node->name()).c_str();
  op = new Database::OpData(p, strlen(p), *node);
  o.send(*op, missing);
  delete list_node;
  ++my_time;
  rc = o.add(*op);
  delete node;
  delete op;
  if (rc) {
    cout << "Failed to add: " << rc << endl;
    return 0;
  }
  rc = o.close(false);
  if (rc) {
    cout << "Failed to close: " << rc << endl;
    return 0;
  }
  cout << "Register:" << endl;
  owner_list_reader.show();
  cout << "Dir contents:" << endl;
  sys_rc = system("ls -R test_db");


  cout << endl << "Abort client" << endl;
  rc = o.open(true, false);
  if (rc) {
    cout << "Failed to open: " << rc << endl;
    return 0;
  }
  node = new File(Path("test1/testfile"));
  node->stat();
  list_node = NULL;
  p = (remote_path + node->name()).c_str();
  op = new Database::OpData(p, strlen(p), *node);
  o.send(*op, missing);
  delete list_node;
  ++my_time;
  rc = o.add(*op);
  delete node;
  delete op;
  if (rc) {
    cout << "Failed to add: " << rc << endl;
    return 0;
  }
  rc = o.close(true);
  if (rc) {
    cout << "Failed to close: " << rc << endl;
    return 0;
  }
  cout << "Register:" << endl;
  owner_list_reader.show();
  cout << "Dir contents:" << endl;
  sys_rc = system("ls -R test_db");


  cout << endl << "Abort backup" << endl;
  rc = o.open(true, false);
  if (rc) {
    cout << "Failed to open: " << rc << endl;
    return 0;
  }
  node = new File(Path("test2/testfile"));
  node->stat();
  list_node = NULL;
  p = (remote_path + node->name()).c_str();
  op = new Database::OpData(p, strlen(p), *node);
  o.send(*op, missing);
  delete list_node;
  op->extra = "checksum test";
  ++my_time;
  rc = o.add(*op);
  delete node;
  delete op;
  if (rc) {
    cout << "Failed to add: " << rc << endl;
    return 0;
  }
  hbackup::abort();
  rc = o.close(false);
  if (rc) {
    cout << "Failed to close: " << rc << endl;
    return 0;
  }
  cout << "Register:" << endl;
  owner_list_reader.show();
  cout << "Dir contents:" << endl;
  sys_rc = system("ls -R test_db");
  hbackup::abort(0xffff);

  cout << endl << "Recover client" << endl;
  rc = o.open(false, false);
  if (rc) {
    cout << "Failed to open: " << rc << endl;
    return 0;
  }
  rc = o.close(true);
  if (rc) {
    cout << "Failed to close: " << rc << endl;
    return 0;
  }
  cout << "Register:" << endl;
  owner_list_reader.show();
  cout << "Dir contents:" << endl;
  sys_rc = system("ls -R test_db");


  cout << endl << "Recover client (empty journal)" << endl;
  {
    ListWriter journal("test_db/client/journal");
    journal.open();
    journal.close();
  }
  rc = o.open(false, false);
  if (rc) {
    cout << "Failed to open: " << rc << endl;
    return 0;
  }
  rc = o.close(true);
  if (rc) {
    cout << "Failed to close: " << rc << endl;
    return 0;
  }
  cout << "Register:" << endl;
  owner_list_reader.show();
  cout << "Dir contents:" << endl;
  sys_rc = system("ls -R test_db");


  cout << endl << "Recover client (empty, not closed journal)" << endl;
  {
    ListWriter journal("test_db/client/journal2");
    journal.open();
    journal.close();
    sys_rc = system("head -1 test_db/client/journal2 > test_db/client/journal");
    sys_rc = system("rm test_db/client/journal2");
  }
  rc = o.open(false, false);
  if (rc) {
    cout << "Failed to open: " << rc << endl;
    return 0;
  }
  rc = o.close(true);
  if (rc) {
    cout << "Failed to close: " << rc << endl;
    return 0;
  }
  cout << "Register:" << endl;
  owner_list_reader.show();
  cout << "Dir contents:" << endl;
  sys_rc = system("ls -R test_db");


  cout << endl << "Get checksums" << endl;
  list<CompData> checksums;
  rc = o.getChecksums(checksums);
  if (rc) {
    cout << "Failed to get checksums: " << rc << endl;
    return 0;
  }
  cout << "Register:" << endl;
  if (checksums.empty()) {
    cout << "...is empty" << endl;
  } else
  for (list<CompData>::iterator i = checksums.begin();
      i != checksums.end(); i++) {
    cout << " -> " << i->checksum() << ", " << i->size() << endl;
  }
  cout << "Dir contents:" << endl;
  sys_rc = system("ls -R test_db");


  cout << endl << "End of tests" << endl;
  return 0;
}
