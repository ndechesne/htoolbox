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

  // Test
  cout << endl << "Only consider controlled files" << endl;

  // Create pseudo parsers list member
  parser_list = new CvsParser(Parser::controlled);

  /* Directory */
  if ((parser = parser_list->isControlled("test1")) == NULL) {
    cout << "test1" << " will be ignored" << endl;
  } else {
    parser->list();
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->isControlled("test1/cvs")) == NULL) {
    cout << "test1/cvs" << " will be ignored" << endl;
  } else {
    parser->list();
    /* Nodes */
    node = new File("test1/cvs/nofile");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/nofile" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/filenew.c");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/filenew.c" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/filemod.o");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/filemod.o" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/fileutd.h");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/fileutd.h" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/fileoth");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/fileoth" << " will be ignored" << endl;
    }
    delete node;
    node = new Directory("test1/cvs/dirutd");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirutd" << " will be ignored" << endl;
    }
    delete node;
    node = new Directory("test1/cvs/diroth");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/diroth" << " will be ignored" << endl;
    }
    delete node;
    node = new Directory("test1/cvs/CVS");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/CVS" << " will be ignored" << endl;
    }
    delete node;
    if ((parser2 = parser->isControlled("test1/cvs/CVS")) == NULL) {
      cout << "test1/cvs/CVS" << " is not controlled" << endl;
    } else {
      node = new File("test1/cvs/CVS/Entries");
      if (parser2->ignore(*node)) {
        cout << node->path() << " will be ignored" << endl;
      }
      delete node;
      node = new File("test1/cvs/CVS/Root");
      if (parser2->ignore(*node)) {
        cout << node->path() << " will be ignored" << endl;
      }
      delete node;
      node = new File("test1/cvs/CVS/Whatever");
      if (parser2->ignore(*node)) {
        cout << node->path() << " will be ignored" << endl;
      }
      delete node;
      delete parser2;
    }
    // Use parser so directory appears as child of controlled directory
    if ((parser2 = parser->isControlled("test1/cvs/diroth")) == NULL) {
      cout << "test1/cvs/diroth" << " is not controlled" << endl;
    } else {
      delete parser2;
    }
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->isControlled("test1/cvs/dirutd")) == NULL) {
    cout << "test1/cvs/dirutd" << " will be ignored" << endl;
  } else {
    parser->list();
    /* Nodes */
    node = new File("test1/cvs/dirutd/fileutd");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirutd/fileutd" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/dirutd/fileoth");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirutd/fileoth" << " will be ignored" << endl;
    }
    delete parser;
    delete node;
  }

  /* Directory */
  if ((parser = parser_list->isControlled("test1/cvs/dirbad")) == NULL) {
    cout << "test1/cvs/dirbad" << " will be ignored" << endl;
  } else {
    parser->list();
    /* Nodes */
    node = new File("test1/cvs/dirbad/fileutd");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirbad/fileutd" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/dirbad/fileoth");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirbad/fileoth" << " will be ignored" << endl;
    }
    delete node;
    delete parser;
  }

  /* CVS directory */
  if ((parser = parser_list->isControlled("test1/cvs/CVS")) == NULL) {
    cout << "test1/cvs/CVS" << " will be ignored" << endl;
  } else {
    parser->list();
    node = new File("test1/cvs/CVS/Entries");
    if (parser->ignore(*node)) {
      cout << node->path() << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/CVS/Root");
    if (parser->ignore(*node)) {
      cout << node->path() << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/CVS/Whatever");
    if (parser->ignore(*node)) {
      cout << node->path() << " will be ignored" << endl;
    }
    delete node;
    delete parser;
  }

  // Test
  cout << endl << "Only consider controlled modified files" << endl;

  // Create pseudo parsers list member
  parser_list = new CvsParser(Parser::modified);

  /* Directory */
  if ((parser = parser_list->isControlled("test1")) == NULL) {
    cout << "test1" << " will be ignored" << endl;
  } else {
    parser->list();
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->isControlled("test1/cvs")) == NULL) {
    cout << "test1/cvs" << " will be ignored" << endl;
  } else {
    parser->list();
    /* Nodes */
    node = new File("test1/cvs/nofile");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/nofile" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/filenew.c");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/filenew.c" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/filemod.o");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/filemod.o" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/fileutd.h");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/fileutd.h" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/fileoth");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/fileoth" << " will be ignored" << endl;
    }
    delete node;
    node = new Directory("test1/cvs/dirutd");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirutd" << " will be ignored" << endl;
    }
    delete node;
    node = new Directory("test1/cvs/diroth");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/diroth" << " will be ignored" << endl;
    }
    delete node;
    node = new Directory("test1/cvs/CVS");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/CVS" << " will be ignored" << endl;
    }
    delete node;
    if ((parser2 = parser->isControlled("test1/cvs/CVS")) == NULL) {
      cout << "test1/cvs/CVS" << " is not controlled" << endl;
    } else {
      node = new File("test1/cvs/CVS/Entries");
      if (parser2->ignore(*node)) {
        cout << node->path() << " will be ignored" << endl;
      }
      delete node;
      node = new File("test1/cvs/CVS/Root");
      if (parser2->ignore(*node)) {
        cout << node->path() << " will be ignored" << endl;
      }
      delete node;
      node = new File("test1/cvs/CVS/Whatever");
      if (parser2->ignore(*node)) {
        cout << node->path() << " will be ignored" << endl;
      }
      delete node;
      delete parser2;
    }
    // Use parser so directory appears as child of controlled directory
    if ((parser2 = parser->isControlled("test1/cvs/diroth")) == NULL) {
      cout << "test1/cvs/diroth" << " is not controlled" << endl;
    } else {
      delete parser2;
    }
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->isControlled("test1/cvs/dirutd")) == NULL) {
    cout << "test1/cvs/dirutd" << " will be ignored" << endl;
  } else {
    parser->list();
    /* Nodes */
    node = new File("test1/cvs/dirutd/fileutd");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirutd/fileutd" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/dirutd/fileoth");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirutd/fileoth" << " will be ignored" << endl;
    }
    delete parser;
    delete node;
  }

  /* Directory */
  if ((parser = parser_list->isControlled("test1/cvs/dirbad")) == NULL) {
    cout << "test1/cvs/dirbad" << " will be ignored" << endl;
  } else {
    parser->list();
    /* Nodes */
    node = new File("test1/cvs/dirbad/fileutd");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirbad/fileutd" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/dirbad/fileoth");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirbad/fileoth" << " will be ignored" << endl;
    }
    delete node;
    delete parser;
  }

  /* CVS directory */
  if ((parser = parser_list->isControlled("test1/cvs/CVS")) == NULL) {
    cout << "test1/cvs/CVS" << " will be ignored" << endl;
  } else {
    parser->list();
    node = new File("test1/cvs/CVS/Entries");
    if (parser->ignore(*node)) {
      cout << node->path() << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/CVS/Root");
    if (parser->ignore(*node)) {
      cout << node->path() << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/CVS/Whatever");
    if (parser->ignore(*node)) {
      cout << node->path() << " will be ignored" << endl;
    }
    delete node;
    delete parser;
  }

  // Test
  cout << endl << "Only consider controlled modified and non-controlled files"
    << endl;

  // Create pseudo parsers list member
  parser_list = new CvsParser(Parser::modifiedandothers);

  /* Directory */
  if ((parser = parser_list->isControlled("test1")) == NULL) {
    cout << "test1" << " will be ignored" << endl;
  } else {
    parser->list();
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->isControlled("test1/cvs")) == NULL) {
    cout << "test1/cvs" << " will be ignored" << endl;
  } else {
    parser->list();
    /* Nodes */
    node = new File("test1/cvs/nofile");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/nofile" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/filenew.c");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/filenew.c" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/filemod.o");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/filemod.o" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/fileutd.h");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/fileutd.h" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/fileoth");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/fileoth" << " will be ignored" << endl;
    }
    delete node;
    node = new Directory("test1/cvs/dirutd");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirutd" << " will be ignored" << endl;
    }
    delete node;
    node = new Directory("test1/cvs/diroth");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/diroth" << " will be ignored" << endl;
    }
    delete node;
    node = new Directory("test1/cvs/CVS");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/CVS" << " will be ignored" << endl;
    }
    delete node;
    if ((parser2 = parser->isControlled("test1/cvs/CVS")) == NULL) {
      cout << "test1/cvs/CVS" << " is not controlled" << endl;
    } else {
      node = new File("test1/cvs/CVS/Entries");
      if (parser2->ignore(*node)) {
        cout << node->path() << " will be ignored" << endl;
      }
      delete node;
      node = new File("test1/cvs/CVS/Root");
      if (parser2->ignore(*node)) {
        cout << node->path() << " will be ignored" << endl;
      }
      delete node;
      node = new File("test1/cvs/CVS/Whatever");
      if (parser2->ignore(*node)) {
        cout << node->path() << " will be ignored" << endl;
      }
      delete node;
      delete parser2;
    }
    // Use parser so directory appears as child of controlled directory
    if ((parser2 = parser->isControlled("test1/cvs/diroth")) == NULL) {
      cout << "test1/cvs/diroth" << " is not controlled" << endl;
    } else {
      delete parser2;
    }
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->isControlled("test1/cvs/dirutd")) == NULL) {
    cout << "test1/cvs/dirutd" << " will be ignored" << endl;
  } else {
    parser->list();
    /* Nodes */
    node = new File("test1/cvs/dirutd/fileutd");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirutd/fileutd" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/dirutd/fileoth");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirutd/fileoth" << " will be ignored" << endl;
    }
    delete parser;
    delete node;
  }

  /* Directory */
  if ((parser = parser_list->isControlled("test1/cvs/dirbad")) == NULL) {
    cout << "test1/cvs/dirbad" << " will be ignored" << endl;
  } else {
    parser->list();
    /* Nodes */
    node = new File("test1/cvs/dirbad/fileutd");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirbad/fileutd" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/dirbad/fileoth");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirbad/fileoth" << " will be ignored" << endl;
    }
    delete node;
    delete parser;
  }

  /* CVS directory */
  if ((parser = parser_list->isControlled("test1/cvs/CVS")) == NULL) {
    cout << "test1/cvs/CVS" << " will be ignored" << endl;
  } else {
    parser->list();
    node = new File("test1/cvs/CVS/Entries");
    if (parser->ignore(*node)) {
      cout << node->path() << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/CVS/Root");
    if (parser->ignore(*node)) {
      cout << node->path() << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/CVS/Whatever");
    if (parser->ignore(*node)) {
      cout << node->path() << " will be ignored" << endl;
    }
    delete node;
    delete parser;
  }


  // Test
  cout << endl << "Only consider non-controlled files" << endl;

  // Create pseudo parsers list member
  parser_list = new CvsParser(Parser::others);

  /* Directory */
  if ((parser = parser_list->isControlled("test1")) == NULL) {
    cout << "test1" << " will be ignored" << endl;
  } else {
    parser->list();
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->isControlled("test1/cvs")) == NULL) {
    cout << "test1/cvs" << " will be ignored" << endl;
  } else {
    parser->list();
    /* Nodes */
    node = new File("test1/cvs/nofile");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/nofile" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/filenew.c");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/filenew.c" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/filemod.o");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/filemod.o" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/fileutd.h");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/fileutd.h" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/fileoth");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/fileoth" << " will be ignored" << endl;
    }
    delete node;
    node = new Directory("test1/cvs/dirutd");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirutd" << " will be ignored" << endl;
    }
    delete node;
    node = new Directory("test1/cvs/diroth");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/diroth" << " will be ignored" << endl;
    }
    delete node;
    node = new Directory("test1/cvs/CVS");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/CVS" << " will be ignored" << endl;
    }
    delete node;
    if ((parser2 = parser->isControlled("test1/cvs/CVS")) == NULL) {
      cout << "test1/cvs/CVS" << " is not controlled" << endl;
    } else {
      node = new File("test1/cvs/CVS/Entries");
      if (parser2->ignore(*node)) {
        cout << node->path() << " will be ignored" << endl;
      }
      delete node;
      node = new File("test1/cvs/CVS/Root");
      if (parser2->ignore(*node)) {
        cout << node->path() << " will be ignored" << endl;
      }
      delete node;
      node = new File("test1/cvs/CVS/Whatever");
      if (parser2->ignore(*node)) {
        cout << node->path() << " will be ignored" << endl;
      }
      delete node;
      delete parser2;
    }
    // Use parser so directory appears as child of controlled directory
    if ((parser2 = parser->isControlled("test1/cvs/diroth")) == NULL) {
      cout << "test1/cvs/diroth" << " is not controlled" << endl;
    } else {
      delete parser2;
    }
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->isControlled("test1/cvs/dirutd")) == NULL) {
    cout << "test1/cvs/dirutd" << " will be ignored" << endl;
  } else {
    parser->list();
    /* Nodes */
    node = new File("test1/cvs/dirutd/fileutd");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirutd/fileutd" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/dirutd/fileoth");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirutd/fileoth" << " will be ignored" << endl;
    }
    delete parser;
    delete node;
  }

  /* Directory */
  if ((parser = parser_list->isControlled("test1/cvs/dirbad")) == NULL) {
    cout << "test1/cvs/dirbad" << " will be ignored" << endl;
  } else {
    parser->list();
    /* Nodes */
    node = new File("test1/cvs/dirbad/fileutd");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirbad/fileutd" << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/dirbad/fileoth");
    if (parser->ignore(*node)) {
      cout << "test1/cvs/dirbad/fileoth" << " will be ignored" << endl;
    }
    delete node;
    delete parser;
  }

  /* CVS directory */
  if ((parser = parser_list->isControlled("test1/cvs/CVS")) == NULL) {
    cout << "test1/cvs/CVS" << " will be ignored" << endl;
  } else {
    parser->list();
    node = new File("test1/cvs/CVS/Entries");
    if (parser->ignore(*node)) {
      cout << node->path() << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/CVS/Root");
    if (parser->ignore(*node)) {
      cout << node->path() << " will be ignored" << endl;
    }
    delete node;
    node = new File("test1/cvs/CVS/Whatever");
    if (parser->ignore(*node)) {
      cout << node->path() << " will be ignored" << endl;
    }
    delete node;
    delete parser;
  }

  return 0;
}
