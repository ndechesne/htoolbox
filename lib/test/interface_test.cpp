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
#include "opdata.h"
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

  cout << endl << "Test: same backup" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
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
  hbackup->show(2);
  hbackup->backup();
  hbackup->close();
  delete hbackup;

  abort(2);
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->backup();
  hbackup->close();
  delete hbackup;
  abort(0xffff);

  {
    Stream s("test_db/.data/6d7fce9fee471194aa8b5b6e47267f03-0/data");
    Stream d("test_db/.data/6d7fce9fee471194aa8b5b6e47267f03-0/data.gz");
    if (! d.open("w", 5, -1, true, s.size())) {
      if (! s.open("r")) {
        if (! s.copy(&d) && ! s.close()) {
          s.remove();
          cout << endl << "Test: force-compress DB data" << endl;
        }
      }
      d.close();
    }
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
  hbackup->restore();
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: list paths in UNIX client 'myClient'" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->addClient("myClient");
  hbackup->restore();
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: list paths in DOS client 'client.xp'" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->addClient("client.xp");
  hbackup->restore();
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
  system("rm -rf test_r");
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
  system("rm -rf test_r");
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
  system("rm -rf test_r");
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
  system("rm -rf test_r");
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
  hbackup->restore();
  hbackup->close();
  delete hbackup;

  cout << endl << "Test: list paths in DOS client 'myClient.xp'" << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->addClient("myClient.xp");
  hbackup->restore();
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
    hbackup->restore();
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
  hbackup->restore(NULL, HBackup::none, "/home");
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
  hbackup->restore(NULL, HBackup::none, "C:");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("ls -l test_r/home/User/test/dir/file3.txt.gz | sed \"s/.*-> //\"");
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
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
  system("find test_r -printf '%y\t%p\n'  | sort -k2; rm -rf test_r");
  delete hbackup;


  cout << endl << "Test: check for corrupted data" << endl;
  system("echo > test_db/.data/0f2ea973d77135dc3d06c8e68da6dc59-0/data");
  system("echo > test_db/.data/b90f8fa56ea1d39881d4a199c7a81d35-0/data");
  system("echo > test_db/.data/fef51838cd3cfe8ed96355742eb71fbd-0/data");
  system("rm -f test_db/.data/59ca0efa9f5633cb0371bbc0355478d8-0/data");
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
  system("echo > test_db/.data/0f2ea973d77135dc3d06c8e68da6dc59-0/data");
  system("echo > test_db/.data/b90f8fa56ea1d39881d4a199c7a81d35-0/data");
  system("echo > test_db/.data/fef51838cd3cfe8ed96355742eb71fbd-0/data");
  system("rm -f test_db/.data/59ca0efa9f5633cb0371bbc0355478d8-0/data.gz");
  system("sed -i \"s/644\t[^\t]*$/644\td41d8cd98f00b204e9800998ecf8427e-0/\" test_db/myClient/list");
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
  system("rm -f test_db/.data/0f2ea973d77135dc3d06c8e68da6dc59-0/data");
  system("rm -f test_db/.data/b90f8fa56ea1d39881d4a199c7a81d35-0/data.gz");
  system("rm -f test_db/.data/fef51838cd3cfe8ed96355742eb71fbd-0/data.gz");
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
  system("touch test2/testfile2");
  system("gunzip test_db/.data/fb00cd74a5f35e89a7fbdd3c1d05375a-0/data.gz");
  // Wrong gzip format, data should be flat => conflict
  system("gzip test_db/.data/5252f242d27b8c2c9fdbdcbb33545d07-0/data");
  // Wrong gzip format, data should be compressed => conflict
  system("gunzip test_db/.data/816df6f64deba63b029ca19d880ee10a-0/data.gz");
  system("gzip test_db/.data/816df6f64deba63b029ca19d880ee10a-0/data");
  remove("test1/testfile");
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->backup();
  hbackup->close();
  delete hbackup;
  system("ls test_db/.data/6d7fce9fee471194aa8b5b6e47267f03-0");

  cout << endl << "Test: second backup should not recover again"
    << endl;
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->backup();
  hbackup->close();
  delete hbackup;
  // Scan should show the recovered data
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->scan();
  hbackup->close();
  delete hbackup;
  // Again, back up does not need to recover again
  hbackup = new HBackup();
  if (hbackup->open("etc/hbackup.conf")) {
    return 1;
  }
  hbackup->show(2);
  hbackup->backup();
  hbackup->close();
  delete hbackup;

  return 0;
}
