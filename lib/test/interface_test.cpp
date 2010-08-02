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
#include <iomanip>
#include <list>

#include <stdio.h>
#include <errno.h>

using namespace std;

#include "test.h"

#include "line.h"
#include "files.h"
#include "configuration.h"
#include "conditions.h"
#include "filters.h"
#include "parsers.h"
#include "opdata.h"
#include "db.h"
#include "attributes.h"
#include "paths.h"
#include "clients.h"

time_t time(time_t* tm) {
  time_t val = 1249617904;
  if (tm != NULL) {
    *tm = val;
  }
  return val;
}

static void copy_progress(long long previous, long long current, long long total) {
  if ((current != total) || (previous != 0)) {
    hlog_verbose_temp("Copy: %5.1lf%%",
      100 * static_cast<double>(current) / static_cast<double>(total));
  }
}

static void list_progress(long long previous, long long current, long long total) {
  if ((current != total) || (previous != 0)) {
    hlog_verbose_temp("List: %5.1lf%%",
      100 * static_cast<double>(current) / static_cast<double>(total));
  }
}

static int showClientConfigs() {
  Directory dir("test_db/.configs");
  if (dir.createList()) {
    cout << "Could not open configs dir" << endl;
    return -1;
  }
  bool failed = false;
  const list<Node*> ls = dir.nodesListConst();
  for (list<Node*>::const_iterator i = ls.begin(); i != ls.end(); i++) {
    cout << "Config for " << (*i)->path() << endl;
    Config config;
    Stream stream((*i)->path());
    if (! stream.open(O_RDONLY)) {
      Line line;
      while (stream.getLine(line) > 0) {
        cout << line << endl;
      }
      stream.close();
    } else {
      cout << "Could not open config file " << stream.path() << endl;
      failed = true;
    }
  }
  return failed ? -1 : 0;
}

int main(void) {
  HBackup*     hbackup;
  list<string> names;
  int          sys_rc;

  report.setLevel(debug);
  hbackup::setCopyProgressCallback(copy_progress);
  hbackup::setListProgressCallback(list_progress);

  cout << endl << "Test: wrong config file" << endl;
  hbackup = new HBackup();
  hbackup->open("");
  delete hbackup;

  cout << endl << "Test: backup, not initialized" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->backup();
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: check config" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf", false, true)) {
    return 1;
  }
  hbackup->show(2);
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
  hbackup->show(2);
  hbackup->backup(true);
  hbackup->close();
  delete hbackup;
  showClientConfigs();

  cout << endl << "Test: same backup" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->backup();
  hbackup->close();
  delete hbackup;
  showClientConfigs();

  cout << endl << "Test: interrupted backup" << endl;
  sys_rc = system("dd if=/dev/zero of=test1/dir\\ space/big_file bs=1k count=500"
    " > /dev/null 2>&1");
  abort(2);
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->backup();
  hbackup->close();
  delete hbackup;
  showClientConfigs();

  abort(2);
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->backup();
  hbackup->close();
  delete hbackup;
  showClientConfigs();
  abort(0xffff);

  {
    Stream s("test_db/.data/6d7fce9fee471194aa8b5b6e47267f03-0/data");
    Stream d("test_db/.data/6d7fce9fee471194aa8b5b6e47267f03-0/data.gz");
    if (! d.open(O_WRONLY, 5, true)) {
      if (! s.open(O_RDONLY)) {
        if (! s.copy(&d) && ! s.close()) {
          s.remove();
          cout << endl << "Test: force-compress DB data" << endl;
        }
      }
      d.close();
    }
    sys_rc = system("echo -n 2 > "
      "test_db/.data/6d7fce9fee471194aa8b5b6e47267f03-0/meta");
  }

  cout << endl << "Test: specify clients" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->addClient("myhost");
  hbackup->addClient("client.xp");
  hbackup->backup();
  hbackup->close();
  delete hbackup;
  showClientConfigs();

  cout << endl << "Test: scan DB" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->scan();
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: check DB" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->check();
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: list clients" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  names.clear();
  hbackup->list(&names);
  for (list<string>::const_iterator i = names.begin(); i != names.end(); ++i) {
    cout << "\t" << *i << endl;
  }
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: list paths in UNIX client 'myClient'" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->addClient("myClient");
  names.clear();
  hbackup->list(&names);
  for (list<string>::const_iterator i = names.begin(); i != names.end(); ++i) {
    cout << "\t" << *i << endl;
  }
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: list paths in DOS client 'client.xp'" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->addClient("client.xp");
  names.clear();
  hbackup->list(&names);
  for (list<string>::const_iterator i = names.begin(); i != names.end(); ++i) {
    cout << "\t" << *i << endl;
  }
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: restore file" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->addClient("myhost");
  hbackup->restore("test_r", HBackup::none, "test2/testfile", 0);
  hbackup->close();
  sys_rc = system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore dir" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->addClient("myhost");
  hbackup->restore("test_r", HBackup::none, "test1", 0);
  hbackup->close();
  // Show test1/testfile's metadata
  File meta("test_r/test1/testfile");
  printf("test1/testfile metadata: gid = %u, mtime = %lu, perms = 0%o\n",
    meta.gid(), meta.mtime(), meta.mode());
  sys_rc = system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore subdir" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->addClient("myhost");
  hbackup->restore("test_r", HBackup::none, "test1/cvs", 0);
  hbackup->close();
  sys_rc = system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore client" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->addClient("myhost");
  hbackup->restore("test_r", HBackup::none, "", 0);
  hbackup->close();
  sys_rc = system("rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: typical backup" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->backup();
  hbackup->close();
  delete hbackup;
  showClientConfigs();

  cout << endl << "Test: specify clients" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->addClient("myClient");
  hbackup->addClient("client.xp");
  hbackup->backup();
  hbackup->close();
  delete hbackup;
  showClientConfigs();

  cout << endl << "Test: user-mode backup" << endl;
  hbackup = new HBackup();
  if (hbackup->open("test_user", true)) {
    return 1;
  }
  hbackup->show(2);
  hbackup->backup(true);
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: list clients" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  names.clear();
  hbackup->list(&names);
  for (list<string>::const_iterator i = names.begin(); i != names.end(); ++i) {
    cout << "\t" << *i << endl;
  }
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: list paths in DOS client 'myClient.xp'" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->addClient("myClient.xp");
  names.clear();
  hbackup->list(&names);
  for (list<string>::const_iterator i = names.begin(); i != names.end(); ++i) {
    cout << "\t" << *i << endl;
  }
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: list paths in dual-boot client 'myClient*'" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  if ((hbackup->addClient("myClient") == 0)
  && (hbackup->addClient("myClient.xp") == 0)) {
    hbackup->show(2);
    names.clear();
    hbackup->list(&names);
    for (list<string>::const_iterator i = names.begin(); i != names.end(); ++i) {
      cout << "\t" << *i << endl;
    }
  }
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: list sub-paths in UNIX client 'myClient'" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->addClient("myClient");
  names.clear();
  hbackup->list(&names, HBackup::none, "/home");
  for (list<string>::const_iterator i = names.begin(); i != names.end(); ++i) {
    cout << "\t" << *i << endl;
  }
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: list sub-paths in DOS client 'myClient.xp', 'C:'"
    << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient.xp");
  hbackup->show(2);
  names.clear();
  hbackup->list(&names, HBackup::none, "C:");
  for (list<string>::const_iterator i = names.begin(); i != names.end(); ++i) {
    cout << "\t" << *i << endl;
  }
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: restore file (UNIX)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::none, "/home/User/test/File2.txt", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore subdir (UNIX)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::none, "/home/User/test", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore dir (UNIX)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::none, "/", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore client (UNIX)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::none, "", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore file (DOS)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient.xp");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::none, "C:/Test Dir/My File.TXT", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore subdir (DOS)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient.xp");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::none, "C:/Test Dir/My Dir", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore dir (DOS)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient.xp");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::none, "C:", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore client (DOS)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient.xp");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::none, "", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore client (dual-boot)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->addClient("myClient.xp");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::none, "", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;


  cout << endl << "Test: restore (symlinks) file (UNIX)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::symbolic, "/home/User/test/File2.txt", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore (symlinks) subdir (UNIX)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::symbolic, "/home/User/test", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore (symlinks) dir (UNIX)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::symbolic, "/", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore (symlinks) client (UNIX)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::symbolic, "", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore (symlinks) file (DOS)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient.xp");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::symbolic, "C:/Test Dir/My File.TXT", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore (symlinks) subdir (DOS)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient.xp");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::symbolic, "C:/Test Dir/My Dir", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore (symlinks) dir (DOS)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient.xp");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::symbolic, "C:", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore (symlinks) client (DOS)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient.xp");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::symbolic, "", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore (symlinks) client (dual-boot)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->addClient("myClient.xp");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::symbolic, "", 0);
  hbackup->close();
  sys_rc = system("ls -l test_r/home/User/test/dir/file3.txt.gz | sed \"s/.*-> //\"");
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;


  cout << endl << "Test: restore (hard links) file (UNIX)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::hard, "/home/User/test/File2.txt", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore (hard links) subdir (UNIX)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::hard, "/home/User/test", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore (hard links) dir (UNIX)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::hard, "/", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore (hard links) client (UNIX)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::hard, "", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore (hard links) file (DOS)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient.xp");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::hard, "C:/Test Dir/My File.TXT", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore (hard links) subdir (DOS)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient.xp");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::hard, "C:/Test Dir/My Dir", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore (hard links) dir (DOS)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient.xp");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::hard, "C:", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore (hard links) client (DOS)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient.xp");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::hard, "", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;

  cout << endl << "Test: restore (hard links) client (dual-boot)" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->addClient("myClient");
  hbackup->addClient("myClient.xp");
  hbackup->show(2);
  hbackup->restore("test_r", HBackup::hard, "", 0);
  hbackup->close();
  sys_rc = system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;


  cout << endl << "Test: check for corrupted data" << endl;
  sys_rc = system("echo > test_db/.data/0f2ea973d77135dc3d06c8e68da6dc59-0/data.gz");
  sys_rc = system("echo > test_db/.data/b90f8fa56ea1d39881d4a199c7a81d35-0/data");
  sys_rc = system("echo > test_db/.data/fef51838cd3cfe8ed96355742eb71fbd-0/data");
  sys_rc = system("rm -f test_db/.data/59ca0efa9f5633cb0371bbc0355478d8-0/data");
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  cout << "Check" << endl;
  hbackup->check();
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: check for corrupted data again" << endl;
  sys_rc = system("echo > test_db/.data/0f2ea973d77135dc3d06c8e68da6dc59-0/data.gz");
  sys_rc = system("echo > test_db/.data/b90f8fa56ea1d39881d4a199c7a81d35-0/data");
  sys_rc = system("echo > test_db/.data/fef51838cd3cfe8ed96355742eb71fbd-0/data");
  sys_rc = system("sed -i \"s/644\t[^\t]*\\(.\\)$/644\td41d8cd98f00b204e9800998ecf8427e-0\\1/\" test_db/myClient/list");
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  cout << "Check" << endl;
  hbackup->check();
  hbackup->close();
  delete hbackup;


  cout << endl << "Test: scan for missing data" << endl;
  sys_rc = system("rm -f test_db/.data/0f2ea973d77135dc3d06c8e68da6dc59-0/data.gz");
  sys_rc = system("rm -f test_db/.data/b90f8fa56ea1d39881d4a199c7a81d35-0/data.gz");
  sys_rc = system("rm -f test_db/.data/fef51838cd3cfe8ed96355742eb71fbd-0/data.gz");
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  cout << "Just scan" << endl;
  hbackup->scan(false);
  cout << "Just scan again" << endl;
  hbackup->scan(false);
  hbackup->close();
  delete hbackup;


  cout << endl << "Test: scan for obsolete data, case1" << endl;
  sys_rc = system("mkdir test_db/.data/00000000000000000000000000000000-0");
  sys_rc = system("touch test_db/.data/00000000000000000000000000000000-0/data");
  sys_rc = system("echo 1 > test_db/.data/00000000000000000000000000000000-0/meta");
  sys_rc = system("mkdir test_db/.data/88888888888888888888888888888888-0");
  sys_rc = system("touch test_db/.data/88888888888888888888888888888888-0/data");
  sys_rc = system("echo 1000 > test_db/.data/88888888888888888888888888888888-0/meta");
  sys_rc = system("mkdir test_db/.data/ffffffffffffffffffffffffffffffff-0");
  sys_rc = system("touch test_db/.data/ffffffffffffffffffffffffffffffff-0/data");
  sys_rc = system("echo 1000000 > test_db/.data/ffffffffffffffffffffffffffffffff-0/meta");
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  cout << "Just scan" << endl;
  hbackup->scan(false);
  cout << "Scan and remove" << endl;
  hbackup->scan(true);
  cout << "Just scan again" << endl;
  hbackup->scan(false);
  hbackup->close();
  delete hbackup;


  cout << endl << "Test: scan for obsolete data, case 2" << endl;
  sys_rc = system("mkdir test_db/.data/33333333333333333333333333333333-0");
  sys_rc = system("touch test_db/.data/33333333333333333333333333333333-0/data");
  sys_rc = system("echo 3 > test_db/.data/33333333333333333333333333333333-0/meta");
  sys_rc = system("mkdir test_db/.data/77777777777777777777777777777777-0");
  sys_rc = system("touch test_db/.data/77777777777777777777777777777777-0/data");
  sys_rc = system("echo 7 > test_db/.data/77777777777777777777777777777777-0/meta");
  sys_rc = system("mkdir test_db/.data/dddddddddddddddddddddddddddddddd-0");
  sys_rc = system("touch test_db/.data/dddddddddddddddddddddddddddddddd-0/data");
  sys_rc = system("echo d > test_db/.data/dddddddddddddddddddddddddddddddd-0/data");
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  cout << "Just scan" << endl;
  hbackup->scan(false);
  cout << "Scan and remove" << endl;
  hbackup->scan(true);
  cout << "Just scan again" << endl;
  hbackup->scan(false);
  hbackup->close();
  delete hbackup;


  cout << endl << "Test: backup recovers broken checksums + replace data"
    << endl;
  // Replace with compressed data
  sys_rc = system("touch test2/testfile2");
  sys_rc = system("gunzip test_db/.data/fb00cd74a5f35e89a7fbdd3c1d05375a-0/data.gz");
  // Wrong gzip format, data should be flat => conflict
  sys_rc = system("gzip test_db/.data/5252f242d27b8c2c9fdbdcbb33545d07-0/data");
  sys_rc = system("rm test_db/.data/5252f242d27b8c2c9fdbdcbb33545d07-0/meta");
  // Wrong gzip format, data should be compressed => conflict
  sys_rc = system("gunzip test_db/.data/816df6f64deba63b029ca19d880ee10a-0/data.gz");
  sys_rc = system("gzip test_db/.data/816df6f64deba63b029ca19d880ee10a-0/data");
  remove("test1/testfile");
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->backup();
  hbackup->close();
  delete hbackup;
  sys_rc = system("ls test_db/.data/6d7fce9fee471194aa8b5b6e47267f03-0");
  showClientConfigs();

  cout << endl << "Test: second backup should not recover again" << endl;
  // Wrong gzip format and corrupted => what?
  sys_rc = system("gzip test2/testfile -c > test_db/.data/fb00cd74a5f35e89a7fbdd3c1d05375a-0/data");
  sys_rc = system("echo 185 > test_db/.data/fb00cd74a5f35e89a7fbdd3c1d05375a-0/meta");
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->backup();
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: scan shows recovered data" << endl;
  // Scan should show the recovered data
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->scan();
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: check finds corrupted data" << endl;
  // Check should find out about the corrupted data
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->check();
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: scan removes corrupted data" << endl;
  // Scan should remove the corrupted data
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->scan();
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: third backup should not recover again" << endl;
  // Again, back up does not need to recover again
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->backup();
  hbackup->close();
  delete hbackup;
  showClientConfigs();

  cout << endl << "Test: declared path missing" << endl;
  sys_rc = system("rm -rf test_cifs/Test*");
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->addClient("aClient.vista");
  hbackup->backup();
  hbackup->close();
  delete hbackup;
  showClientConfigs();

  int _unused;
  _unused = system("for name in `ls hbackup.log*`; do "
                   "echo \"$name:\"; cat $name; done");

  return 0;
}
