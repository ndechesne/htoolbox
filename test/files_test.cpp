/*
    Copyright (C) 2006 - 2011  Hervé Fache

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

#include "report.h"
#include "files.h"

using namespace htoolbox;

static int parseList(Node *d, const char* cur_dir) {
  list<Node*>::iterator i = d->nodesList().begin();
  while (i != d->nodesList().end()) {
    switch ((*i)->type()) {
      case 'd': {
        Path dir_path(cur_dir, (*i)->name());
        if (! (*i)->createList()) {
          parseList((*i), dir_path.c_str());
        } else {
          cerr << "Failed to create list for " << (*i)->name() << " in "
            << cur_dir << endl;
        }
      }
      break;
    }
    i++;
  }
  return 0;
}

static void showList(const Node& d, int level = 0);

static void showFile(const Node& g, int level = 0) {
  int level_no = level;
  printf(" ");
  while (level_no--) printf("-");
  printf("> ");
  const char* type;
  if (g.type() == 'd') {
    type = "Dir.";
  } else
  if (g.type() == 'f') {
    type = "File";
  } else
  if (g.type() == 'l') {
    type = "Link";
  } else
  {
    type = "Oth.";
  }
  printf("%s: %s, path = %s, type = %c, mtime = %d, size = %jd, uid = %d, "
         "gid = %d, mode = %03o",
         type, g.name(), g.path(), g.type(), g.mtime() != 0,
         (g.type() == 'd') ? 1 : g.size(),
         static_cast<int>(g.uid() != 0), static_cast<int>(g.gid() != 0),
         g.mode());
  if (g.link()[0] != '\0') {
    printf(", link = %s", g.link());
  }
  if (g.hash()[0] != '\0') {
    printf(", hash = %s", g.hash());
  }
  printf("\n");
  if (g.type() == 'd') {
    showList(g, level);
  }
}

static void showList(const Node& d, int level) {
  ++level;
  list<Node*>::const_iterator i;
  for (i = d.nodesListConst().begin(); i != d.nodesListConst().end(); i++) {
    if (! strcmp((*i)->name(), "cvs")) continue;
    if (! strcmp((*i)->name(), "svn")) continue;
    showFile(**i, level);
  }
}

static void createNshowFile(Node &g) {
  if (g.type() == 'd') {
    g.createList();
  }
  showFile(g);
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
  // trailing slashes
  pth0 = new Path("//////////12//////3/456////////", Path::NO_TRAILING_SLASHES);
  hlog_regression("Path (%zu) = '%s'", pth0->length(), pth0->c_str());
  delete pth0;
  // multiple slashes
  pth0 = new Path("//////////12//////3/456////////", Path::NO_REPEATED_SLASHES);
  hlog_regression("Path (%zu) = '%s'", pth0->length(), pth0->c_str());
  delete pth0;
  // multiple and trailing slashes
  pth0 = new Path("//////////12//////3/456////////",
    Path::NO_TRAILING_SLASHES | Path::NO_REPEATED_SLASHES);
  hlog_regression("Path (%zu) = '%s'", pth0->length(), pth0->c_str());
  delete pth0;
  //
  pth0 = new Path("//1123//456", "7899//11599//");
  cout << pth0->c_str() << endl;
  Path pth1(*pth0);
  if (pth1.c_str() != pth0->c_str()) {
    hlog_regression("buffers differ");
  }
  delete pth0;
  Node nod1(pth1);
  if (pth1.c_str() != nod1.path()) {
    hlog_regression("buffers differ");
  }

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


  cout << endl << "Test: pathcmp" << endl;
  cout << "a <> a: " << Path::pathcmp("a", "a") << endl;
  cout << "a <> b: " << Path::pathcmp("a", "b") << endl;
  cout << "b <> a: " << Path::pathcmp("b", "a") << endl;
  cout << "a1 <> b: " << Path::pathcmp("a1", "b") << endl;
  cout << "b <> a1: " << Path::pathcmp("b", "a1") << endl;
  cout << "a1 <> a: " << Path::pathcmp("a1", "a") << endl;
  cout << "a <> a1: " << Path::pathcmp("a", "a1") << endl;
  cout << "a/ <> a: " << Path::pathcmp("a/", "a") << endl;
  cout << "a <> a/: " << Path::pathcmp("a", "a/") << endl;
  cout << "a\t <> a/: " << Path::pathcmp("a\t", "a/") << endl;
  cout << "a/ <> a\t " << Path::pathcmp("a/", "a\t") << endl;
  cout << "a\t <> a\t " << Path::pathcmp("a\t", "a\t") << endl;
  cout << "a\n <> a/: " << Path::pathcmp("a\n", "a/") << endl;
  cout << "a/ <> a\n " << Path::pathcmp("a/", "a\n") << endl;
  cout << "a\n <> a\n " << Path::pathcmp("a\n", "a\n") << endl;
  cout << "a/ <> a.: " << Path::pathcmp("a/", "a.") << endl;
  cout << "a. <> a/: " << Path::pathcmp("a.", "a/") << endl;
  cout << "a/ <> a-: " << Path::pathcmp("a/", "a-") << endl;
  cout << "a- <> a/: " << Path::pathcmp("a-", "a/") << endl;
  cout << "a/ <> a/: " << Path::pathcmp("a/", "a/") << endl;
  cout << "a/ <> a+/: " << Path::pathcmp("a/", "a+/") << endl;
  cout << "a+/ <> a/: " << Path::pathcmp("a+/", "a/") << endl;
  cout << "abcd <> abce, 3: " << Path::pathcmp("abcd", "abce", 3) << endl;
  cout << "abcd <> abce, 4: " << Path::pathcmp("abcd", "abce", 4) << endl;
  cout << "abcd <> abce, 5: " << Path::pathcmp("abcd", "abce", 5) << endl;


  cout << endl << "File types" << endl;
  // File types
  Node *g;
  g = new Node("test1/testfile");
  showFile(*g);
  delete g;
  g = new Node("test1/testlink");
  showFile(*g);
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
  cout << "File exists? " << Node("test1/testfile").exists() << endl;
  cout << "File is file? " << Node("test1/testfile").isReg() << endl;
  cout << "File is dir? " << Node("test1/testfile").isDir() << endl;
  cout << "File is link? " << Node("test1/testfile").isLink() << endl;

  cout << "Dir exists? " << Node("test1/testdir").exists() << endl;
  cout << "Dir is file? " << Node("test1/testdir").isReg() << endl;
  cout << "Dir is dir? " << Node("test1/testdir").isDir() << endl;
  cout << "Dir is link? " << Node("test1/testdir").isLink() << endl;

  cout << "Link exists? " << Node("test1/testlink").exists() << endl;
  cout << "Link is file? " << Node("test1/testlink").isReg() << endl;
  cout << "Link is dir? " << Node("test1/testlink").isDir() << endl;
  cout << "Link is link? " << Node("test1/testlink").isLink() << endl;

  cout << endl << "Creation tests" << endl;
  cout << "File exists? " << Node("test1/touchedfile").exists() << endl;
  cout << "File is file? " << Node("test1/touchedfile").isReg() << endl;
  cout << "File is dir? " << Node("test1/touchedfile").isDir() << endl;
  cout << "File is link? " << Node("test1/touchedfile").isLink() << endl;

  cout << "Dir exists? " << Node("test1/toucheddir").exists() << endl;
  cout << "Dir is file? " << Node("test1/toucheddir").isReg() << endl;
  cout << "Dir is dir? " << Node("test1/toucheddir").isDir() << endl;
  cout << "Dir is link? " << Node("test1/toucheddir").isLink() << endl;

  cout << "Link exists? " << Node("test1/touchedlink").exists() << endl;
  cout << "Link is file? " << Node("test1/touchedlink").isReg() << endl;
  cout << "Link is dir? " << Node("test1/touchedlink").isDir() << endl;
  cout << "Link is link? " << Node("test1/touchedlink").isLink() << endl;

  cout << "Create" << endl;
  if (Node::touch("test1/touchedfile"))
    cout << "failed to create file: " << strerror(errno) << endl;
  if (Node("test1/toucheddir").mkdir())
    cout << "failed to create dir" << endl;

  cout << "File exists? " << Node("test1/touchedfile").exists() << endl;
  cout << "File is file? " << Node("test1/touchedfile").isReg() << endl;
  cout << "File is dir? " << Node("test1/touchedfile").isDir() << endl;
  cout << "File is link? " << Node("test1/touchedfile").isLink() << endl;

  cout << "Dir exists? " << Node("test1/toucheddir").exists() << endl;
  cout << "Dir is file? " << Node("test1/toucheddir").isReg() << endl;
  cout << "Dir is dir? " << Node("test1/toucheddir").isDir() << endl;
  cout << "Dir is link? " << Node("test1/toucheddir").isLink() << endl;

  cout << "Link exists? " << Node("test1/touchedlink").exists() << endl;
  cout << "Link is file? " << Node("test1/touchedlink").isReg() << endl;
  cout << "Link is dir? " << Node("test1/touchedlink").isDir() << endl;
  cout << "Link is link? " << Node("test1/touchedlink").isLink() << endl;

  cout << "Create again" << endl;
  if (Node::touch("test1/touchedfile"))
    cout << "failed to create file: " << strerror(errno) << endl;
  if (Node("test1/toucheddir").mkdir())
    cout << "failed to create dir: " << strerror(errno) << endl;

  cout << "File exists? " << Node("test1/touchedfile").exists() << endl;
  cout << "File is file? " << Node("test1/touchedfile").isReg() << endl;
  cout << "File is dir? " << Node("test1/touchedfile").isDir() << endl;
  cout << "File is link? " << Node("test1/touchedfile").isLink() << endl;

  cout << "Dir exists? " << Node("test1/toucheddir").exists() << endl;
  cout << "Dir is file? " << Node("test1/toucheddir").isReg() << endl;
  cout << "Dir is dir? " << Node("test1/toucheddir").isDir() << endl;
  cout << "Dir is link? " << Node("test1/toucheddir").isLink() << endl;

  cout << "Link exists? " << Node("test1/touchedlink").exists() << endl;
  cout << "Link is file? " << Node("test1/touchedlink").isReg() << endl;
  cout << "Link is dir? " << Node("test1/touchedlink").isDir() << endl;
  cout << "Link is link? " << Node("test1/touchedlink").isLink() << endl;

  cout << endl << "Parsing test" << endl;
  Node* d = new Node("test1");
  if (d->isDir()) {
    if (! d->createList()) {
      if (! parseList(d, "test1")) {
        showFile(*d);
      }
    } else {
      cerr << "Failed to create list: " << strerror(errno) << endl;
    }
  }
  delete d;


  cout << endl << "mkdir_p test" << endl;
  if (Node::mkdir_p("a/b/c/d", 0740) < 0) {
    hlog_regression("mkdir_p test 1 failed: %m");
  } else {
    hlog_regression("mkdir_p test 1 passed");
  }
  if (Node::mkdir_p("a/b/c/d", 0750) < 0) {
    hlog_regression("mkdir_p test 2 failed: %m");
  } else {
    hlog_regression("mkdir_p test 2 passed");
  }
  if (Node::mkdir_p("/a/b/c/d", 0700) >= 0) {
    hlog_regression("mkdir_p test 3 failed");
  } else {
    hlog_regression("mkdir_p test 3 passed: %m");
  }

  return 0;
}
