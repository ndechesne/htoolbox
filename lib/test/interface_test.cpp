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
#include <iomanip>
#include <list>
#include <errno.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "report.h"
#include "conditions.h"
#include "filters.h"
#include "parsers.h"
#include "db.h"
#include "paths.h"
#include "clients.h"

using namespace hbackup;

static void progress(long long previous, long long current, long long total) {
  if (current < total) {
    cout << "Done: " << setw(5) << setiosflags(ios::fixed) << setprecision(1)
      << 100.0 * current /total << "%\r" << flush;
  } else if (previous != 0) {
    cout << "            \r";
  }
}

int main(void) {
  HBackup* hbackup;

  setVerbosityLevel(debug);
  hbackup::setProgressCallback(progress);

  cout << endl << "Test: wrong config file" << endl;
  hbackup = new HBackup();
  hbackup->open("");
  delete hbackup;

  cout << endl << "Test: backup, not initialized" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->backup();
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: check config" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf", false, true)) {
    return 1;
  }
  if (hbackup->open("test_user", true, true)) {
    return 1;
  }
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: typical backup" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->backup(true);
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: same backup" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->backup();
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: interrupted backup" << endl;
  system("dd if=/dev/zero of=test1/dir\\ space/big_file bs=1k count=500"
    " > /dev/null 2>&1");
  abort(2);
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->backup();
  hbackup->close();
  delete hbackup;
  abort(0xffff);

  cout << endl << "Test: specify clients" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myhost");
  hbackup->addClient("client.xp");
  hbackup->backup();
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: scan DB" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->scan();
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: check DB" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->check();
  hbackup->close();
  delete hbackup;

  list<string> records;

  cout << endl << "Test: list clients" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->getList(records);
  hbackup->close();
  delete hbackup;
  cout << "List of clients: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: list paths in UNIX client" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->getList(records);
  hbackup->close();
  delete hbackup;
  cout << "List of paths in 'myClient': " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: list paths in DOS client" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("client.xp");
  hbackup->getList(records);
  hbackup->close();
  delete hbackup;
  cout << "List of paths in 'client.xp': " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: restore file" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myhost");
  hbackup->restore("test_r", "test2/testfile", 0);
  hbackup->close();
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore dir" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myhost");
  hbackup->restore("test_r", "test1", 0);
  hbackup->close();
  // Show test1/testfile's metadata
  File meta("test_r/test1/testfile");
  printf("test1/testfile metadata: gid = %u, mtime = %lu, perms = 0%o\n",
    meta.gid(), meta.mtime(), meta.mode());
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore subdir" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myhost");
  hbackup->restore("test_r", "test1/cvs", 0);
  hbackup->close();
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore client" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myhost");
  hbackup->restore("test_r", "", 0);
  hbackup->close();
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: add dual-boot client" << endl;
  system("echo >> etc/hbackup.conf");
  system("echo client myClient xp >> etc/hbackup.conf");
  system("echo protocol smb>> etc/hbackup.conf");
  system("echo config C:\\\\Backup\\\\Backup.LST >> etc/hbackup.conf");

  cout << endl << "Test: typical backup" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->backup();
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: specify clients" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->addClient("client.xp");
  hbackup->backup();
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: user-mode backup" << endl;
  hbackup = new HBackup();
  if (hbackup->open("test_user", true)) {
    return 1;
  }
  hbackup->backup(true);
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: list clients" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->getList(records);
  hbackup->close();
  delete hbackup;
  cout << "List of clients: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: list paths in DOS client" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient.xp");
  hbackup->getList(records);
  hbackup->close();
  delete hbackup;
  cout << "List of paths in 'myClient.xp': " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: list paths in dual-boot client" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  if (hbackup->addClient("myClient*") == 0) {
    hbackup->getList(records);
    cout << "List of paths in 'myClient*': " << records.size() << endl;
    for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
      cout << " -> " << *i << endl;
    }
    records.clear();
  }
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: list sub-paths in UNIX client" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->getList(records, "/home");
  hbackup->close();
  delete hbackup;
  cout << "List of paths in 'myClient': " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: list sub-paths in DOS client" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient.xp");
  hbackup->getList(records, "C:");
  hbackup->close();
  delete hbackup;
  cout << "List of paths in 'myClient.xp', 'C:': " << records.size()
    << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " -> " << *i << endl;
  }
  records.clear();

  cout << endl << "Test: restore file (UNIX)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->restore("test_r", "/home/User/test/File2.txt", 0);
  hbackup->close();
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore subdir (UNIX)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->restore("test_r", "/home/User/test", 0);
  hbackup->close();
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore dir (UNIX)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->restore("test_r", "/", 0);
  hbackup->close();
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore client (UNIX)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->restore("test_r", "", 0);
  hbackup->close();
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore file (DOS)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient.xp");
  hbackup->restore("test_r", "C:/Test Dir/My File.TXT", 0);
  hbackup->close();
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore subdir (DOS)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient.xp");
  hbackup->restore("test_r", "C:/Test Dir/My Dir", 0);
  hbackup->close();
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore dir (DOS)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient.xp");
  hbackup->restore("test_r", "C:", 0);
  hbackup->close();
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore client (DOS)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient.xp");
  hbackup->restore("test_r", "", 0);
  hbackup->close();
  system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore client (dual-boot)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient*");
  hbackup->restore("test_r", "", 0);
  hbackup->close();
  system("rm -rf test_r");
  delete hbackup;


  cout << endl << "Test: check for corrupted data" << endl;
  system("echo > test_db/.data/0f2ea973d77135dc3d06c8e68da6dc59-0/data.gz");
  system("echo > test_db/.data/b90f8fa56ea1d39881d4a199c7a81d35-0/data");
  system("echo > test_db/.data/fef51838cd3cfe8ed96355742eb71fbd-0/data");
  system("rm -f test_db/.data/59ca0efa9f5633cb0371bbc0355478d8-0/data.gz");
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  cout << "Check" << endl;
  hbackup->check();
  hbackup->close();
  delete hbackup;


  cout << endl << "Test: scan for missing data" << endl;
  system("rm -f test_db/.data/0f2ea973d77135dc3d06c8e68da6dc59-0/data.gz");
  system("rm -f test_db/.data/b90f8fa56ea1d39881d4a199c7a81d35-0/data.gz");
  system("rm -f test_db/.data/fef51838cd3cfe8ed96355742eb71fbd-0/data.gz");
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  cout << "Just scan" << endl;
  hbackup->scan(false);
  cout << "Just scan again" << endl;
  hbackup->scan(false);
  hbackup->close();
  delete hbackup;


  cout << endl << "Test: scan for obsolete data, case1" << endl;
  system("mkdir test_db/.data/00000000000000000000000000000000-0");
  system("touch test_db/.data/00000000000000000000000000000000-0/data");
  system("mkdir test_db/.data/88888888888888888888888888888888-0");
  system("touch test_db/.data/88888888888888888888888888888888-0/data");
  system("mkdir test_db/.data/ffffffffffffffffffffffffffffffff-0");
  system("touch test_db/.data/ffffffffffffffffffffffffffffffff-0/data");
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  cout << "Just scan" << endl;
  hbackup->scan(false);
  cout << "Scan and remove" << endl;
  hbackup->scan(true);
  cout << "Just scan again" << endl;
  hbackup->scan(false);
  hbackup->close();
  delete hbackup;


  cout << endl << "Test: scan for obsolete data, case 2" << endl;
  system("mkdir test_db/.data/33333333333333333333333333333333-0");
  system("touch test_db/.data/33333333333333333333333333333333-0/data");
  system("mkdir test_db/.data/77777777777777777777777777777777-0");
  system("touch test_db/.data/77777777777777777777777777777777-0/data");
  system("mkdir test_db/.data/dddddddddddddddddddddddddddddddd-0");
  system("touch test_db/.data/dddddddddddddddddddddddddddddddd-0/data");
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  cout << "Just scan" << endl;
  hbackup->scan(false);
  cout << "Scan and remove" << endl;
  hbackup->scan(true);
  cout << "Just scan again" << endl;
  hbackup->scan(false);
  hbackup->close();
  delete hbackup;


  cout << endl << "Test: backup recovers missing checksums" << endl;
  remove("test1/testfile");
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->backup();
  hbackup->close();
  delete hbackup;
  // Second backup should not recover again
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->backup();
  hbackup->close();
  delete hbackup;
  // Scan should show the recovered data
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->scan();
  hbackup->close();
  delete hbackup;
  // Again, back up does not need to recover again
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->backup();
  hbackup->close();
  delete hbackup;

  return 0;
}
