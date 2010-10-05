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
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>

using namespace std;

#include "hreport.h"
#include "files.h"
#include "filereaderwriter.h"
#include "unzipreader.h"
#include "zipwriter.h"
#include "hasher.h"

using namespace hbackup;
using namespace htools;

static int parseList(Directory *d, const char* cur_dir) {
  list<Node*>::iterator i = d->nodesList().begin();
  while (i != d->nodesList().end()) {
    Node* payload = *i;
    switch (payload->type()) {
      case 'd': {
        Directory *di = new Directory(*payload);
        delete *i;
        *i = di;
        Path dir_path(cur_dir, di->name());
        if (! di->createList()) {
          parseList(di, dir_path.c_str());
        } else {
          cerr << "Failed to create list for " << di->name() << " in "
            << cur_dir << endl;
        }
      }
      break;
    }
    i++;
  }
  return 0;
}

static void showList(const Directory& d, int level = 0);

static void defaultShowFile(const Node& g) {
  const char* type;
  if (g.type() == 'f') {
    type = "File";
  } else
  if (g.type() == 'l') {
    type = "Link";
  } else
  {
    type = "Oth.";
  }
  printf("%s: %s, path = %s, type = %c, mtime = %d, size = %lld, uid = %d, "
         "gid = %d, mode = %03o",
         type, g.name(), g.path(), g.type(), g.mtime() != 0, g.size(),
         static_cast<int>(g.uid() != 0), static_cast<int>(g.gid() != 0),
         g.mode());
  if (g.link()[0] != '\0') {
    printf(", link = %s", g.link());
  }
  printf("\n");
}

static void showFile(const Node& g, int level = 1) {
  int level_no = level;
  cout << " ";
  while (level_no--) cout << "-";
  cout << "> ";
  switch (g.type()) {
    case 'd': {
      const Directory& d = static_cast<const Directory&>(g);
      cout << "Dir.: " << d.name()
        << ", path = " << d.path()
        << ", type = " << d.type()
        << ", mtime = " << (d.mtime() != 0)
        << ", size = " << (d.size() != 0)
        << ", uid = " << static_cast<int>(d.uid() != 0)
        << ", gid = " << static_cast<int>(d.gid() != 0)
        << oct << ", mode = " << d.mode() << dec
        << endl;
      if (level) {
        showList(d, level);
      }
    } break;
    default:
      defaultShowFile(g);
  }
}

static void showList(const Directory& d, int level) {
  if (level == 0) {
    showFile(d, level);
  }
  ++level;
  list<Node*>::const_iterator i;
  for (i = d.nodesListConst().begin(); i != d.nodesListConst().end(); i++) {
    if (! strcmp((*i)->name(), "cvs")) continue;
    if (! strcmp((*i)->name(), "svn")) continue;
    if (! strcmp((*i)->name(), "bzr")) continue;
    showFile(**i, level);
  }
}

static void createNshowFile(const Node &g) {
  switch (g.type()) {
  case 'f':
  case 'l':
    defaultShowFile(g);
    break;
  case 'd': {
    Directory d(g);
    d.createList();
    showList(d);
    } break;
  default:
    cout << "Unknown file type: " << g.type() << endl;
  }
}

bool cancel(unsigned short __unused) {
  (void) __unused;
  return true;
}

int main(void) {
  report.setLevel(regression);

  string line;

  cout << "Tools Test" << endl;

  mode_t mask = umask(0022);
  mask = umask(0022);
  printf("Our mask = 0%03o\n", mask);

  cout << endl << "Test: constructors" << endl;
  Path* pth0;
  pth0 = new Path("");
  cout << pth0->c_str() << endl;
  delete pth0;
  pth0 = new Path("123");
  cout << pth0->c_str() << endl;
  delete pth0;
  pth0 = new Path("123/456", "");
  cout << pth0->c_str() << endl;
  delete pth0;
  pth0 = new Path("", "789");
  cout << pth0->c_str() << endl;
  delete pth0;
  pth0 = new Path(Path("123", "456").c_str(), "789");
  cout << pth0->c_str() << endl;
  delete pth0;
  pth0 = new Path("123/456", "789/159");
  cout << pth0->c_str() << endl;
  delete pth0;
  pth0 = new Path("//1123//456", "7899//11599//");
  cout << pth0->c_str() << endl;
  delete pth0;

  cout << endl << "Test: basename and dirname" << endl;
  pth0 = new Path("this/is a path/to a/file");
  cout << pth0->c_str();
  cout << " -> base: ";
  cout << basename(pth0->c_str());
  cout << ", dir: ";
  cout << pth0->dirname().c_str();
  cout << endl;
  delete pth0;
  pth0 = new Path("this is a file");
  cout << pth0->c_str();
  cout << " -> base: ";
  cout << basename(pth0->c_str());
  cout << ", dir: ";
  cout << pth0->dirname().c_str();
  cout << endl;
  delete pth0;
  pth0 = new Path("this is a path/");
  cout << pth0->c_str();
  cout << " -> base: ";
  cout << basename(pth0->c_str());
  cout << ", dir: ";
  cout << pth0->dirname().c_str();
  cout << endl;
  delete pth0;

  cout << endl << "Test: fromDos" << endl;
  char c_str[256];
  strcpy(c_str, "a:\\backup\\");
  cout << c_str << " -> ";
  cout << Path::fromDos(c_str) << endl;
  strcpy(c_str, "z:/backup");
  cout << c_str << " -> ";
  cout << Path::fromDos(c_str) << endl;
  strcpy(c_str, "1:/backup");
  cout << c_str << " -> ";
  cout << Path::fromDos(c_str) << endl;
  strcpy(c_str, "f;/backup");
  cout << c_str << " -> ";
  cout << Path::fromDos(c_str) << endl;
  strcpy(c_str, "l:|backup");
  cout << c_str << " -> ";
  cout << Path::fromDos(c_str) << endl;
  strcpy(c_str, "this\\is a path/ Unix\\ cannot cope with/\\");
  cout << c_str << " -> ";
  cout << Path::fromDos(c_str) << endl;
  cout << c_str << " -> ";
  cout << Path::fromDos(c_str) << endl;

  cout << endl << "Test: noTrailingSlashes" << endl;
  strcpy(c_str, "this/is a path/ Unix/ cannot cope with//");
  cout << c_str << " -> ";
  cout << Path::noTrailingSlashes(c_str) << endl;
  Path::fromDos(c_str);
  cout << c_str << " -> ";
  cout << Path::noTrailingSlashes(c_str) << endl;
  strcpy(c_str, "");
  cout << c_str << " -> ";
  cout << Path::noTrailingSlashes(c_str) << endl;
  strcpy(c_str, "/");
  cout << c_str << " -> ";
  cout << Path::noTrailingSlashes(c_str) << endl;

  cout << endl << "Test: compare" << endl;
  cout << "a <> a: " << Path::compare("a", "a") << endl;
  cout << "a <> b: " << Path::compare("a", "b") << endl;
  cout << "b <> a: " << Path::compare("b", "a") << endl;
  cout << "a1 <> b: " << Path::compare("a1", "b") << endl;
  cout << "b <> a1: " << Path::compare("b", "a1") << endl;
  cout << "a1 <> a: " << Path::compare("a1", "a") << endl;
  cout << "a <> a1: " << Path::compare("a", "a1") << endl;
  cout << "a/ <> a: " << Path::compare("a/", "a") << endl;
  cout << "a <> a/: " << Path::compare("a", "a/") << endl;
  cout << "a\t <> a/: " << Path::compare("a\t", "a/") << endl;
  cout << "a/ <> a\t " << Path::compare("a/", "a\t") << endl;
  cout << "a\t <> a\t " << Path::compare("a\t", "a\t") << endl;
  cout << "a\n <> a/: " << Path::compare("a\n", "a/") << endl;
  cout << "a/ <> a\n " << Path::compare("a/", "a\n") << endl;
  cout << "a\n <> a\n " << Path::compare("a\n", "a\n") << endl;
  cout << "a/ <> a.: " << Path::compare("a/", "a.") << endl;
  cout << "a. <> a/: " << Path::compare("a.", "a/") << endl;
  cout << "a/ <> a-: " << Path::compare("a/", "a-") << endl;
  cout << "a- <> a/: " << Path::compare("a-", "a/") << endl;
  cout << "a/ <> a/: " << Path::compare("a/", "a/") << endl;
  cout << "abcd <> abce, 3: " << Path::compare("abcd", "abce", 3) << endl;
  cout << "abcd <> abce, 4: " << Path::compare("abcd", "abce", 4) << endl;
  cout << "abcd <> abce, 5: " << Path::compare("abcd", "abce", 5) << endl;


  cout << endl << "File types" << endl;
  // File types
  Node *g;
  g = new Node("test1/testfile");
  createNshowFile(*g);
  delete g;
  g = new Node("test1/testlink");
  createNshowFile(*g);
  delete g;
  g = new Node("test1/testdir");
  createNshowFile(*g);
  delete g;
  g = new Node("test1/subdir");
  createNshowFile(*g);
  delete g;


  cout << endl << "Link change" << endl;
  Node* l = new Node("test1/testlink");
  showFile(*l);
  char max_link[PATH_MAX];
  memset(max_link, '?', sizeof(max_link) - 1);
  max_link[sizeof(max_link) - 1] = '\0';
  l->setLink(max_link);
  showFile(*l);
  delete l;


  cout << endl << "Validity tests" << endl;
  cout << "File is file? " << Node("test1/testfile").isReg() << endl;
  cout << "File is dir? " << Directory("test1/testfile").isValid() << endl;
  cout << "File is link? " << Node("test1/testfile").isLink() << endl;
  cout << "Dir is file? " << Node("test1/testdir").isReg() << endl;
  cout << "Dir is dir? " << Directory("test1/testdir").isValid() << endl;
  cout << "Dir is link? " << Node("test1/testdir").isLink() << endl;
  cout << "Link is file? " << Node("test1/testlink").isReg() << endl;
  cout << "Link is dir? " << Directory("test1/testlink").isValid() << endl;
  cout << "Link is link? " << Node("test1/testlink").isLink() << endl;

  cout << endl << "Creation tests" << endl;
  cout << "File is file? " << Node("test1/touchedfile").isReg() << endl;
  cout << "File is dir? " << Directory("test1/touchedfile").isValid() << endl;
  cout << "File is link? " << Node("test1/touchedfile").isLink() << endl;
  cout << "Dir is file? " << Node("test1/toucheddir").isReg() << endl;
  cout << "Dir is dir? " << Directory("test1/toucheddir").isValid() << endl;
  cout << "Dir is link? " << Node("test1/toucheddir").isLink() << endl;
  cout << "Link is file? " << Node("test1/touchedlink").isReg() << endl;
  cout << "Link is dir? " << Directory("test1/touchedlink").isValid() << endl;
  cout << "Link is link? " << Node("test1/touchedlink").isLink() << endl;

  cout << "Create" << endl;
  if (Node::touch("test1/touchedfile"))
    cout << "failed to create file: " << strerror(errno) << endl;
  if (Directory("test1/toucheddir").create())
    cout << "failed to create dir" << endl;

  cout << "File is file? " << Node("test1/touchedfile").isReg() << endl;
  cout << "File is dir? " << Directory("test1/touchedfile").isValid() << endl;
  cout << "File is link? " << Node("test1/touchedfile").isLink() << endl;
  cout << "Dir is file? " << Node("test1/toucheddir").isReg() << endl;
  cout << "Dir is dir? " << Directory("test1/toucheddir").isValid() << endl;
  cout << "Dir is link? " << Node("test1/toucheddir").isLink() << endl;
  cout << "Link is file? " << Node("test1/touchedlink").isReg() << endl;
  cout << "Link is dir? " << Directory("test1/touchedlink").isValid() << endl;
  cout << "Link is link? " << Node("test1/touchedlink").isLink() << endl;

  cout << "Create again" << endl;
  if (Node::touch("test1/touchedfile"))
    cout << "failed to create file: " << strerror(errno) << endl;
  if (Directory("test1/toucheddir").create())
    cout << "failed to create dir: " << strerror(errno) << endl;

  cout << "File is file? " << Node("test1/touchedfile").isReg() << endl;
  cout << "File is dir? " << Directory("test1/touchedfile").isValid() << endl;
  cout << "File is link? " << Node("test1/touchedfile").isLink() << endl;
  cout << "Dir is file? " << Node("test1/toucheddir").isReg() << endl;
  cout << "Dir is dir? " << Directory("test1/toucheddir").isValid() << endl;
  cout << "Dir is link? " << Node("test1/toucheddir").isLink() << endl;
  cout << "Link is file? " << Node("test1/touchedlink").isReg() << endl;
  cout << "Link is dir? " << Directory("test1/touchedlink").isValid() << endl;
  cout << "Link is link? " << Node("test1/touchedlink").isLink() << endl;

  cout << endl << "Parsing test" << endl;

  Directory* d = new Directory("test1");
  if (d->isValid()) {
    if (! d->createList()) {
      if (! parseList(d, "test1")) {
        showList(*d);
      }
    } else {
      cerr << "Failed to create list: " << strerror(errno) << endl;
    }
  }
  delete d;


  hlog_regression("Directory::createList() performance test");
  {
    struct timeval tm_start;
    struct timeval tm_end;
    Directory d(".");
    if (d.createList() < 0) return 0;
    d.deleteList();
    gettimeofday(&tm_start, NULL);
    const int MAX = 10000;
    for (int i = 0; i < MAX; ++i) {
      if (d.createList() < 0) return 0;
      d.deleteList();
    }
    gettimeofday(&tm_end, NULL);
    int diff = (tm_end.tv_sec - tm_start.tv_sec) * 1000000 +
                (tm_end.tv_usec - tm_start.tv_usec);
    const int MAX_US = 90;
    hlog_regression("duration <= %d us? %s",
                    MAX_US, diff < MAX_US * MAX ? "yes" : "no");
  }

  return 0;
}
