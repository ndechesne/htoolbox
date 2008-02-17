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

#include <sys/types.h>
#include <sys/stat.h>

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

using namespace hbackup;

static bool killed  = false;
static bool killall = false;

int hbackup::terminating(const char* module) {
  if (killall) {
    return 1;
  }
  if (killed && (module != NULL) && (! strcmp(module, "data"))) {
    cout << "Killing inside " << module << endl;
    killall = true;
    return 1;
  }
  return 0;
}

static time_t my_time = 0;
time_t time(time_t *t) {
  return 2000000000 + (my_time * 24 * 3600);
}

static void showLine(
    time_t          timestamp,
    const char*     client,
    const char*     path,
    const Node*     node) {
  printf("[%2ld]", (timestamp == 0)? 0 : (timestamp - 2000000000) / 24 / 3600);
  if (node != NULL) {
    printf(" %c %5llu %03o", node->type(), node->size(), node->mode());
    if (node->type() == 'f') {
      printf(" %s", ((File*) node)->checksum());
    }
    if (node->type() == 'l') {
      printf(" %s", ((Link*) node)->link());
    }
  } else {
    printf(" -");
  }
  cout << endl;
}

static void showList(List& slist) {
  if (! slist.open("r")) {
    time_t ts;
    char*  client = NULL;
    char*  path   = NULL;
    Node*  node   = NULL;
    string last_client;
    string last_path;

    while (slist.getEntry(&ts, &client, &path, &node) > 0) {
      if (last_client != client) {
        cout << client << endl;
        last_client = client;
      }
      if (last_path != path) {
        cout << "\t" << path << endl;
        last_path = path;
      }
      showLine(ts, client, path, node);
    }
    if (node != NULL) {
      showLine(ts, "", "", node);
    }
    free(client);
    free(path);
    free(node);
    slist.close();
  } else {
    cerr << "Failed to open list" << endl;
  }
}

int main(void) {
  umask(0022);
  setVerbosityLevel(debug);

  ClientPath* path = new ClientPath("/home/User");
  Database    db("test_db");
  // Journal
  List real_journal(Path("test_db", "journal"));
  List journal(Path("test_db", "journal~"));
  List dblist(Path("test_db", "list"));
  // Filter
  Filter* bazaar = path->addFilter("and", "bazaar");
  if ((bazaar == NULL)
   || bazaar->add("type", "d", false)
   || bazaar->add("path", "bzr", false)) {
    cout << "Failed to add filter" << endl;
  }
  path->setIgnore(bazaar);

  // Initialisation
  cout << endl << "Initial backup, check '/' precedence" << endl;
  my_time++;
  db.open_rw();

  // '-' is before '/' in the ASCII table...
  system("touch test1/subdir-file");
  system("touch test1/subdirfile");
  system("touch test1/àccénts_test");

  cout << "first with subdir/testfile NOT readable" << endl;
  system("chmod 000 test1/subdir/testfile");
  db.setClient("myClient");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }
  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);
  // Initialize list of missing checksums
  if (! db.open_rw()) {
    db.scan();
    db.close();
  }

  // Next test
  cout << endl << "As previous, with a .hbackup directory" << endl;
  my_time++;
  db.open_rw();

  mkdir("test1/.hbackup", 0755);
  db.setClient("myClient");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  // Next test
  cout << endl << "As previous with new big_file, interrupted" << endl;
  my_time++;
  db.open_rw();

  system("dd if=/dev/zero of=test1/cvs/big_file bs=1k count=500"
    " > /dev/null 2>&1");
  killed = true;
  db.setClient("myClient");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(real_journal);

  // Next test
  cout << endl << "As previous with subdir/testfile readable" << endl;
  my_time++;
  db.open_rw();

  killed  = false;
  killall = false;
  system("chmod 644 test1/subdir/testfile");
  db.setClient("myClient");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  // Next test
  cout << endl << "As previous with subdir/testfile in ignore list" << endl;
  my_time++;
  db.open_rw();

  Filter* testfile = path->addFilter("and", "testfile");
  if ((testfile == NULL)
   || testfile->add("type", "file", false)
   || testfile->add("path", "subdir/testfile", false)) {
    cout << "Failed to add filter" << endl;
  }
  Filter* filter = path->addFilter("or", "ignore1");
  if (filter == NULL) {
    cout << "Failed to add 'or' filter" << endl;
  }
  filter->add(new Condition(Condition::subfilter, bazaar, false));
  filter->add(new Condition(Condition::subfilter, testfile, false));
  path->setIgnore(filter);
  db.setClient("myClient");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  // Next test
  cout << endl << "As previous with subdir in ignore list" << endl;
  my_time++;
  db.open_rw();

  Filter* subdir = path->addFilter("and", "subdir");
  if ((subdir == NULL)
   || subdir->add("type", "dir", false)
   || subdir->add("path", "subdir", false)) {
    cout << "Failed to add subdir 'and' filter" << endl;
  }
  filter = path->addFilter("or", "ignore1");
  if (filter == NULL) {
    cout << "Failed to add 'or' filter" << endl;
  }
  filter->add(new Condition(Condition::subfilter, bazaar, false));
  filter->add(new Condition(Condition::subfilter, subdir, false));
  path->setIgnore(filter);
  db.setClient("myClient");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  // Next test
  cout << endl << "As previous with testlink modified" << endl;
  my_time++;
  db.open_rw();

  system("sleep 1 && ln -sf testnull test1/testlink");
  db.setClient("myClient");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  // Next test
  cout << endl << "As previous with testlink in ignore list" << endl;
  my_time++;
  db.open_rw();

  Filter* testlink = path->addFilter("and", "testlink");
  if ((testlink == NULL)
   || testlink->add("type", "link", false)
   || testlink->add("path_start", "testlink", false)) {
    cout << "Failed to add testlink 'and' filter" << endl;
  }
  filter = path->addFilter("or", "ignore1");
  if (filter == NULL) {
    cout << "Failed to add 'or' filter" << endl;
  }
  filter->add(new Condition(Condition::subfilter, bazaar, false));
  filter->add(new Condition(Condition::subfilter, subdir, false));
  filter->add(new Condition(Condition::subfilter, testlink, false));
  path->setIgnore(filter);
  db.setClient("myClient");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  // Next test
  cout << endl << "As previous with CVS parser (controlled)" << endl;
  my_time++;
  db.open_rw();

  if (path->addParser("cvs", "controlled")) {
    cout << "Failed to add CVS parser" << endl;
  }
  db.setClient("myClient");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  // Next test
  cout << endl << "As previous" << endl;
  my_time++;
  db.open_rw();

  db.setClient("myClient");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  // Next test
  cout << endl << "As previous with Subversion parser (modified)" << endl;
  my_time++;
  db.open_rw();

  if (path->addParser("cvs", "controlled")) {
    cout << "Failed to add CVS parser" << endl;
  }
  if (path->addParser("svn", "modified")) {
    cout << "Failed to add Subversion parser" << endl;
  }
  db.setClient("myClient");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  // Next test
  cout << endl << "As previous with cvs/dirutd in ignore list" << endl;
  my_time++;
  db.open_rw();

  Filter* cvs_dirutd = path->addFilter("and", "cvs_dirutd");
  if ((cvs_dirutd == NULL)
   || cvs_dirutd->add("type", "dir", false)
   || cvs_dirutd->add("path", "cvs/dirutd", false)) {
    cout << "Failed to add 'and' filter" << endl;
  }
  Filter* empty = path->addFilter("or", "empty");
  filter = path->addFilter("or", "ignore2");
  if (filter == NULL) {
    cout << "Failed to add 'or' filter" << endl;
  }
  filter->add(new Condition(Condition::subfilter, bazaar, false));
  filter->add(new Condition(Condition::subfilter, subdir, false));
  filter->add(new Condition(Condition::subfilter, testlink, false));
  filter->add(new Condition(Condition::subfilter, cvs_dirutd, false));
  filter->add(new Condition(Condition::subfilter, empty, false));
  path->setIgnore(filter);
  db.setClient("myClient");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  // Next test
  cout << endl << "As previous with svn/dirutd in ignore list" << endl;
  my_time++;
  db.open_rw();

  Filter* svn_dirutd = path->addFilter("and", "svn_dirutd");
  if ((svn_dirutd == NULL)
   || svn_dirutd->add("type", "dir", false)
   || svn_dirutd->add("path", "svn/dirutd", false)) {
    cout << "Failed to add 'and' filter" << endl;
  }
  filter->add(new Condition(Condition::subfilter, bazaar, false));
  filter->add(new Condition(Condition::subfilter, svn_dirutd, false));
  db.setClient("myClient");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  // Next test
  cout << endl << "As previous with testpipe gone" << endl;
  my_time++;
  db.open_rw();

  rename("test1/testpipe", "testpipe");
  db.setClient("myClient");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  // Next test
  cout << endl << "As previous with testfile mode changed" << endl;
  my_time++;
  db.open_rw();

  system("chmod 660 test1/testfile");
  db.setClient("myClient");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  // Next test
  cout << endl << "As previous with cvs/filenew.c touched and testpipe restored" << endl;
  my_time++;
  db.open_rw();

   // Also test that we behave correctly when dir exists but is empty
  system("echo blah > test1/cvs/filenew.c");
  mkdir("test_db/data/0d599f0ec05c3bda8c3b8a68c32a1b47-0", 0755);

  rename("testpipe", "test1/testpipe");
  db.setClient("myClient");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  // Next test
  cout << endl << "Some troublesome past cases" << endl;
  my_time++;
  db.open_rw();

  system("mkdir -p test1/docbook-xml/3.1.7");
  system("touch test1/docbook-xml/3.1.7/dbgenent.ent");
  system("mkdir test1/docbook-xml/4.0");
  system("touch test1/docbook-xml/4.0/dbgenent.ent");
  system("mkdir test1/docbook-xml/4.1.2");
  system("touch test1/docbook-xml/4.1.2/dbgenent.mod");
  system("mkdir test1/docbook-xml/4.2");
  system("touch test1/docbook-xml/4.2/dbgenent.mod");
  system("mkdir test1/docbook-xml/4.3");
  system("touch test1/docbook-xml/4.3/dbgenent.mod");
  system("mkdir test1/docbook-xml/4.4");
  system("touch test1/docbook-xml/4.4/dbgenent.mod");
  system("touch test1/docbook-xml.cat");
  system("touch test1/docbook-xml.cat.old");

  system("mkdir -p test1/testdir/biblio");
  system("touch test1/testdir/biblio/biblio.dbf");
  system("touch test1/testdir/biblio/biblio.dbt");
  system("touch test1/testdir/biblio.odb");
  system("touch test1/testdir/evolocal.odb");

  db.setClient("myClient");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  // Next test
  cout << endl << "Same again: journal and list left untouched" << endl;
  my_time++;
  db.open_rw();

  db.setClient("myClient");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  // Next test
  cout << endl << "List recovery after crash" << endl;
  my_time++;
  system("touch \"test1/test space\"");
  system("mkdir test1/crash");
  system("touch test1/crash/file");
  system("rm -rf test1/testdir");
  db.open_rw();

  db.setClient("hisClient");
  if (! path->parse(db, "test2")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.setClient("myClient");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  // Save files to test recovery
  system("cp test_db/list test_db/list.cp");
  system("cp test_db/journal test_db/journal.cp");

  if (db.close()) {
    return 0;
  }

  // Replace files to test recovery
  rename("test_db/list.cp", "test_db/list");
  rename("test_db/journal.cp", "test_db/journal");

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(real_journal);

  // Recover now
  db.open_rw();

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);
  if (db.close()) {
    return 0;
  }


  db.open_rw();

  if (db.close()) {
    return 0;
  }


  // Next test
  cout << endl << "Same again" << endl;
  my_time++;
  db.open_rw();

  db.setClient("myClient");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  // Next test
  cout << endl << "Same again" << endl;
  my_time++;
  db.open_rw();

  db.setClient("hisClient");
  if (! path->parse(db, "test2")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  // Next test
  my_time++;
  cout << endl << "Expiry delay: 18, date: " << my_time << endl;
  db.open_rw();

  system("touch test1/testfile");
  system("chmod 0 test1/testfile");
  // Make removal complain
  system("rm -r test_db/data/285b35198a5e188b3a0df3ed33f93a26-0");
  db.setClient("myClient", 18 * 24 * 3600);
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }
  system("du -sb test_db/data/trash/*");

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  // Next test
  cout << endl << "Prevent data dir from being removed" << endl;
  my_time++;
  db.open_rw();

  system("touch test1/testfile~");
  system("rm -f test1/testfile");
  // Make removal fail
  system("chmod 0 test_db/data/59ca0efa9f5633cb0371bbc0355478d8-0");
  db.setClient("myClient", 0);
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  int rc = db.close();
  system("chmod 755 test_db/data/59ca0efa9f5633cb0371bbc0355478d8-0");
  if (rc) {
    return 0;
  }
  system("du -sb test_db/data/trash/*");

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  // Next test
  cout << endl << "Add one file" << endl;
  my_time++;
  db.open_rw();

  system("echo blah > test1/testfile~");
  db.setClient("myClient", 0);
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  if (db.close()) {
    return 0;
  }
  system("du -sb test_db/data/trash/*");

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);

  delete path;
  return 0;
}
