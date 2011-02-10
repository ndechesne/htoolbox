/*
    Copyright (C) 2010 - 2011  Hervé Fache

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <iostream>

using namespace std;

#include "report.h"
#include "filereaderwriter.h"
#include "zipwriter.h"
#include "unzipreader.h"
#include "linereaderwriter.h"

using namespace htoolbox;

int main() {
  report.setLevel(regression);

  IReaderWriter* fr;
  LineReaderWriter*    readfile;

  IReaderWriter* writefile;

  char* line_test;
  size_t line_test_capacity;
  ssize_t line_size;


  hlog_regression("Empty file");

  writefile = new FileReaderWriter("lineread", true);
  if (writefile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  delete writefile;

  fr = new FileReaderWriter("lineread", false);
  readfile = new LineReaderWriter(fr, true);
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


  hlog_regression("Simple file");

  writefile = new FileReaderWriter("lineread", true);
  if (writefile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->put("abcdef\nghi\njkl", 14);
    writefile->put(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  delete writefile;

  fr = new FileReaderWriter("lineread", false);
  readfile = new LineReaderWriter(fr, true);
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


  hlog_regression("Compressed empty file");

  writefile = new FileReaderWriter("lineread.gz", true);
  writefile = new ZipWriter(writefile, true, 5);
  if (writefile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  delete writefile;

  fr = new FileReaderWriter("lineread.gz", false);
  fr = new UnzipReader(fr, true);
  readfile = new LineReaderWriter(fr, true);
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


  hlog_regression("Compressed simple file");

  writefile = new FileReaderWriter("lineread.gz", true);
  writefile = new ZipWriter(writefile, true, 5);
  LineReaderWriter* writeline = new LineReaderWriter(writefile, false);
  if (writeline->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writeline->putLine("abcdef", 6);
    writeline->putLine("ghi", 3);
    writeline->put("jkl", 3);
    if (writeline->close()) cout << "Error closing write file" << endl;
  }
  delete writefile;
  delete writeline;

  fr = new FileReaderWriter("lineread.gz", false);
  fr = new UnzipReader(fr, true);
  readfile = new LineReaderWriter(fr, true);
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


  hlog_regression("Compressed simple file, getDelim()");

  writefile = new FileReaderWriter("lineread.gz", true);
  writefile = new ZipWriter(writefile, true, 5);
  if (writefile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->put("abcdef\b\nghi\b\njkl", 16);
    writefile->put(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  delete writefile;

  fr = new FileReaderWriter("lineread.gz", false);
  fr = new UnzipReader(fr, true);
  readfile = new LineReaderWriter(fr, true);
  if (readfile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  }
  cout << "Reading compressed file:" << endl;
  while ((line_size = readfile->getLine(&line_test, &line_test_capacity, '\b')) > 0) {
    hlog_regression("Line[%zu] (%zd): '%s'",
      line_test_capacity, line_size, line_test);
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  delete readfile;


  hlog_regression("Compressed huge file with huge lines, getDelim()");

  char line[200000];
  for (size_t i = 0; i < sizeof(line); ++i) {
    line[i] = static_cast<char>((rand() & 0x3f) + 0x20);
  }
  writefile = new FileReaderWriter("lineread.gz", true);
  writefile = new ZipWriter(writefile, true, 5);
  writeline = new LineReaderWriter(writefile, false);
  if (writeline->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    for (size_t i = 1; i < 20; ++i) {
      writeline->put("\n", 1);
      writeline->putLine(line, sizeof(line) * i / 20 - 2, '\b');
    }
    if (writeline->close()) cout << "Error closing write file" << endl;
  }
  delete writeline;
  delete writefile;

  fr = new FileReaderWriter("lineread.gz", false);
  fr = new UnzipReader(fr, true);
  readfile = new LineReaderWriter(fr, true);
  if (readfile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  }
  cout << "Reading compressed big file:" << endl;
  while ((line_size = readfile->getLine(&line_test, &line_test_capacity, '\b')) > 0) {
    bool ok = (line_test[0] == '\n') &&
              (memcmp(line, &line_test[1], line_size - 2) == 0) &&
              (line_test[line_size - 1] == '\b');
    printf("Line[%zu] (%zd): %s\n",
      line_test_capacity, line_size, ok ? "ok" : "ko");
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  delete readfile;


  hlog_regression("Same file, getDelim() / read() / getDelim()");

  fr = new FileReaderWriter("lineread.gz", false);
  fr = new UnzipReader(fr, true);
  readfile = new LineReaderWriter(fr, true);
  if (readfile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  }
  cout << "Reading compressed big file:" << endl;
  size_t iteration = 0;
  while ((line_size = readfile->getLine(&line_test, &line_test_capacity, '\b')) > 0) {
    ++iteration;
    bool ok;
    if (iteration == 17) {
      ok = (memcmp(&line[10 - 1], line_test, line_size - 2) == 0) &&
           (line_test[line_size - 1] == '\b');
    } else
    if (iteration == 19) {
      ok = (memcmp(&line[120000 - 1], line_test, line_size - 2) == 0) &&
           (line_test[line_size - 1] == '\b');
    } else
    {
      ok = (line_test[0] == '\n') &&
           (memcmp(line, &line_test[1], line_size - 2) == 0) &&
           (line_test[line_size - 1] == '\b');
    }
    printf("Line[%zu] (%zd): %s\n",
      line_test_capacity, line_size, ok ? "ok" : "ko");
    if (iteration == 16) {
      line_size = readfile->get(line_test, 10);
      line_test[line_size] = '\0';
      ok = (line_test[0] == '\n') &&
           (memcmp(line, &line_test[1], line_size - 2) == 0);
      hlog_regression("read %zd bytes: %s", line_size, ok ? "ok" : "ko");
    } else
    if (iteration == 18) {
      line_size = readfile->get(line_test, 120000);
      line_test[line_size] = '\0';
      ok = (line_test[0] == '\n') &&
           (memcmp(line, &line_test[1], line_size - 2) == 0);
      hlog_regression("read %zd bytes: %s", line_size, ok ? "ok" : "ko");
    }
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  delete readfile;

  free(line_test);

  return 0;
}
