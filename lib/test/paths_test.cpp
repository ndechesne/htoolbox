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
#include <string>

using namespace std;

#include "strings.h"
#include "files.h"
#include "filters.h"
#include "parsers.h"
#include "cvs_parser.h"
#include "list.h"
#include "db.h"
#include "paths.h"
#include "hbackup.h"

using namespace hbackup;

int hbackup::verbosity(void) {
  return 4;
}

int hbackup::terminating(void) {
  return 0;
}

static time_t my_time = 0;
time_t time(time_t *t) {
  return my_time;
}

static void showLine(time_t timestamp, char* prefix, char* path, Node* node) {
  printf("[%2ld] %-16s %-34s", timestamp, prefix, path);
  if (node != NULL) {
    printf(" %c %5llu %03o", node->type(), node->size(), node->mode());
    if (node->type() == 'f') {
      printf(" %s", ((File*) node)->checksum());
    }
    if (node->type() == 'l') {
      printf(" %s", ((Link*) node)->link());
    }
  } else {
    printf(" [rm]");
  }
  cout << endl;
}

int main(void) {
  Path* path = new Path("/home/User");
  Database  db("test_db");
  // Journal
  List    journal("test_db", "journal~");
  List    list("test_db", "list");
  time_t  timestamp;
  char*   prefix  = NULL;
  char*   fpath   = NULL;
  Node*   node    = NULL;

  // Initialisation
  my_time++;
  db.open();

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }

  // '-' is before '/' in the ASCII table...
  system("touch test1/subdir-file");
  system("touch test1/subdirfile");
  system("touch test1/àccénts_test");

  cout << "first with subdir/testfile NOT readable" << endl;
  system("chmod 000 test1/subdir/testfile");
  db.setPrefix("file://localhost");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  db.close();
  // Show list contents
  cout << endl << "List:" << endl;
  if (! list.open("r")) {
    while (list.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    list.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;
  // Show journal contents
  cout << endl << "Journal:" << endl;
  if (! journal.open("r")) {
    while (journal.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;

  // Next test
  my_time++;
  db.open();

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }

  cout << "as previous" << endl;
  db.setPrefix("file://localhost");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  db.close();
  // Show list contents
  cout << endl << "List:" << endl;
  if (! list.open("r")) {
    while (list.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    list.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;
  // Show journal contents
  cout << endl << "Journal:" << endl;
  if (! journal.open("r")) {
    while (journal.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;

  // Next test
  my_time++;
  db.open();

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }

  cout << "as previous with subdir/testfile readable" << endl;
  system("chmod 644 test1/subdir/testfile");
  db.setPrefix("file://localhost");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  db.close();
  // Show list contents
  cout << endl << "List:" << endl;
  if (! list.open("r")) {
    while (list.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    list.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;
  // Show journal contents
  cout << endl << "Journal:" << endl;
  if (! journal.open("r")) {
    while (journal.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;

  // Next test
  my_time++;
  db.open();

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }

  cout << "as previous with subdir/testfile in ignore list" << endl;
  if (path->addFilter("type", "file")
   || path->addFilter("path", "subdir/testfile", true)) {
    cout << "Failed to add filter" << endl;
  }
  db.setPrefix("file://localhost");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  db.close();
  // Show list contents
  cout << endl << "List:" << endl;
  if (! list.open("r")) {
    while (list.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    list.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;
  // Show journal contents
  cout << endl << "Journal:" << endl;
  if (! journal.open("r")) {
    while (journal.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;

  // Next test
  my_time++;
  db.open();

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }

  cout << "as previous with subdir in ignore list" << endl;
  if (path->addFilter("type", "dir")
   || path->addFilter("path", "subdir", true)) {
    cout << "Failed to add filter" << endl;
  }
  db.setPrefix("file://localhost");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  db.close();
  // Show list contents
  cout << endl << "List:" << endl;
  if (! list.open("r")) {
    while (list.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    list.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;
  // Show journal contents
  cout << endl << "Journal:" << endl;
  if (! journal.open("r")) {
    while (journal.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;

  // Next test
  my_time++;
  db.open();

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }

  cout << "as previous with testlink modified" << endl;
  system("sleep 1 && ln -sf testnull test1/testlink");
  if (path->addFilter("type", "dir")
   || path->addFilter("path", "subdir", true)) {
    cout << "Failed to add filter" << endl;
  }
  db.setPrefix("file://localhost");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  db.close();
  // Show list contents
  cout << endl << "List:" << endl;
  if (! list.open("r")) {
    while (list.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    list.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;
  // Show journal contents
  cout << endl << "Journal:" << endl;
  if (! journal.open("r")) {
    while (journal.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;

  // Next test
  my_time++;
  db.open();

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }

  cout << "as previous with testlink in ignore list" << endl;
  if (path->addFilter("type", "link")
   || path->addFilter("path_start", "testlink", true)) {
    cout << "Failed to add filter" << endl;
  }
  db.setPrefix("file://localhost");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  db.close();
  // Show list contents
  cout << endl << "List:" << endl;
  if (! list.open("r")) {
    while (list.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    list.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;
  // Show journal contents
  cout << endl << "Journal:" << endl;
  if (! journal.open("r")) {
    while (journal.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;

  // Next test
  my_time++;
  db.open();

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }

  cout << "as previous with CVS parser" << endl;
  if (path->addParser("cvs", "controlled")) {
    cout << "Failed to add parser" << endl;
  }
  db.setPrefix("file://localhost");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  db.close();
  // Show list contents
  cout << endl << "List:" << endl;
  if (! list.open("r")) {
    while (list.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    list.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;
  // Show journal contents
  cout << endl << "Journal:" << endl;
  if (! journal.open("r")) {
    while (journal.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;

  // Next test
  my_time++;
  db.open();

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }

  cout << "as previous" << endl;
  db.setPrefix("file://localhost");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  db.close();
  // Show list contents
  cout << endl << "List:" << endl;
  if (! list.open("r")) {
    while (list.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    list.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;
  // Show journal contents
  cout << endl << "Journal:" << endl;
  if (! journal.open("r")) {
    while (journal.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;

  // Next test
  my_time++;
  db.open();

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }

  cout << "as previous with cvs/dirutd in ignore list" << endl;
  if (path->addFilter("type", "dir")
   || path->addFilter("path", "cvs/dirutd", true)) {
    cout << "Failed to add filter" << endl;
  }
  db.setPrefix("file://localhost");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  db.close();
  // Show list contents
  cout << endl << "List:" << endl;
  if (! list.open("r")) {
    while (list.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    list.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;
  // Show journal contents
  cout << endl << "Journal:" << endl;
  if (! journal.open("r")) {
    while (journal.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;

  // Next test
  my_time++;
  db.open();

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }

  cout << "as previous with testpipe gone" << endl;
  remove("test1/testpipe");
  db.setPrefix("file://localhost");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  db.close();
  // Show list contents
  cout << endl << "List:" << endl;
  if (! list.open("r")) {
    while (list.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    list.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;
  // Show journal contents
  cout << endl << "Journal:" << endl;
  if (! journal.open("r")) {
    while (journal.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;

  // Next test
  my_time++;
  db.open();

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }

  cout << "as previous with testfile mode changed" << endl;
  system("chmod 660 test1/testfile");
  db.setPrefix("file://localhost");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  db.close();
  // Show list contents
  cout << endl << "List:" << endl;
  if (! list.open("r")) {
    while (list.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    list.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;
  // Show journal contents
  cout << endl << "Journal:" << endl;
  if (! journal.open("r")) {
    while (journal.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;

  // Next test
  my_time++;
  db.open();

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }

  cout << "as previous with cvs/filenew.c touched" << endl;
  system("echo blah > test1/cvs/filenew.c");
  db.setPrefix("file://localhost");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  db.close();
  // Show list contents
  cout << endl << "List:" << endl;
  if (! list.open("r")) {
    while (list.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    list.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;
  // Show journal contents
  cout << endl << "Journal:" << endl;
  if (! journal.open("r")) {
    while (journal.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;

  // Next test
  my_time++;
  db.open();

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }

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

  db.setPrefix("file://localhost");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  db.close();
  // Show list contents
  cout << endl << "List:" << endl;
  if (! list.open("r")) {
    while (list.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    list.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;
  // Show journal contents
  cout << endl << "Journal:" << endl;
  if (! journal.open("r")) {
    while (journal.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;

  // Next test
  my_time++;
  db.open();

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }

  db.setPrefix("file://localhost");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  db.close();
  // Show list contents
  cout << endl << "List:" << endl;
  if (! list.open("r")) {
    while (list.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    list.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;
  // Show journal contents
  cout << endl << "Journal:" << endl;
  if (! journal.open("r")) {
    while (journal.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }
  free(prefix);
  prefix = NULL;
  free(fpath);
  fpath = NULL;
  free(node);
  node = NULL;

  // Next test
  my_time++;
  cout << endl << "List recovery after crash" << endl;
  system("touch \"test1/test space\"");
  system("mkdir test1/crash");
  system("touch test1/crash/file");
  system("rm -rf test1/testdir");
  db.open();

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }

  db.setPrefix("file://host");
  if (! path->parse(db, "test2")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }
  db.setPrefix("file://localhost");
  if (! path->parse(db, "test1")) {
    cout << "Parsed " << path->nodes() << " file(s)\n";
  }

  {
    // NOT closing the database! => new one
    Database  db("test_db");
    remove("test_db/lock");
    // Show list contents
    cout << endl << "List:" << endl;
    if (! list.open("r")) {
      while (list.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
        showLine(timestamp, prefix, fpath, node);
      }
      list.close();
    } else {
      cerr << "Failed to open journal" << endl;
    }
    free(prefix);
    prefix = NULL;
    free(fpath);
    fpath = NULL;
    free(node);
    node = NULL;
    // Show journal contents
    cout << endl << "Journal:" << endl;
    List real_journal("test_db", "journal");
    if (! real_journal.open("r")) {
      while (real_journal.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
        showLine(timestamp, prefix, fpath, node);
      }
      real_journal.close();
    } else {
      cerr << "Failed to open real journal" << endl;
    }
    free(prefix);
    prefix = NULL;
    free(fpath);
    fpath = NULL;
    free(node);
    node = NULL;

    // Recover now
    db.open();

    // Display DB contents
    cout << "Active list:  " << ((DbList*)db.active())->size()
      << " element(s):\n";
    for (DbList::iterator i = ((DbList*)db.active())->begin();
        i != ((DbList*)db.active())->end(); i++) {
      i->line();
    }

    // Show list contents
    cout << endl << "List:" << endl;
    if (! list.open("r")) {
      while (list.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
        showLine(timestamp, prefix, fpath, node);
      }
      list.close();
    } else {
      cerr << "Failed to open journal" << endl;
    }
    free(prefix);
    prefix = NULL;
    free(fpath);
    fpath = NULL;
    free(node);
    node = NULL;
    // Show journal contents
    cout << endl << "Journal:" << endl;
    if (! journal.open("r")) {
      while (journal.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
        showLine(timestamp, prefix, fpath, node);
      }
      journal.close();
    } else {
      cerr << "Failed to open journal" << endl;
    }
    free(prefix);
    prefix = NULL;
    free(fpath);
    fpath = NULL;
    free(node);
    node = NULL;
    db.close();

    db.open();

    // Display DB contents
    cout << "Active list:  " << ((DbList*)db.active())->size()
      << " element(s):\n";
    for (DbList::iterator i = ((DbList*)db.active())->begin();
        i != ((DbList*)db.active())->end(); i++) {
      i->line();
    }

    db.close();

    // Next test
    my_time++;
    db.open();

    // Display DB contents
    cout << "Active list:  " << ((DbList*)db.active())->size()
      << " element(s):\n";
    for (DbList::iterator i = ((DbList*)db.active())->begin();
        i != ((DbList*)db.active())->end(); i++) {
      i->line();
    }

    db.setPrefix("file://localhost");
    if (! path->parse(db, "test1")) {
      cout << "Parsed " << path->nodes() << " file(s)\n";
    }

    db.close();
    // Show list contents
    cout << endl << "List:" << endl;
    if (! list.open("r")) {
      while (list.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
        showLine(timestamp, prefix, fpath, node);
      }
      list.close();
    } else {
      cerr << "Failed to open journal" << endl;
    }
    free(prefix);
    prefix = NULL;
    free(fpath);
    fpath = NULL;
    free(node);
    node = NULL;
    // Show journal contents
    cout << endl << "Journal:" << endl;
    if (! journal.open("r")) {
      while (journal.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
        showLine(timestamp, prefix, fpath, node);
      }
      journal.close();
    } else {
      cerr << "Failed to open journal" << endl;
    }
    free(prefix);
    prefix = NULL;
    free(fpath);
    fpath = NULL;
    free(node);
    node = NULL;

    // Next test
    my_time++;
    db.open();

    // Display DB contents
    cout << "Active list:  " << ((DbList*)db.active())->size()
      << " element(s):\n";
    for (DbList::iterator i = ((DbList*)db.active())->begin();
        i != ((DbList*)db.active())->end(); i++) {
      i->line();
    }

    db.setPrefix("file://host");
    if (! path->parse(db, "test2")) {
      cout << "Parsed " << path->nodes() << " file(s)\n";
    }

    db.close();
    // Show list contents
    cout << endl << "List:" << endl;
    if (! list.open("r")) {
      while (list.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
        showLine(timestamp, prefix, fpath, node);
      }
      list.close();
    } else {
      cerr << "Failed to open journal" << endl;
    }
    free(prefix);
    prefix = NULL;
    free(fpath);
    fpath = NULL;
    free(node);
    node = NULL;
    // Show journal contents
    cout << endl << "Journal:" << endl;
    if (! journal.open("r")) {
      while (journal.getEntry(&timestamp, &prefix, &fpath, &node) > 0) {
        showLine(timestamp, prefix, fpath, node);
      }
      journal.close();
    } else {
      cerr << "Failed to open journal" << endl;
    }
    free(prefix);
    prefix = NULL;
    free(fpath);
    fpath = NULL;
    free(node);
    node = NULL;

    cerr << endl << "Some final garbage because of our little pseudo-crash..."
      << endl;
  }

  delete path;
  return 0;
}
