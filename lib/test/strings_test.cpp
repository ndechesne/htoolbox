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

using namespace std;

#include <stdio.h>
#include <stdlib.h>

#include "strings.h"

using namespace hbackup;

int main(void) {
  cout << endl << endl << "StrPath test" << endl;
  cout << endl << "constructors" << endl;
  StrPath* pth0;
  pth0 = new StrPath();
  cout << pth0->length() << ": " << pth0->c_str() << endl;
  delete pth0;
  pth0 = new StrPath("");
  cout << pth0->length() << ": " << pth0->c_str() << endl;
  delete pth0;
  pth0 = new StrPath("123");
  cout << pth0->length() << ": " << pth0->c_str() << endl;
  delete pth0;
  pth0 = new StrPath("123/456", "");
  cout << pth0->length() << ": " << pth0->c_str() << endl;
  delete pth0;
  pth0 = new StrPath("", "789");
  cout << pth0->length() << ": " << pth0->c_str() << endl;
  delete pth0;
  pth0 = new StrPath("123/456", "789");
  cout << pth0->length() << ": " << pth0->c_str() << endl;
  delete pth0;

  cout << endl << "compare" << endl;
  cout << "a <> a: " << StrPath("a").compare("a") << endl;
  cout << "a <> b: " << StrPath("a").compare("b") << endl;
  cout << "b <> a: " << StrPath("b").compare("a") << endl;
  cout << "a1 <> b: " << StrPath("a1").compare("b") << endl;
  cout << "b <> a1: " << StrPath("b").compare("a1") << endl;
  cout << "a1 <> a: " << StrPath("a1").compare("a") << endl;
  cout << "a <> a1: " << StrPath("a").compare("a1") << endl;
  cout << "a/ <> a: " << StrPath("a/").compare("a") << endl;
  cout << "a <> a/: " << StrPath("a").compare("a/") << endl;
  cout << "a\t <> a/: " << StrPath("a\t").compare("a/") << endl;
  cout << "a/ <> a\t " << StrPath("a/").compare("a\t") << endl;
  cout << "a\t <> a\t " << StrPath("a\t").compare("a\t") << endl;
  cout << "a\n <> a/: " << StrPath("a\n").compare("a/") << endl;
  cout << "a/ <> a\n " << StrPath("a/").compare("a\n") << endl;
  cout << "a\n <> a\n " << StrPath("a\n").compare("a\n") << endl;
  cout << "a/ <> a.: " << StrPath("a/").compare("a.") << endl;
  cout << "a. <> a/: " << StrPath("a.").compare("a/") << endl;
  cout << "a/ <> a-: " << StrPath("a/").compare("a-") << endl;
  cout << "a- <> a/: " << StrPath("a-").compare("a/") << endl;
  cout << "a/ <> a/: " << StrPath("a/").compare("a/") << endl;
  cout << "abcd <> abce, 3: " << StrPath("abcd").compare("abce", 3) << endl;
  cout << "abcd <> abce, 4: " << StrPath("abcd").compare("abce", 4) << endl;
  cout << "abcd <> abce, 5: " << StrPath("abcd").compare("abce", 5) << endl;

  cout << endl << "Test: toUnix" << endl;
  StrPath pth1 = "a:\\backup\\";
  cout << pth1.c_str() << " -> ";
  cout << pth1.toUnix().c_str() << endl;
  pth1 = "z:/backup";
  cout << pth1.c_str() << " -> ";
  cout << pth1.toUnix().c_str() << endl;
  pth1 = "1:/backup";
  cout << pth1.c_str() << " -> ";
  cout << pth1.toUnix().c_str() << endl;
  pth1 = "f;/backup";
  cout << pth1.c_str() << " -> ";
  cout << pth1.toUnix().c_str() << endl;
  pth1 = "l:|backup";
  cout << pth1.c_str() << " -> ";
  cout << pth1.toUnix().c_str() << endl;
  pth1 = "this\\is a path/ Unix\\ cannot cope with/\\";
  cout << pth1.c_str() << " -> ";
  cout << pth1.toUnix().c_str() << endl;
  cout << pth1.c_str() << " -> ";
  cout << pth1.toUnix().c_str() << endl;

  cout << endl << "Test: noEndingSlash" << endl;
  cout << pth1.c_str() << " -> ";
  cout << pth1.noEndingSlash().c_str() << endl;
  cout << pth1.c_str() << " -> ";
  cout << pth1.noEndingSlash().c_str() << endl;
  pth1 = "";
  cout << pth1.c_str() << " -> ";
  cout << pth1.noEndingSlash().c_str() << endl;
  pth1 = "/";
  cout << pth1.c_str() << " -> ";
  cout << pth1.noEndingSlash().c_str() << endl;

  cout << endl << "Test: basename and dirname" << endl;
  pth1 = "this/is a path/to a/file";
  cout << pth1.c_str();
  cout << " -> base: ";
  cout << pth1.basename().c_str();
  cout << ", dir: ";
  cout << pth1.dirname().c_str();
  cout << endl;
  pth1 = "this is a file";
  cout << pth1.c_str();
  cout << " -> base: ";
  cout << pth1.basename().c_str();
  cout << ", dir: ";
  cout << pth1.dirname().c_str();
  cout << endl;
  pth1 = "this is a path/";
  cout << pth1.c_str();
  cout << " -> base: ";
  cout << pth1.basename().c_str();
  cout << ", dir: ";
  cout << pth1.dirname().c_str();
  cout << endl;

  cout << endl << "Test: comparators" << endl;
  pth1 = "this is a path/";
  StrPath pth2 = "this is a path.";
  cout << pth1.c_str() << " == " << pth2.c_str() << ": " << int(pth1 == pth2)
    << endl;
  cout << pth1.c_str() << " != " << pth2.c_str() << ": " << int(pth1 != pth2)
    << endl;
  cout << pth1.c_str() << " < " << pth2.c_str() << ": " << int(pth1 < pth2)
    << endl;
  cout << pth1.c_str() << " > " << pth2.c_str() << ": " << int(pth1 > pth2)
    << endl;
  string str2 = "this is a path#";
  cout << pth1.c_str() << " < " << str2.c_str() << ": " << int(pth1 < str2)
    << endl;
  cout << pth1.c_str() << " > " << str2.c_str() << ": " << int(pth1 > str2)
    << endl;
  cout << pth1.c_str() << " < " << "this is a path-" << ": "
    << int(pth1 < "this is a path-") << endl;
  cout << pth1.c_str() << " > " << "this is a path-" << ": "
    << int(pth1 > "this is a path-") << endl;
  pth2 = "this is a path to somewhere";
  cout << pth1.c_str() << " == " << pth2.c_str() << ": " << int(pth1 == pth2)
    << endl;
  cout << pth1.c_str() << " != " << pth2.c_str() << ": " << int(pth1 != pth2)
    << endl;
  cout << pth1.c_str() << " <= " << pth2.c_str() << ": " << int(pth1 <= pth2)
    << endl;
  cout << pth1.c_str() << " >= " << pth2.c_str() << ": " << int(pth1 >= pth2)
    << endl;

  return 0;
}
