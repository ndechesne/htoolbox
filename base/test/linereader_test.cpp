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

#include <stdio.h>
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
  ssize_t line_size;


  writefile = new FileReaderWriter("lineread", true);
  if (writefile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->write(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  delete writefile;

  fr = new FileReaderWriter("lineread", false);
  readfile = new LineReader(fr, true);
  line_test = NULL;
  line_test_capacity = 0;
  readfile->open();
  cout << "Reading uncompressed empty file:" << endl;
  while ((line_size = readfile->getLine(&line_test, &line_test_capacity)) > 0) {
    hlog_regression("Line[%zu] (%zd): '%s'",
      line_test_capacity, line_size, line_test);
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  delete readfile;


  writefile = new FileReaderWriter("lineread", true);
  if (writefile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->write("abcdef\nghi\njkl", 14);
    writefile->write(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  delete writefile;

  fr = new FileReaderWriter("lineread", false);
  readfile = new LineReader(fr, true);
  if (readfile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  }
  cout << "Reading uncompressed file:" << endl;
  while ((line_size = readfile->getLine(&line_test, &line_test_capacity)) > 0) {
    hlog_regression("Line[%zu] (%zd): '%s'",
      line_test_capacity, line_size, line_test);
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  delete readfile;

  free(line_test);


  // With compression

  writefile = new FileReaderWriter("lineread.gz", true);
  writefile = new ZipWriter(writefile, true, 5);
  if (writefile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->write(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  delete writefile;

  fr = new FileReaderWriter("lineread.gz", false);
  fr = new UnzipReader(fr, true);
  readfile = new LineReader(fr, true);
  line_test = NULL;
  line_test_capacity = 0;
  readfile->open();
  cout << "Reading compressed empty file:" << endl;
  while ((line_size = readfile->getLine(&line_test, &line_test_capacity)) > 0) {
    hlog_regression("Line[%zu] (%zd): '%s'",
      line_test_capacity, line_size, line_test);
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  delete readfile;


  writefile = new FileReaderWriter("lineread.gz", true);
  writefile = new ZipWriter(writefile, true, 5);
  if (writefile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->write("abcdef\nghi\njkl", 14);
    writefile->write(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  delete writefile;

  fr = new FileReaderWriter("lineread.gz", false);
  fr = new UnzipReader(fr, true);
  readfile = new LineReader(fr, true);
  if (readfile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  }
  cout << "Reading compressed file:" << endl;
  while ((line_size = readfile->getLine(&line_test, &line_test_capacity)) > 0) {
    hlog_regression("Line[%zu] (%zd): '%s'",
      line_test_capacity, line_size, line_test);
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  delete readfile;


  writefile = new FileReaderWriter("lineread.gz", true);
  writefile = new ZipWriter(writefile, true, 5);
  if (writefile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->write("abcdef\b\nghi\b\njkl", 16);
    writefile->write(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  delete writefile;

  fr = new FileReaderWriter("lineread.gz", false);
  fr = new UnzipReader(fr, true);
  readfile = new LineReader(fr, true);
  if (readfile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  }
  cout << "Reading compressed file:" << endl;
  while ((line_size = readfile->getDelim(&line_test, &line_test_capacity, '\b')) > 0) {
    hlog_regression("Line[%zu] (%zd): '%s'",
      line_test_capacity, line_size, line_test);
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  delete readfile;


  // Create line bigger than internal buffer
  char line[200000];
  for (size_t i = 0; i < sizeof(line); ++i) {
    line[i] = static_cast<char>((rand() & 0x3f) + 0x20);
  }
  writefile = new FileReaderWriter("lineread.gz", true);
  writefile = new ZipWriter(writefile, true, 5);
  if (writefile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    for (size_t i = 1; i < 20; ++i) {
      writefile->write("\n", 1);
      writefile->write(line, sizeof(line) * i / 20 - 2);
      writefile->write("\b", 1);
    }
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  delete writefile;

  fr = new FileReaderWriter("lineread.gz", false);
  fr = new UnzipReader(fr, true);
  readfile = new LineReader(fr, true);
  if (readfile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  }
  cout << "Reading compressed big file:" << endl;
  while ((line_size = readfile->getDelim(&line_test, &line_test_capacity, '\b')) > 0) {
    bool ok = (line_test[0] == '\n') &&
              (memcmp(line, &line_test[1], line_size - 2) == 0) &&
              (line_test[line_size - 1] == '\b');
    printf("Line[%zu] (%zd): %s\n",
      line_test_capacity, line_size, ok ? "ok" : "ko");
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  delete readfile;


  free(line_test);


  return 0;
}