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
#include <errno.h>

using namespace std;

#include "strings.h"
#include "files.h"
#include "conditions.h"
#include "filters.h"
#include "parsers.h"
#include "list.h"
#include "db.h"
#include "paths.h"
#include "clients.h"
#include "hbackup.h"

using namespace hbackup;

int hbackup::verbosity(void) {
  return 3;
}

int hbackup::terminating(const char* unused) {
  return 0;
}

int main(void) {
  HBackup* hbackup;

#if 0
  cout << endl << "Test: wrong config file" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("")) {
    return 1;
  }
  delete hbackup;
#endif

  cout << endl << "Test: typical backup" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->backup();
  delete hbackup;

  cout << endl << "Test: specify clients" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myhost");
  hbackup->addClient("client");
  hbackup->backup();
  delete hbackup;

  cout << endl << "Test: scan DB" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->check();
  delete hbackup;

  cout << endl << "Test: check DB" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->check(true);
  delete hbackup;

#if 0
  cout << endl << "Test: list prefixes" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  // TODO
  delete hbackup;
#endif

  cout << endl << "Test: restore file" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->restore("test_r", "file://myhost", "test2/testfile", 0);
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore dir" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->restore("test_r", "file://myhost", "test1", 0);
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore subdir" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->restore("test_r", "file://myhost", "test1/cvs", 0);
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: prefix" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->restore("test_r", "file://myhost", "", 0);
  system("rm -rf test_r");
  delete hbackup;

  return 0;
}
