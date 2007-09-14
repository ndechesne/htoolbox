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
#include <errno.h>

using namespace std;

#include "strings.h"
#include "files.h"
#include "list.h"
#include "hbackup.h"

using namespace hbackup;

static int verbose = 4;

int hbackup::verbosity(void) {
  return verbose;
}

int hbackup::terminating(void) {
  return 0;
}

static time_t my_time = 0;
time_t time(time_t *t) {
  return my_time;
}

int main(void) {
  List    list("test_db/list");
  List    journal("test_db/journal");
  List    merge("test_db/merge");
  char*   line   = NULL;
  char*   prefix = NULL;
  char*   path   = NULL;
  Node*   node   = NULL;
  time_t  ts;

  cout << "Test: DB lists" << endl;
  mkdir("test_db", 0755);

  if (list.open("w")) {
    cerr << "Failed to open list" << endl;
    return 0;
  }
  list.close();

  cout << endl << "Test: journal write" << endl;
  my_time++;

  if (journal.open("w")) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  journal.removed("prefix", "file_gone");
  node = new Stream("test1/test space");
  ((Stream*) node)->computeChecksum();
  journal.added("prefix2", "file sp", node);
  free(node);
  node = new Stream("test1/testfile");
  ((Stream*) node)->computeChecksum();
  journal.added("prefix2", "file_new", node);
  free(node);
  node = new Link("test1/testlink");
  journal.added("prefix3", "link", node);
  free(node);
  node = new Directory("test1/testdir");
  journal.added("prefix5", "path", node);
  free(node);
  journal.close();

  cout << endl << "Test: journal read" << endl;
  my_time++;

  node = NULL;
  journal.open("r");
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  } else
  while (journal.getEntry(&ts, &prefix, &path, &node) > 0) {
    cout << "Prefix: " << prefix << endl;
    cout << "Path:   " << path << endl;
    cout << "TS:     " << ts << endl;
    if (node == NULL) {
      cout << "Type:   removed" << endl;
    } else {
      switch (node->type()) {
        case 'f':
          cout << "Type:   file" << endl;
          break;
        case 'l':
          cout << "Type:   link" << endl;
          break;
        default:
          cout << "Type:   other" << endl;
      }
      cout << "Name:   " << node->name() << endl;
      cout << "Size:   " << node->size() << endl;
      switch (node->type()) {
        case 'f':
          cout << "Chcksm: " << ((File*) node)->checksum() << endl;
          break;
        case 'l':
          cout << "Link:   " << ((Link*) node)->link() << endl;
      }
    }
    cout << endl;
  }
  journal.close();
  free(line);

  cout << endl << "Test: journal merge into empty list" << endl;
  my_time++;

  if (list.open("r")) {
    cerr << "Failed to open list" << endl;
    return 0;
  }
  if (list.isEmpty()) {
    cout << "List is empty" << endl;
  }
  if (journal.open("r")) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  }
  if (merge.open("w")) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (merge.merge(list, journal)) {
    cerr << "Failed to merge: " << strerror(errno) << endl;
    return 0;
  }
  merge.close();
  journal.close();
  list.close();

  cout << endl << "Test: merge read" << endl;
  my_time++;

  merge.open("r");
  if (merge.isEmpty()) {
    cout << "Merge is empty" << endl;
  } else
  while (merge.getEntry(&ts, &prefix, &path, &node) > 0) {
    cout << "Prefix: " << prefix << endl;
    cout << "Path:   " << path << endl;
    cout << "TS:     " << ts << endl;
    if (node == NULL) {
      cout << "Type:   removed" << endl;
    } else {
      switch (node->type()) {
        case 'f':
          cout << "Type:   file" << endl;
          break;
        case 'l':
          cout << "Type:   link" << endl;
          break;
        default:
          cout << "Type:   other" << endl;
      }
      cout << "Name:   " << node->name() << endl;
      cout << "Size:   " << node->size() << endl;
      switch (node->type()) {
        case 'f':
          cout << "Chcksm: " << ((File*) node)->checksum() << endl;
          break;
        case 'l':
          cout << "Link:   " << ((Link*) node)->link() << endl;
      }
      free(node);
      node = NULL;
    }
    cout << endl;
  }
  merge.close();
  free(line);

  if (rename("test_db/merge", "test_db/list")) {
    cerr << "Failed to rename merge into list" << endl;
  }

  cout << endl << "Test: journal write again" << endl;
  my_time++;

  if (journal.open("w")) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  system("echo \"this is my new test\" > test1/testfile");
  node = new Stream("test1/testfile");
  ((Stream*) node)->computeChecksum();
  journal.added("prefix", "file_new", node);
  journal.added("prefix2", "file_new", node);
  free(node);
  node = new Stream("test1/test space");
  ((Stream*) node)->computeChecksum();
  journal.added("prefix4", "file_new", node);
  free(node);
  journal.close();

  cout << endl << "Test: journal read" << endl;
  my_time++;

  node = NULL;
  journal.open("r");
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  } else
  while (journal.getEntry(&ts, &prefix, &path, &node) > 0) {
    cout << "Prefix: " << prefix << endl;
    cout << "Path:   " << path << endl;
    cout << "TS:     " << ts << endl;
    if (node == NULL) {
      cout << "Type:   removed" << endl;
    } else {
      switch (node->type()) {
        case 'f':
          cout << "Type:   file" << endl;
          break;
        case 'l':
          cout << "Type:   link" << endl;
          break;
        default:
          cout << "Type:   other" << endl;
      }
      cout << "Name:   " << node->name() << endl;
      cout << "Size:   " << node->size() << endl;
      switch (node->type()) {
        case 'f':
          cout << "Chcksm: " << ((File*) node)->checksum() << endl;
          break;
        case 'l':
          cout << "Link:   " << ((Link*) node)->link() << endl;
      }
    }
    cout << endl;
  }
  journal.close();
  free(line);

  cout << endl << "Test: journal merge into list" << endl;
  my_time++;

  if (list.open("r")) {
    cerr << "Failed to open list" << endl;
    return 0;
  }
  if (list.isEmpty()) {
    cout << "List is empty" << endl;
  }
  if (journal.open("r")) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  }
  if (merge.open("w")) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (merge.merge(list, journal)) {
    cerr << "Failed to merge: " << strerror(errno) << endl;
    return 0;
  }
  merge.close();
  journal.close();
  list.close();

  cout << endl << "Test: merge read" << endl;
  my_time++;

  merge.open("r");
  if (merge.isEmpty()) {
    cout << "Merge is empty" << endl;
  } else
  while (merge.getEntry(&ts, &prefix, &path, &node) > 0) {
    cout << "Prefix: " << prefix << endl;
    cout << "Path:   " << path << endl;
    cout << "TS:     " << ts << endl;
    if (node == NULL) {
      cout << "Type:   removed" << endl;
    } else {
      switch (node->type()) {
        case 'f':
          cout << "Type:   file" << endl;
          break;
        case 'l':
          cout << "Type:   link" << endl;
          break;
        default:
          cout << "Type:   other" << endl;
      }
      cout << "Name:   " << node->name() << endl;
      cout << "Size:   " << node->size() << endl;
      switch (node->type()) {
        case 'f':
          cout << "Chcksm: " << ((File*) node)->checksum() << endl;
          break;
        case 'l':
          cout << "Link:   " << ((Link*) node)->link() << endl;
      }
      free(node);
      node = NULL;
    }
    cout << endl;
  }
  merge.close();
  free(line);

  if (rename("test_db/merge", "test_db/list")) {
    cerr << "Failed to rename merge into list" << endl;
  }

  cout << endl << "Test: journal prefix out of order" << endl;
  my_time++;

  if (journal.open("w")) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  system("echo \"this is my new test\" > test1/testfile");
  node = new Stream("test1/testfile");
  ((Stream*) node)->computeChecksum();
  journal.added("prefix4", "file_new", node);
  journal.added("prefix2", "file_new", node);
  free(node);
  journal.close();

  cout << endl << "Test: journal read" << endl;
  my_time++;

  node = NULL;
  journal.open("r");
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  } else
  while (journal.getEntry(&ts, &prefix, &path, &node) > 0) {
    cout << "Prefix: " << prefix << endl;
    cout << "Path:   " << path << endl;
    cout << "TS:     " << ts << endl;
    if (node == NULL) {
      cout << "Type:   removed" << endl;
    } else {
      switch (node->type()) {
        case 'f':
          cout << "Type:   file" << endl;
          break;
        case 'l':
          cout << "Type:   link" << endl;
          break;
        default:
          cout << "Type:   other" << endl;
      }
      cout << "Name:   " << node->name() << endl;
      cout << "Size:   " << node->size() << endl;
      switch (node->type()) {
        case 'f':
          cout << "Chcksm: " << ((File*) node)->checksum() << endl;
          break;
        case 'l':
          cout << "Link:   " << ((Link*) node)->link() << endl;
      }
    }
    cout << endl;
  }
  journal.close();
  free(line);

  cout << endl << "Test: journal merge into list" << endl;
  my_time++;

  if (list.open("r")) {
    cerr << "Failed to open list" << endl;
    return 0;
  }
  if (list.isEmpty()) {
    cout << "List is empty" << endl;
  }
  if (journal.open("r")) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  }
  if (merge.open("w")) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (merge.merge(list, journal)) {
    cerr << "Failed to merge: " << strerror(errno) << endl;
//     return 0;
  }
  merge.close();
  journal.close();
  list.close();

  cout << endl << "Test: merge read" << endl;
  my_time++;

  merge.open("r");
  if (merge.isEmpty()) {
    cout << "Merge is empty" << endl;
  } else
  while (merge.getEntry(&ts, &prefix, &path, &node) > 0) {
    cout << "Prefix: " << prefix << endl;
    cout << "Path:   " << path << endl;
    cout << "TS:     " << ts << endl;
    if (node == NULL) {
      cout << "Type:   removed" << endl;
    } else {
      switch (node->type()) {
        case 'f':
          cout << "Type:   file" << endl;
          break;
        case 'l':
          cout << "Type:   link" << endl;
          break;
        default:
          cout << "Type:   other" << endl;
      }
      cout << "Name:   " << node->name() << endl;
      cout << "Size:   " << node->size() << endl;
      switch (node->type()) {
        case 'f':
          cout << "Chcksm: " << ((File*) node)->checksum() << endl;
          break;
        case 'l':
          cout << "Link:   " << ((Link*) node)->link() << endl;
      }
      free(node);
      node = NULL;
    }
    cout << endl;
  }
  merge.close();
  free(line);

  cout << endl << "Test: journal path out of order" << endl;
  my_time++;

  if (journal.open("w")) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  system("echo \"this is my new test\" > test1/testfile");
  node = new Stream("test1/testfile");
  ((Stream*) node)->computeChecksum();
  journal.added("prefix2", "file_new", node);
  journal.added("prefix2", "file_gone", node);
  free(node);
  journal.close();

  cout << endl << "Test: journal read" << endl;
  my_time++;

  node = NULL;
  journal.open("r");
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  } else
  while (journal.getEntry(&ts, &prefix, &path, &node) > 0) {
    cout << "Prefix: " << prefix << endl;
    cout << "Path:   " << path << endl;
    cout << "TS:     " << ts << endl;
    if (node == NULL) {
      cout << "Type:   removed" << endl;
    } else {
      switch (node->type()) {
        case 'f':
          cout << "Type:   file" << endl;
          break;
        case 'l':
          cout << "Type:   link" << endl;
          break;
        default:
          cout << "Type:   other" << endl;
      }
      cout << "Name:   " << node->name() << endl;
      cout << "Size:   " << node->size() << endl;
      switch (node->type()) {
        case 'f':
          cout << "Chcksm: " << ((File*) node)->checksum() << endl;
          break;
        case 'l':
          cout << "Link:   " << ((Link*) node)->link() << endl;
      }
    }
    cout << endl;
  }
  journal.close();
  free(line);

  cout << endl << "Test: journal merge into list" << endl;
  my_time++;

  if (list.open("r")) {
    cerr << "Failed to open list" << endl;
    return 0;
  }
  if (list.isEmpty()) {
    cout << "List is empty" << endl;
  }
  if (journal.open("r")) {
    cerr << "Failed to open journal" << endl;
    return 0;
  }
  if (journal.isEmpty()) {
    cout << "Journal is empty" << endl;
  }
  if (merge.open("w")) {
    cerr << "Failed to open merge" << endl;
    return 0;
  }
  if (merge.merge(list, journal)) {
    cerr << "Failed to merge: " << strerror(errno) << endl;
//     return 0;
  }
  merge.close();
  journal.close();
  list.close();

  cout << endl << "Test: merge read" << endl;
  my_time++;

  merge.open("r");
  if (merge.isEmpty()) {
    cout << "Merge is empty" << endl;
  } else
  while (merge.getEntry(&ts, &prefix, &path, &node) > 0) {
    cout << "Prefix: " << prefix << endl;
    cout << "Path:   " << path << endl;
    cout << "TS:     " << ts << endl;
    if (node == NULL) {
      cout << "Type:   removed" << endl;
    } else {
      switch (node->type()) {
        case 'f':
          cout << "Type:   file" << endl;
          break;
        case 'l':
          cout << "Type:   link" << endl;
          break;
        default:
          cout << "Type:   other" << endl;
      }
      cout << "Name:   " << node->name() << endl;
      cout << "Size:   " << node->size() << endl;
      switch (node->type()) {
        case 'f':
          cout << "Chcksm: " << ((File*) node)->checksum() << endl;
          break;
        case 'l':
          cout << "Link:   " << ((Link*) node)->link() << endl;
      }
      free(node);
      node = NULL;
    }
    cout << endl;
  }
  merge.close();

  cout << endl << "Test: prefix find" << endl;
  my_time++;

  merge.open("r");
  if (merge.isEmpty()) {
    cout << "Merge is empty" << endl;
  }
  if (! merge.findPrefix("path")) {
    cout << "prefix 'path' not found" << endl;
  } else {
    cout << "prefix 'path' found" << endl;
  }
  while (merge.getEntry(&ts, &prefix, &path, &node) > 0) {
    cout << "Prefix: " << prefix << endl;
    cout << "Path:   " << path << endl;
    cout << "TS:     " << ts << endl;
    if (node == NULL) {
      cout << "Type:   removed" << endl;
    } else {
      switch (node->type()) {
        case 'f':
          cout << "Type:   file" << endl;
          break;
        case 'l':
          cout << "Type:   link" << endl;
          break;
        default:
          cout << "Type:   other" << endl;
      }
      cout << "Name:   " << node->name() << endl;
      cout << "Size:   " << node->size() << endl;
      switch (node->type()) {
        case 'f':
          cout << "Chcksm: " << ((File*) node)->checksum() << endl;
          break;
        case 'l':
          cout << "Link:   " << ((Link*) node)->link() << endl;
      }
      free(node);
      node = NULL;
    }
    cout << endl;
  }
  merge.close();

  merge.open("r");
  if (merge.isEmpty()) {
    cout << "Merge is empty" << endl;
  }
  if (! merge.findPrefix("silly")) {
    cout << "prefix 'silly' not found" << endl;
  } else {
    cout << "prefix 'silly' found" << endl;
  }
  while (merge.getEntry(&ts, &prefix, &path, &node) > 0) {
    cout << "Prefix: " << prefix << endl;
    cout << "Path:   " << path << endl;
    cout << "TS:     " << ts << endl;
    if (node == NULL) {
      cout << "Type:   removed" << endl;
    } else {
      switch (node->type()) {
        case 'f':
          cout << "Type:   file" << endl;
          break;
        case 'l':
          cout << "Type:   link" << endl;
          break;
        default:
          cout << "Type:   other" << endl;
      }
      cout << "Name:   " << node->name() << endl;
      cout << "Size:   " << node->size() << endl;
      switch (node->type()) {
        case 'f':
          cout << "Chcksm: " << ((File*) node)->checksum() << endl;
          break;
        case 'l':
          cout << "Link:   " << ((Link*) node)->link() << endl;
      }
      free(node);
      node = NULL;
    }
    cout << endl;
  }
  merge.close();

  merge.open("r");
  if (merge.isEmpty()) {
    cout << "Merge is empty" << endl;
  }
  if (! merge.findPrefix("prefix2")) {
    cout << "prefix 'prefix2' not found" << endl;
  } else {
    cout << "prefix 'prefix2' found" << endl;
  }
  while (merge.getEntry(&ts, &prefix, &path, &node) > 0) {
    cout << "Prefix: " << prefix << endl;
    cout << "Path:   " << path << endl;
    cout << "TS:     " << ts << endl;
    if (node == NULL) {
      cout << "Type:   removed" << endl;
    } else {
      switch (node->type()) {
        case 'f':
          cout << "Type:   file" << endl;
          break;
        case 'l':
          cout << "Type:   link" << endl;
          break;
        default:
          cout << "Type:   other" << endl;
      }
      cout << "Name:   " << node->name() << endl;
      cout << "Size:   " << node->size() << endl;
      switch (node->type()) {
        case 'f':
          cout << "Chcksm: " << ((File*) node)->checksum() << endl;
          break;
        case 'l':
          cout << "Link:   " << ((Link*) node)->link() << endl;
      }
      free(node);
      node = NULL;
    }
    cout << endl;
  }
  merge.close();
  free(line);

  return 0;
}
