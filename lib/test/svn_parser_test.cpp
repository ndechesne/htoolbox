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
#include <list>

#include <sys/stat.h>
#include <dlfcn.h>

using namespace std;

#include "test.h"

#include "files.h"
#include "hreport.h"
#include "parsers.h"

int main(void) {
  IParser* parser_list;
  IParser* parser;
  IParser* parser2;
  Node* node;

  report.setLevel(debug);

  // Load plugin
  void* file_handle = dlopen("../../../parsers/.libs/svn_parser.so", RTLD_NOW);
  if (file_handle == NULL) {
    hlog_alert("plugin not found");
    exit(0);
  }
  void* manifest_handle = dlsym(file_handle, "manifest");
  if (manifest_handle == NULL) {
    hlog_alert("symbol manifest not found in plugin");
    exit(0);
  }
  ParserManifest* manifest = static_cast<ParserManifest*>(manifest_handle);
  IParser& ParserManager = *manifest->parser;

  // Test
  cout << endl << "Only consider controlled files" << endl;

  // Create pseudo parsers list member
  parser_list = ParserManager.createInstance(IParser::controlled);

  /* Directory */
  if ((parser = parser_list->createChildIfControlled("test1")) == NULL) {
    cout << "test1" << " is not controlled" << endl;
  } else {
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->createChildIfControlled("test1/svn")) == NULL) {
    cout << "test1/svn" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new File("test1/svn/nofile");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/filenew.c");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/filemod.o");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/fileutd.h");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Directory("test1/svn/dirutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Directory("test1/svn/dirbad");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Directory("test1/svn/.svn");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    if ((parser2 = parser->createChildIfControlled("test1/svn/.svn")) == NULL) {
      cout << "test1/svn/.svn" << " is not controlled" << endl;
    } else {
      node = new File("test1/svn/.svn/entries");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      node = new File("test1/svn/.svn/format");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      node = new File("test1/svn/.svn/whatever");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      delete parser2;
    }
    // Use parser so directory appears as child of controlled directory
    if ((parser2 = parser->createChildIfControlled("test1/svn/dirbad")) == NULL) {
      cout << "test1/svn/dirbad" << " is not controlled" << endl;
    } else {
      delete parser2;
    }
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->createChildIfControlled("test1/svn/dirutd")) == NULL) {
    cout << "test1/svn/dirutd" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new File("test1/svn/dirutd/fileutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/dirutd/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete parser;
    delete node;
  }

  /* Directory */
  if ((parser = parser_list->createChildIfControlled("test1/svn/diroth")) == NULL) {
    cout << "test1/svn/diroth" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new File("test1/svn/diroth/fileutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/diroth/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    delete parser;
  }

  /* Subversion directory */
  if ((parser = parser_list->createChildIfControlled("test1/svn/.svn")) == NULL) {
    cout << "test1/svn/.svn" << " is not controlled" << endl;
  } else {
    node = new File("test1/svn/.svn/entries");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/.svn/format");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/.svn/whatever");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    delete parser;
  }

  // Test
  cout << endl << "Only consider controlled modified files" << endl;

  // Create pseudo parsers list member
  parser_list = ParserManager.createInstance(IParser::modified);

  /* Directory */
  if ((parser = parser_list->createChildIfControlled("test1")) == NULL) {
    cout << "test1" << " is not controlled" << endl;
  } else {
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->createChildIfControlled("test1/svn")) == NULL) {
    cout << "test1/svn" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new File("test1/svn/nofile");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/filenew.c");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/filemod.o");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/fileutd.h");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Directory("test1/svn/dirutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Directory("test1/svn/dirbad");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Directory("test1/svn/.svn");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    if ((parser2 = parser->createChildIfControlled("test1/svn/.svn")) == NULL) {
      cout << "test1/svn/.svn" << " is not controlled" << endl;
    } else {
      node = new File("test1/svn/.svn/entries");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      node = new File("test1/svn/.svn/format");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      node = new File("test1/svn/.svn/whatever");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      delete parser2;
    }
    // Use parser so directory appears as child of controlled directory
    if ((parser2 = parser->createChildIfControlled("test1/svn/dirbad")) == NULL) {
      cout << "test1/svn/dirbad" << " is not controlled" << endl;
    } else {
      delete parser2;
    }
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->createChildIfControlled("test1/svn/dirutd")) == NULL) {
    cout << "test1/svn/dirutd" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new File("test1/svn/dirutd/fileutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/dirutd/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete parser;
    delete node;
  }

  /* Directory */
  if ((parser = parser_list->createChildIfControlled("test1/svn/diroth")) == NULL) {
    cout << "test1/svn/diroth" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new File("test1/svn/diroth/fileutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/diroth/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    delete parser;
  }

  /* Subversion directory */
  if ((parser = parser_list->createChildIfControlled("test1/svn/.svn")) == NULL) {
    cout << "test1/svn/.svn" << " is not controlled" << endl;
  } else {
    node = new File("test1/svn/.svn/entries");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/.svn/format");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/.svn/whatever");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    delete parser;
  }

  // Test
  cout << endl << "Only consider controlled modified and non-controlled files"
    << endl;

  // Create pseudo parsers list member
  parser_list = ParserManager.createInstance(IParser::modifiedandothers);

  /* Directory */
  if ((parser = parser_list->createChildIfControlled("test1")) == NULL) {
    cout << "test1" << " is not controlled" << endl;
  } else {
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->createChildIfControlled("test1/svn")) == NULL) {
    cout << "test1/svn" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new File("test1/svn/nofile");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/filenew.c");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/filemod.o");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/fileutd.h");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Directory("test1/svn/dirutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Directory("test1/svn/dirbad");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Directory("test1/svn/.svn");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    if ((parser2 = parser->createChildIfControlled("test1/svn/.svn")) == NULL) {
      cout << "test1/svn/.svn" << " is not controlled" << endl;
    } else {
      node = new File("test1/svn/.svn/entries");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      node = new File("test1/svn/.svn/format");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      node = new File("test1/svn/.svn/whatever");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      delete parser2;
    }
    // Use parser so directory appears as child of controlled directory
    if ((parser2 = parser->createChildIfControlled("test1/svn/dirbad")) == NULL) {
      cout << "test1/svn/dirbad" << " is not controlled" << endl;
    } else {
      delete parser2;
    }
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->createChildIfControlled("test1/svn/dirutd")) == NULL) {
    cout << "test1/svn/dirutd" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new File("test1/svn/dirutd/fileutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/dirutd/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete parser;
    delete node;
  }

  /* Directory */
  if ((parser = parser_list->createChildIfControlled("test1/svn/diroth")) == NULL) {
    cout << "test1/svn/diroth" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new File("test1/svn/diroth/fileutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/diroth/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    delete parser;
  }

  /* Subversion directory */
  if ((parser = parser_list->createChildIfControlled("test1/svn/.svn")) == NULL) {
    cout << "test1/svn/.svn" << " is not controlled" << endl;
  } else {
    node = new File("test1/svn/.svn/entries");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/.svn/format");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/.svn/whatever");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    delete parser;
  }


  // Test
  cout << endl << "Only consider non-controlled files" << endl;

  // Create pseudo parsers list member
  parser_list = ParserManager.createInstance(IParser::others);

  /* Directory */
  if ((parser = parser_list->createChildIfControlled("test1")) == NULL) {
    cout << "test1" << " is not controlled" << endl;
  } else {
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->createChildIfControlled("test1/svn")) == NULL) {
    cout << "test1/svn" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new File("test1/svn/nofile");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/filenew.c");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/filemod.o");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/fileutd.h");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Directory("test1/svn/dirutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Directory("test1/svn/dirbad");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Directory("test1/svn/.svn");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    if ((parser2 = parser->createChildIfControlled("test1/svn/.svn")) == NULL) {
      cout << "test1/svn/.svn" << " is not controlled" << endl;
    } else {
      node = new File("test1/svn/.svn/entries");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      node = new File("test1/svn/.svn/format");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      node = new File("test1/svn/.svn/whatever");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      delete parser2;
    }
    // Use parser so directory appears as child of controlled directory
    if ((parser2 = parser->createChildIfControlled("test1/svn/dirbad")) == NULL) {
      cout << "test1/svn/dirbad" << " is not controlled" << endl;
    } else {
      delete parser2;
    }
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->createChildIfControlled("test1/svn/dirutd")) == NULL) {
    cout << "test1/svn/dirutd" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new File("test1/svn/dirutd/fileutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/dirutd/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete parser;
    delete node;
  }

  /* Directory */
  if ((parser = parser_list->createChildIfControlled("test1/svn/diroth")) == NULL) {
    cout << "test1/svn/diroth" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new File("test1/svn/diroth/fileutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/diroth/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    delete parser;
  }

  /* Subversion directory */
  if ((parser = parser_list->createChildIfControlled("test1/svn/.svn")) == NULL) {
    cout << "test1/svn/.svn" << " is not controlled" << endl;
  } else {
    node = new File("test1/svn/.svn/entries");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/.svn/format");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new File("test1/svn/.svn/whatever");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    delete parser;
  }

  return 0;
}
