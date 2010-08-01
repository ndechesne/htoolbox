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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

#include "test.h"

#include "files.h"
#include "conditions.h"
#include "filters.h"
#include "parsers.h"
#include "list.h"
#include "opdata.h"
#include "db.h"
#include "attributes.h"
#include "paths.h"

static void progress(long long previous, long long current, long long total) {
  if (current < total) {
    cout << "Done: " << setw(5) << setiosflags(ios::fixed) << setprecision(1)
      << 100.0 * static_cast<double>(current) / static_cast<double>(total)
      << "%\r" << flush;
  } else if (previous != 0) {
    cout << "            \r";
  }
}

static time_t my_time = 0;
time_t time(time_t *t) {
  (void) t;
  return 2000000000 + (my_time * 24 * 3600);
}

int main(void) {
  int sys_rc;

  umask(0022);
  report.setLevel(debug);

  Attributes attributes;
  ClientPath* path = new ClientPath("/home/User", attributes);
  Database    db("test_db");
  db.setProgressCallback(progress);
  // myClient's lists
  ListReader real_journal_reader(Path("test_db", "myClient/journal"));
  ListReader journal_reader(Path("test_db", "myClient/journal~"));
  ListReader dblist_reader(Path("test_db", "myClient/list"));
  // Filter
  vector<string> params;
  params.resize(3);
  params[0] = "filter";
  params[1] = "and";
  params[2] = "bazaar";
  Filter* bazaar = path->attributes().addFilter(params);
  if ((bazaar == NULL)
   || bazaar->add("type", "d", false)
   || bazaar->add("path", "bzr", false)) {
    cout << "Failed to add filter" << endl;
  }
  path->attributes().addIgnore(bazaar);

  // Initialisation
  cout << endl << "Initial backup, check '/' precedence" << endl;
  my_time++;
  if (db.open(false, true) < 0) {
    cout << "failed to open DB" << endl;
    return 0;
  }

  // '-' is before '/' in the ASCII table...
  sys_rc = system("touch test1/subdir-file");
  sys_rc = system("touch test1/subdirfile");
  sys_rc = system("touch test1/àccénts_test");

  cout << "first with subdir NOT readable" << endl;
  sys_rc = system("chmod 000 test1/subdir");
  sys_rc = system("chmod 000 test1/subdirfile");
  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }
  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);
  // Initialize list of missing checksums
  if (! db.open()) {
    db.scan();
    db.close();
  }


  // Next test
  cout << endl << "As previous" << endl;
  my_time++;
  db.open();

  mkdir("test1/.hbackup", 0755);
  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);


  // Next test
  cout << endl << "As previous, with newly inaccessible dir and report once"
    << endl;
  sys_rc = system("chmod 000 test1/cvs");
  path->attributes().setReportCopyErrorOnce();
  my_time++;
  db.open();

  mkdir("test1/.hbackup", 0755);
  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);


  // Next test
  cout << endl << "As previous, with subdir/testfile NOT readable" << endl;
  sys_rc = system("chmod 755 test1/subdir");
  sys_rc = system("chmod 755 test1/cvs");
  sys_rc = system("chmod 644 test1/subdirfile");
  sys_rc = system("chmod 000 test1/subdir/testfile");
  my_time++;
  db.open();

  mkdir("test1/.hbackup", 0755);
  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl << "As first, with a .hbackup directory" << endl;
  my_time++;
  db.open();

  mkdir("test1/.hbackup", 0755);
  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl << "As previous with new big_file, interrupted" << endl;
  my_time++;
  db.open();

  sys_rc = system("dd if=/dev/zero of=test1/cvs/big_file bs=1k count=500"
    " > /dev/null 2>&1");
  abort(2);
  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient(true);

  if (db.close()) {
    return 0;
  }
  abort(0xffff);

  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's real journal:" << endl;
  real_journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl << "As previous with subdir/testfile readable" << endl;
  my_time++;
  db.open();

  sys_rc = system("chmod 644 test1/subdir/testfile");
  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl << "As previous with subdir/testfile in ignore list" << endl;
  my_time++;
  db.open();

  params[2] = "testfile";
  Filter* testfile = path->attributes().addFilter(params);
  if ((testfile == NULL)
   || testfile->add("type", "file", false)
   || testfile->add("path", "subdir/testfile", false)) {
    cout << "Failed to add filter" << endl;
  }
  params[1] = "or";
  params[2] = "ignore1";
  Filter* filter = path->attributes().addFilter(params);
  if (filter == NULL) {
    cout << "Failed to add 'or' filter" << endl;
  }
  filter->add(new Condition(Condition::filter, bazaar, false));
  filter->add(new Condition(Condition::filter, testfile, false));
  path->attributes().addIgnore(filter);
  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl << "As previous with subdir in ignore list" << endl;
  my_time++;
  db.open();

  params[1] = "and";
  params[2] = "subdir";
  Filter* subdir = path->attributes().addFilter(params);
  if ((subdir == NULL)
   || subdir->add("type", "dir", false)
   || subdir->add("path", "subdir", false)) {
    cout << "Failed to add subdir 'and' filter" << endl;
  }
  params[1] = "or";
  params[2] = "ignore1";
  filter = path->attributes().addFilter(params);
  if (filter == NULL) {
    cout << "Failed to add 'or' filter" << endl;
  }
  filter->add(new Condition(Condition::filter, bazaar, false));
  filter->add(new Condition(Condition::filter, subdir, false));
  path->attributes().addIgnore(filter);
  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl << "As previous with testlink modified" << endl;
  my_time++;
  db.open();

  sys_rc = system("sleep 1 && ln -sf testnull test1/testlink");
  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl << "As previous with testlink in ignore list" << endl;
  my_time++;
  db.open();

  params[1] = "and";
  params[2] = "testlink";
  Filter* testlink = path->attributes().addFilter(params);
  if ((testlink == NULL)
   || testlink->add("type", "link", false)
   || testlink->add("path_start", "testlink", false)) {
    cout << "Failed to add testlink 'and' filter" << endl;
  }
  params[1] = "or";
  params[2] = "ignore1";
  filter = path->attributes().addFilter(params);
  if (filter == NULL) {
    cout << "Failed to add 'or' filter" << endl;
  }
  filter->add(new Condition(Condition::filter, bazaar, false));
  filter->add(new Condition(Condition::filter, subdir, false));
  filter->add(new Condition(Condition::filter, testlink, false));
  path->attributes().addIgnore(filter);
  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl << "As previous with CVS parser (controlled)" << endl;
  my_time++;
  db.open();

  if (path->addParser("cvs", "controlled")) {
    cout << "Failed to add CVS parser" << endl;
  }
  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl << "As previous" << endl;
  my_time++;
  db.open();

  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl << "As previous with Subversion parser (modified)" << endl;
  my_time++;
  db.open();

  if (path->addParser("cvs", "controlled")) {
    cout << "Failed to add CVS parser" << endl;
  }
  if (path->addParser("svn", "modified")) {
    cout << "Failed to add Subversion parser" << endl;
  }
  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl << "As previous with cvs/dirutd in ignore list" << endl;
  my_time++;
  db.open();

  params[1] = "and";
  params[2] = "cvs_dirutd";
  Filter* cvs_dirutd = path->attributes().addFilter(params);
  if ((cvs_dirutd == NULL)
   || cvs_dirutd->add("type", "dir", false)
   || cvs_dirutd->add("path", "cvs/dirutd", false)) {
    cout << "Failed to add 'and' filter" << endl;
  }
  params[1] = "or";
  params[2] = "empty";
  Filter* empty = path->attributes().addFilter(params);
  params[1] = "or";
  params[2] = "ignore2";
  filter = path->attributes().addFilter(params);
  if (filter == NULL) {
    cout << "Failed to add 'or' filter" << endl;
  }
  filter->add(new Condition(Condition::filter, bazaar, false));
  filter->add(new Condition(Condition::filter, subdir, false));
  filter->add(new Condition(Condition::filter, testlink, false));
  filter->add(new Condition(Condition::filter, cvs_dirutd, false));
  filter->add(new Condition(Condition::filter, empty, false));
  path->attributes().addIgnore(filter);
  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl << "As previous with svn/dirutd in ignore list" << endl;
  my_time++;
  db.open();

  params[1] = "and";
  params[2] = "svn_dirutd";
  Filter* svn_dirutd = path->attributes().addFilter(params);
  if ((svn_dirutd == NULL)
   || svn_dirutd->add("type", "dir", false)
   || svn_dirutd->add("path", "svn/dirutd", false)) {
    cout << "Failed to add 'and' filter" << endl;
  }
  filter->add(new Condition(Condition::filter, bazaar, false));
  filter->add(new Condition(Condition::filter, svn_dirutd, false));
  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl << "As previous with testpipe gone" << endl;
  my_time++;
  db.open();

  rename("test1/testpipe", "testpipe");
  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl << "As previous with testfile and testdir mode changed" << endl;
  my_time++;
  db.open();

  sys_rc = system("chmod 770 test1/testdir");
  sys_rc = system("chmod 660 test1/testfile");
  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl
    << "As previous with cvs/filenew.c touched and testpipe restored" << endl;
  my_time++;
  db.open();

   // Also test that we behave correctly when dir exists but is empty
  sys_rc = system("echo blah > test1/cvs/filenew.c");
  mkdir("test_db/.data/0d599f0ec05c3bda8c3b8a68c32a1b47-0", 0755);

  rename("testpipe", "test1/testpipe");
  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl << "Some troublesome past cases" << endl;
  my_time++;
  db.open();

  sys_rc = system("mkdir -p test1/docbook-xml/3.1.7");
  sys_rc = system("touch test1/docbook-xml/3.1.7/dbgenent.ent");
  sys_rc = system("mkdir test1/docbook-xml/4.0");
  sys_rc = system("touch test1/docbook-xml/4.0/dbgenent.ent");
  sys_rc = system("mkdir test1/docbook-xml/4.1.2");
  sys_rc = system("touch test1/docbook-xml/4.1.2/dbgenent.mod");
  sys_rc = system("mkdir test1/docbook-xml/4.2");
  sys_rc = system("touch test1/docbook-xml/4.2/dbgenent.mod");
  sys_rc = system("mkdir test1/docbook-xml/4.3");
  sys_rc = system("touch test1/docbook-xml/4.3/dbgenent.mod");
  sys_rc = system("mkdir test1/docbook-xml/4.4");
  sys_rc = system("touch test1/docbook-xml/4.4/dbgenent.mod");
  sys_rc = system("touch test1/docbook-xml.cat");
  sys_rc = system("touch test1/docbook-xml.cat.old");

  sys_rc = system("mkdir -p test1/testdir/biblio");
  sys_rc = system("touch test1/testdir/biblio/biblio.dbf");
  sys_rc = system("touch test1/testdir/biblio/biblio.dbt");
  sys_rc = system("touch test1/testdir/biblio.odb");
  sys_rc = system("touch test1/testdir/evolocal.odb");

  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl << "Same again: journal and list left untouched" << endl;
  my_time++;
  db.open();

  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl << "Register recovery after crash" << endl;
  my_time++;
  sys_rc = system("touch \"test1/test space\"");
  sys_rc = system("mkdir test1/crash");
  sys_rc = system("touch test1/crash/file");
  sys_rc = system("rm -rf test1/testdir");
  db.open();

  path->show();
  db.openClient("hisClient");
  if (! path->parse(db, "test2", "host2")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();
  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  // Save files to test recovery
  sys_rc = system("cp test_db/myClient/list test_db/myClient/list.cp");
  sys_rc = system("cp test_db/myClient/journal test_db/myClient/journal.cp");

  db.closeClient();
  if (db.close()) {
    return 0;
  }

  // hisClient's lists
  ListReader journal2_reader(Path("test_db", "hisClient/journal~"));
  ListReader dblist2_reader(Path("test_db", "hisClient/list"));

  // Replace files to test recovery
  rename("test_db/myClient/list.cp", "test_db/myClient/list");
  rename("test_db/myClient/journal.cp", "test_db/myClient/journal");

  // Show list contents
  cout << endl << "hisClient's list:" << endl;
  dblist2_reader.show(-1, 2000000000, 24 * 3600);
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "hisClient's journal:" << endl;
  journal2_reader.show(-1, 2000000000, 24 * 3600);
  cout << endl << "myClient's real journal:" << endl;
  real_journal_reader.show(-1, 2000000000, 24 * 3600);

  // Recover now
  db.open();

  // Show list contents
  cout << endl << "hisClient's list:" << endl;
  dblist2_reader.show(-1, 2000000000, 24 * 3600);
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "hisClient's journal:" << endl;
  journal2_reader.show(-1, 2000000000, 24 * 3600);
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  if (db.close()) {
    return 0;
  }


  db.open();

  if (db.close()) {
    return 0;
  }


  // Next test
  cout << endl << "Same again" << endl;
  my_time++;
  db.open();

  path->show();
  db.openClient("myClient");
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "hisClient's list:" << endl;
  dblist2_reader.show(-1, 2000000000, 24 * 3600);
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "hisClient's journal:" << endl;
  journal2_reader.show(-1, 2000000000, 24 * 3600);
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl << "Same again" << endl;
  my_time++;
  db.open();

  path->show();
  db.openClient("hisClient");
  if (! path->parse(db, "test2", "host2")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "hisClient's list:" << endl;
  dblist2_reader.show(-1, 2000000000, 24 * 3600);
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "hisClient's journal:" << endl;
  journal2_reader.show(-1, 2000000000, 24 * 3600);
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  my_time++;
  cout << endl << "Expiry delay: 18, date: " << my_time << endl;
  db.open();

  sys_rc = system("touch test1/testfile");
  sys_rc = system("chmod 0 test1/testfile");
  // Make removal complain
  sys_rc = system("rm -r test_db/.data/285b35198a5e188b3a0df3ed33f93a26-0");
  path->show();
  db.openClient("myClient", 18 * 24 * 3600);
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "hisClient's list:" << endl;
  dblist2_reader.show(-1, 2000000000, 24 * 3600);
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "hisClient's journal:" << endl;
  journal2_reader.show(-1, 2000000000, 24 * 3600);
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl << "Prevent data dir from being removed" << endl;
  my_time++;
  db.open();

  sys_rc = system("touch test1/testfile~");
  sys_rc = system("rm -f test1/testfile");
  // Make removal fail
  sys_rc = system("chmod 0 test_db/.data/59ca0efa9f5633cb0371bbc0355478d8-0");
  path->show();
  db.openClient("myClient", 0);
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  int rc = db.close();
  sys_rc = system("chmod 755 test_db/.data/59ca0efa9f5633cb0371bbc0355478d8-0");
  if (rc) {
    return 0;
  }

  // Show list contents
  cout << endl << "hisClient's list:" << endl;
  dblist2_reader.show(-1, 2000000000, 24 * 3600);
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "hisClient's journal:" << endl;
  journal2_reader.show(-1, 2000000000, 24 * 3600);
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  // Next test
  cout << endl << "Add one file" << endl;
  my_time++;
  db.open();

  sys_rc = system("echo blah > test1/testfile~");
  path->show();
  db.openClient("myClient", 0);
  if (! path->parse(db, "test1", "host")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.closeClient();

  if (db.close()) {
    return 0;
  }

  // Show list contents
  cout << endl << "hisClient's list:" << endl;
  dblist2_reader.show(-1, 2000000000, 24 * 3600);
  cout << endl << "myClient's list:" << endl;
  dblist_reader.show(-1, 2000000000, 24 * 3600);
  // Show journal contents
  cout << endl << "hisClient's journal:" << endl;
  journal2_reader.show(-1, 2000000000, 24 * 3600);
  cout << endl << "myClient's journal:" << endl;
  journal_reader.show(-1, 2000000000, 24 * 3600);

  delete path;
  return 0;
}
