/*
     Copyright (C) 2008  Herve Fache

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

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "list.h"
#include "missing.h"
#include "opdata.h"
#include "owner.h"

using namespace hbackup;

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
  cout << "Done: " << 100 * current / total << "%" << endl;
}

static time_t my_time = 10;
time_t time(time_t *t) {
  return my_time;
}

int main(void) {
  cout << "Owner tests" << endl;
  setVerbosityLevel(debug);

  mkdir("test_db", 0755);
  Owner o("test_db", "client", 10);
  List owner_list("test_db/client/list");
  string remote_path = "/remote/path/";
  cout << "Name: " << o.name() << endl;
  cout << "Path: " << o.path() << endl;
  cout << "Expiration: " << o.expiration() << endl;
  o.setProgressCallback(progress);

  int   rc;
  Node* node;
  Node* list_node;

  cout << endl << "First operation" << endl;
  rc = o.open(true, false);
  if (rc) {
    cout << "Failed to open: " << rc << endl;
    return 0;
  }
  node = new File(Path("test1/testfile"));
  node->stat();
  list_node = NULL;
  rc = o.search((remote_path + node->name()).c_str(), &list_node);
  delete list_node;
  rc = o.add((remote_path + node->name()).c_str(), node, ++my_time);
  delete node;
  if (rc) {
    cout << "Failed to add: " << rc << endl;
    return 0;
  }
  rc = o.close(false);
  if (rc) {
    cout << "Failed to close: " << rc << endl;
    return 0;
  }
  cout << "List:" << endl;
  owner_list.show();
  cout << "Dir contents:" << endl;
  system("ls -R test_db");


  cout << endl << "Update" << endl;
  rc = o.open(true, false);
  if (rc) {
    cout << "Failed to open: " << rc << endl;
    return 0;
  }
  node = new File(Path("test2/testfile"));
  node->stat();
  list_node = NULL;
  rc = o.search((remote_path + node->name()).c_str(), &list_node);
  delete list_node;
  rc = o.add((remote_path + node->name()).c_str(), node, ++my_time);
  delete node;
  if (rc) {
    cout << "Failed to add: " << rc << endl;
    return 0;
  }
  rc = o.close(false);
  if (rc) {
    cout << "Failed to close: " << rc << endl;
    return 0;
  }
  cout << "List:" << endl;
  owner_list.show();
  cout << "Dir contents:" << endl;
  system("ls -R test_db");


  cout << endl << "Abort client" << endl;
  rc = o.open(true, false);
  if (rc) {
    cout << "Failed to open: " << rc << endl;
    return 0;
  }
  node = new File(Path("test1/testfile"));
  node->stat();
  list_node = NULL;
  rc = o.search((remote_path + node->name()).c_str(), &list_node);
  delete list_node;
  rc = o.add((remote_path + node->name()).c_str(), node, ++my_time);
  delete node;
  if (rc) {
    cout << "Failed to add: " << rc << endl;
    return 0;
  }
  rc = o.close(true);
  if (rc) {
    cout << "Failed to close: " << rc << endl;
    return 0;
  }
  cout << "List:" << endl;
  owner_list.show();
  cout << "Dir contents:" << endl;
  system("ls -R test_db");


  cout << endl << "Abort backup" << endl;
  rc = o.open(true, false);
  if (rc) {
    cout << "Failed to open: " << rc << endl;
    return 0;
  }
  node = new File(Path("test2/testfile"));
  node->stat();
  list_node = NULL;
  rc = o.search((remote_path + node->name()).c_str(), &list_node);
  delete list_node;
  dynamic_cast<File*>(node)->setChecksum("checksum test");
  rc = o.add((remote_path + node->name()).c_str(), node, ++my_time);
  delete node;
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
  cout << "List:" << endl;
  owner_list.show();
  cout << "Dir contents:" << endl;
  system("ls -R test_db");
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
  cout << "List:" << endl;
  owner_list.show();
  cout << "Dir contents:" << endl;
  system("ls -R test_db");


  cout << endl << "Recover client (empty journal)" << endl;
  {
    List journal("test_db/client/journal");
    journal.open("w");
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
  cout << "List:" << endl;
  owner_list.show();
  cout << "Dir contents:" << endl;
  system("ls -R test_db");


  cout << endl << "Recover client (empty, not closed journal)" << endl;
  {
    List journal("test_db/client/journal2");
    journal.open("w");
    journal.close();
    system("head -1 test_db/client/journal2 > test_db/client/journal");
    system("rm test_db/client/journal2");
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
  cout << "List:" << endl;
  owner_list.show();
  cout << "Dir contents:" << endl;
  system("ls -R test_db");


  cout << endl << "Get checksums" << endl;
  list<string> checksums;
  rc = o.getChecksums(checksums);
  if (rc) {
    cout << "Failed to get checksums: " << rc << endl;
    return 0;
  }
  cout << "List:" << endl;
  if (checksums.empty()) {
    cout << "...is empty" << endl;
  } else
  for (list<string>::iterator i = checksums.begin(); i != checksums.end(); i++) {
    cout << " -> " << *i << endl;
  }
  cout << "Dir contents:" << endl;
  system("ls -R test_db");


  cout << endl << "End of tests" << endl;
  return 0;
}
