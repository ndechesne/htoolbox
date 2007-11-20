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

#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

#include "strings.h"
#include "files.h"
#include "conditions.h"
#include "filters.h"
#include "parsers.h"
#include "cvs_parser.h"
#include "list.h"
#include "db.h"
#include "paths.h"
#include "hbackup.h"

using namespace hbackup;

static bool killed  = false;
static bool killall = false;

int hbackup::verbosity(void) {
  return 4;
}

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

static time_t my_time = 0;
time_t time(time_t *t) {
  return my_time * 24 * 3600;
}

static void showLine(time_t timestamp, Node* node) {
  printf("[%2ld]", timestamp / 24 / 3600);
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
    while ((slist.getLine() > 0) && (slist.lineStatus() > 0)) {
      const char* line = slist.line();
      if (line[1] != '\t') {
        cout << slist.line();
      } else {
        time_t ts;
        Node*  node   = NULL;

        slist.decodeLine(line, "", &node, &ts);
        showLine(ts, node);
        free(node);
      }
    }
    slist.close();
  } else {
    cerr << "Failed to open list" << endl;
  }
}

int main(void) {
  umask(0022);
  Path* path = new Path("/home/User");
  Database  db("test_db");
  // Journal
  List    real_journal("test_db", "journal");
  List    journal("test_db", "journal~");
  List    dblist("test_db", "list");
  // Filter
  Filter* filter;

  // Initialisation
  my_time++;
  db.open();

  // '-' is before '/' in the ASCII table...
  system("touch test1/subdir-file");
  system("touch test1/subdirfile");
  system("touch test1/àccénts_test");

  cout << "first with subdir/testfile NOT readable" << endl;
  system("chmod 000 test1/subdir/testfile");
  db.setClient("file://localhost");
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
  my_time++;
  db.open();

  cout << "as previous, with a .hbackup directory" << endl;
  mkdir("test1/.hbackup", 0755);
  db.setClient("file://localhost");
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
  my_time++;
  db.open();

  cout << "as previous with new big_file, interrupted" << endl;
  system("dd if=/dev/zero of=test1/cvs/big_file bs=1k count=500"
    " > /dev/null 2>&1");
  killed = true;
  db.setClient("file://localhost");
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
  my_time++;
  db.open();

  cout << "as previous with subdir/testfile readable" << endl;
  killed  = false;
  killall = false;
  system("chmod 644 test1/subdir/testfile");
  db.setClient("file://localhost");
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
  my_time++;
  db.open();

  cout << "as previous with subdir/testfile in ignore list" << endl;
  filter = path->addFilter("and", "testfile");
  if ((filter == NULL)
   || filter->add("type", "file", false)
   || filter->add("path", "subdir/testfile", false)) {
    cout << "Failed to add filter" << endl;
  }
  path->setIgnore(filter);
  db.setClient("file://localhost");
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
  my_time++;
  db.open();

  cout << "as previous with subdir in ignore list" << endl;
  Filter* subdir = path->addFilter("and", "subdir");
  if ((subdir == NULL)
   || subdir->add("type", "dir", false)
   || subdir->add("path", "subdir", false)) {
    cout << "Failed to add subdir 'and' filter" << endl;
  }
  path->setIgnore(subdir);
  db.setClient("file://localhost");
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
  my_time++;
  db.open();

  cout << "as previous with testlink modified" << endl;
  system("sleep 1 && ln -sf testnull test1/testlink");
  db.setClient("file://localhost");
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
  my_time++;
  db.open();

  cout << "as previous with testlink in ignore list" << endl;
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
  filter->add(new Condition(condition_subfilter, subdir, false));
  filter->add(new Condition(condition_subfilter, testlink, false));
  path->setIgnore(filter);
  db.setClient("file://localhost");
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
  my_time++;
  db.open();

  cout << "as previous with CVS parser" << endl;
  if (path->addParser("cvs", "controlled")) {
    cout << "Failed to add parser" << endl;
  }
  db.setClient("file://localhost");
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
  my_time++;
  db.open();

  cout << "as previous" << endl;
  db.setClient("file://localhost");
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
  my_time++;
  db.open();

  cout << "as previous with cvs/dirutd in ignore list" << endl;
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
  filter->add(new Condition(condition_subfilter, subdir, false));
  filter->add(new Condition(condition_subfilter, testlink, false));
  filter->add(new Condition(condition_subfilter, cvs_dirutd, false));
  filter->add(new Condition(condition_subfilter, empty, false));
  path->setIgnore(filter);
  db.setClient("file://localhost");
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
  my_time++;
  db.open();

  cout << "as previous with testpipe gone" << endl;
  rename("test1/testpipe", "testpipe");
  db.setClient("file://localhost");
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
  my_time++;
  db.open();

  cout << "as previous with testfile mode changed" << endl;
  system("chmod 660 test1/testfile");
  db.setClient("file://localhost");
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
  my_time++;
  db.open();

  cout << "as previous with cvs/filenew.c touched and testpipe restored" << endl;
  system("echo blah > test1/cvs/filenew.c");
  rename("testpipe", "test1/testpipe");
  db.setClient("file://localhost");
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
  my_time++;
  db.open();

  cout << "some troublesome past cases" << endl;

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

  db.setClient("file://localhost");
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
  my_time++;
  db.open();

  db.setClient("file://localhost");
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
  my_time++;
  cout << endl << "List recovery after crash" << endl;
  system("touch \"test1/test space\"");
  system("mkdir test1/crash");
  system("touch test1/crash/file");
  system("rm -rf test1/testdir");
  db.open();

  db.setClient("file://host");
  if (! path->parse(db, "test2")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.setClient("file://localhost");
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
  db.open();

  // Show list contents
  cout << endl << "List:" << endl;
  showList(dblist);
  // Show journal contents
  cout << endl << "Journal:" << endl;
  showList(journal);
  if (db.close()) {
    return 0;
  }


  db.open();

  if (db.close()) {
    return 0;
  }


  // Next test
  my_time++;
  db.open();

  db.setClient("file://localhost");
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
  my_time++;
  db.open();

  db.setClient("file://host");
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
  db.open();

  system("touch test1/testfile");
  system("chmod 0 test1/testfile");
  // Make removal complain
  system("rm -r test_db/data/285b35198a5e188b3a0df3ed33f93a26-0");
  db.setClient("file://localhost", 18 * 24 * 3600);
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
  my_time++;
  db.open();

  system("touch test1/testfile~");
  system("rm -f test1/testfile");
  // Make removal fail
  system("touch test_db/data/59ca0efa9f5633cb0371bbc0355478d8-0/blah");
  db.setClient("file://localhost", 0);
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
  my_time++;
  db.open();

  system("echo blah > test1/testfile~");
  db.setClient("file://localhost", 0);
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

  delete path;
  return 0;
}
