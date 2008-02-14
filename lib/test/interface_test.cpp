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
#include <list>
#include <errno.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "report.h"
#include "conditions.h"
#include "filters.h"
#include "parsers.h"
#include "list.h"
#include "db.h"
#include "paths.h"
#include "clients.h"

using namespace hbackup;

static bool killed  = false;
static bool killall = false;

int hbackup::terminating(const char* function) {
  if (killall) {
    return 1;
  }
  if (killed && (function != NULL) && (! strcmp(function, "write"))) {
    cout << "Killing during " << function << endl;
    killall = true;
    return 1;
  }
  return 0;
}

int main(void) {
  HBackup* hbackup;

  setVerbosityLevel(debug);

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

  cout << endl << "Test: same backup" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->backup();
  delete hbackup;

  cout << endl << "Test: interrupted backup" << endl;
  system("dd if=/dev/zero of=test1/dir\\ space/big_file bs=1k count=500"
    " > /dev/null 2>&1");
  killed = true;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->backup();
  delete hbackup;
  killed = false;
  killall = false;

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

  list<string> records;

  cout << endl << "Test: list clients" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->getList(records);
  delete hbackup;
  cout << "List of clients: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: list paths in UNIX client" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->getList(records, "myClient");
  delete hbackup;
  cout << "List of paths in 'myClient': " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: list paths in DOS client" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->getList(records, "client");
  delete hbackup;
  cout << "List of paths in 'client': " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: restore file" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->restore("test_r", "myhost", "test2/testfile", 0);
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore dir" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->restore("test_r", "myhost", "test1", 0);
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore subdir" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->restore("test_r", "myhost", "test1/cvs", 0);
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore client" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->restore("test_r", "myhost", "", 0);
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: add dual-boot client" << endl;
  system("echo >> etc/hbackup.conf");
  system("echo client smb myClient >> etc/hbackup.conf");
  system("echo config C:\\\\Backup\\\\Backup.LST >> etc/hbackup.conf");

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
  hbackup->addClient("myClient");
  hbackup->addClient("client");
  hbackup->backup();
  delete hbackup;

  cout << endl << "Test: user-mode backup" << endl;
  hbackup = new HBackup();
  if (hbackup->setUserPath("test_user")) {
    return 1;
  }
  hbackup->backup();
  delete hbackup;

  cout << endl << "Test: list clients" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->getList(records);
  delete hbackup;
  cout << "List of clients: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: list paths in DUAL client" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->getList(records, "myClient");
  delete hbackup;
  cout << "List of paths in 'myClient': " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: list sub-paths in UNIX client" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->getList(records, "myClient", "/home");
  delete hbackup;
  cout << "List of paths in 'myClient': " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: list sub-paths in DOS client" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->getList(records, "myClient", "C:");
  delete hbackup;
  cout << "List of paths in 'myClient': " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: restore file (UNIX)" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->restore("test_r", "myClient", "/home/User/test/File2.txt", 0);
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore subdir (UNIX)" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->restore("test_r", "myClient", "/home/User/test", 0);
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore dir (UNIX)" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->restore("test_r", "myClient", "/", 0);
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore file (DOS)" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->restore("test_r", "myClient", "C:/Test/File.TXT", 0);
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore subdir (DOS)" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->restore("test_r", "myClient", "C:/Test", 0);
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore dir (DOS)" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->restore("test_r", "myClient", "C:", 0);
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore client" << endl;
  hbackup = new HBackup();
  if (hbackup->readConfig("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->restore("test_r", "myClient", "", 0);
  system("rm -rf test_r");
  delete hbackup;

  return 0;
}
