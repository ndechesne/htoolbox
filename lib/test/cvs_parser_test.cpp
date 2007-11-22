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
#include <sys/stat.h>

using namespace std;

#include "strings.h"
#include "files.h"
#include "parsers.h"
#include "cvs_parser.h"
#include "hbackup.h"

using namespace hbackup;

int hbackup::verbosity(void) {
  return 2;
}

int hbackup::terminating(const char* function) {
  return 0;
}

int main(void) {
  Parser*     parser_list;
  Parser*     parser;
  Parser*     parser2;
  Node*       node;

  // Create pseudo parsers list member
  parser_list = new CvsParser(Parser::controlled);

  /* Directory */
  if ((parser = parser_list->isControlled("test1")) == NULL) {
    cout << "test1" << " is not under CVS control" << endl;
  } else {
    parser->list();
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->isControlled("test1/cvs")) == NULL) {
    cout << "test1/cvs" << " is not under CVS control" << endl;
  } else {
    parser->list();
    /* Nodes */
    node = new Node("test1/cvs/nofile", 'f', 0, 0, 0, 0, 0);
    if (parser->ignore(*node)) {
      cout << "test1/cvs/nofile" << " is not under CVS control" << endl;
    }
    delete node;
    node = new Node("test1/cvs/filenew.c", 'f', 0, 0, 0, 0, 0);
    if (parser->ignore(*node)) {
      cout << "test1/cvs/filenew.c" << " is not under CVS control" << endl;
    }
    delete node;
    node = new Node("test1/cvs/filemod.o", 'f', 0, 0, 0, 0, 0);
    if (parser->ignore(*node)) {
      cout << "test1/cvs/filemod.o" << " is not under CVS control" << endl;
    }
    delete node;
    node = new Node("test1/cvs/fileutd.h", 'f', 0, 0, 0, 0, 0);
    if (parser->ignore(*node)) {
      cout << "test1/cvs/fileutd.h" << " is not under CVS control" << endl;
    }
    delete node;
    node = new Node("test1/cvs/fileoth", 'f', 0, 0, 0, 0, 0);
    if (parser->ignore(*node)) {
      cout << "test1/cvs/fileoth" << " is not under CVS control" << endl;
    }
    delete node;
    node = new Node("test1/cvs/dirutd", 'd', 0, 0, 0, 0, 0);
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirutd" << " is not under CVS control" << endl;
    }
    delete node;
    node = new Node("test1/cvs/diroth", 'd', 0, 0, 0, 0, 0);
    if (parser->ignore(*node)) {
      cout << "test1/cvs/diroth" << " is not under CVS control" << endl;
    }
    delete node;
    node = new Node("test1/cvs/CVS", 'd', 0, 0, 0, 0, 0);
    if (parser->ignore(*node)) {
      cout << "test1/cvs/CVS" << " is not under CVS control" << endl;
    }
    delete node;
    if ((parser2 = parser->isControlled("test1/cvs/CVS")) == NULL) {
      cout << "test1/cvs/CVS" << " is not under CVS control" << endl;
    } else {
      delete parser2;
    }
    // Use parser so directory appears as child of controlled directory
    if ((parser2 = parser->isControlled("test1/cvs/diroth")) == NULL) {
      cout << "test1/cvs/diroth" << " is not under CVS control" << endl;
    } else {
      delete parser2;
    }
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->isControlled("test1/cvs/dirutd")) == NULL) {
    cout << "test1/cvs/dirutd" << " is not under CVS control" << endl;
  } else {
    parser->list();
    /* Nodes */
    node = new Node("test1/cvs/dirutd/fileutd", 'f', 0, 0, 0, 0, 0);
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirutd/fileutd" << " is not under CVS control" << endl;
    }
    delete node;
    node = new Node("test1/cvs/dirutd/fileoth", 'f', 0, 0, 0, 0, 0);
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirutd/fileoth" << " is not under CVS control" << endl;
    }
    delete parser;
    delete node;
  }

  /* Directory */
  if ((parser = parser_list->isControlled("test1/cvs/dirbad")) == NULL) {
    cout << "test1/cvs/dirbad" << " is not under CVS control" << endl;
  } else {
    parser->list();
    /* Nodes */
    node = new Node("test1/cvs/dirbad/fileutd", 'f', 0, 0, 0, 0, 0);
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirbad/fileutd" << " is not under CVS control" << endl;
    }
    delete node;
    node = new Node("test1/cvs/dirbad/fileoth", 'f', 0, 0, 0, 0, 0);
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirbad/fileoth" << " is not under CVS control" << endl;
    }
    delete node;
    delete parser;
  }

  /* CVS directory */
  if ((parser = parser_list->isControlled("test1/cvs/CVS")) == NULL) {
    cout << "test1/cvs/CVS" << " is not under CVS control" << endl;
  } else {
    parser->list();
    delete parser;
  }

  return 0;
}
