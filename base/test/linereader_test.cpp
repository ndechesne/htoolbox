/*
     Copyright (C) 2010  Herve Fache

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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <iostream>

using namespace std;

#include "hreport.h"
#include "filereaderwriter.h"
#include "zipwriter.h"
#include "unzipreader.h"
#include "linereader.h"

using namespace htools;

int main() {
  report.setLevel(regression);

  IReaderWriter* fr;
  LineReader*    readfile;

  IReaderWriter* writefile;

  char* line_test;
  size_t line_test_capacity;


  writefile = new FileReaderWriter("test2/testfile", true);
  if (writefile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->write(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  delete writefile;

  fr = new FileReaderWriter("test2/testfile", false);
  readfile = new LineReader(fr, true);
  line_test = NULL;
  line_test_capacity = 0;
  readfile->open();
  cout << "Reading uncompressed empty file:" << endl;
  while (readfile->getLine(&line_test, &line_test_capacity) > 0) {
    hlog_regression("Line: '%s'", line_test);
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  delete readfile;


  writefile = new FileReaderWriter("test2/testfile", true);
  if (writefile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->write("abcdef\nghi\njkl", 14);
    writefile->write(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  delete writefile;

  fr = new FileReaderWriter("test2/testfile", false);
  readfile = new LineReader(fr, true);
  if (readfile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  }
  cout << "Reading uncompressed file:" << endl;
  while (readfile->getLine(&line_test, &line_test_capacity) > 0) {
    hlog_regression("Line: '%s'", line_test);
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  delete readfile;

  free(line_test);


  // With compression

  writefile = new FileReaderWriter("test2/testfile.gz", true);
  writefile = new ZipWriter(writefile, true, 5);
  if (writefile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->write(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  delete writefile;

  fr = new FileReaderWriter("test2/testfile.gz", false);
  fr = new UnzipReader(fr, true);
  readfile = new LineReader(fr, true);
  line_test = NULL;
  line_test_capacity = 0;
  readfile->open();
  cout << "Reading compressed empty file:" << endl;
  while (readfile->getLine(&line_test, &line_test_capacity) > 0) {
    hlog_regression("Line: '%s'", line_test);
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  delete readfile;


  writefile = new FileReaderWriter("test2/testfile.gz", true);
  writefile = new ZipWriter(writefile, true, 5);
  if (writefile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->write("abcdef\nghi\njkl", 14);
    writefile->write(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  delete writefile;

  fr = new FileReaderWriter("test2/testfile.gz", false);
  fr = new UnzipReader(fr, true);
  readfile = new LineReader(fr, true);
  if (readfile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  }
  cout << "Reading compressed file:" << endl;
  while (readfile->getLine(&line_test, &line_test_capacity) > 0) {
    hlog_regression("Line: '%s'", line_test);
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  delete readfile;

  free(line_test);


  return 0;
}