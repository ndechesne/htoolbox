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
#include "parsers.h"

int main(void) {
  IParser* parser_list;
  IParser* parser;
  IParser* parser2;
  Node* node;

  report.setLevel(debug);

  // Load plugin
  void* file_handle = dlopen("../../../parsers/.libs/cvs_parser.so", RTLD_NOW);
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

  if ((parser = parser_list->createChildIfControlled("test1")) == NULL) {
    cout << "test1" << " is not controlled" << endl;
  } else {
    delete parser;
  }

  if ((parser = parser_list->createChildIfControlled("test1/cvs")) == NULL) {
    cout << "test1/cvs" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new Node("test1/cvs/nofile");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/filenew.c");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/filemod.o");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/fileutd.h");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/dirutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/dirbad");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/CVS");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    if ((parser2 = parser->createChildIfControlled("test1/cvs/CVS")) == NULL) {
      cout << "test1/cvs/CVS" << " is not controlled" << endl;
    } else {
      node = new Node("test1/cvs/CVS/Entries");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      node = new Node("test1/cvs/CVS/Root");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      node = new Node("test1/cvs/CVS/Whatever");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      delete parser2;
    }
    // Use parser so directory appears as child of controlled directory
    if ((parser2 = parser->createChildIfControlled("test1/cvs/dirbad")) == NULL) {
      cout << "test1/cvs/dirbad" << " is not controlled" << endl;
    } else {
      delete parser2;
    }
    delete parser;
  }

  if ((parser = parser_list->createChildIfControlled("test1/cvs/dirutd")) == NULL) {
    cout << "test1/cvs/dirutd" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new Node("test1/cvs/dirutd/fileutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/dirutd/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete parser;
    delete node;
  }

  if ((parser = parser_list->createChildIfControlled("test1/cvs/diroth")) == NULL) {
    cout << "test1/cvs/diroth" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new Node("test1/cvs/diroth/fileutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/diroth/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    delete parser;
  }

  /* CVS directory */
  if ((parser = parser_list->createChildIfControlled("test1/cvs/CVS")) == NULL) {
    cout << "test1/cvs/CVS" << " is not controlled" << endl;
  } else {
    node = new Node("test1/cvs/CVS/Entries");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/CVS/Root");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/CVS/Whatever");
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

  if ((parser = parser_list->createChildIfControlled("test1")) == NULL) {
    cout << "test1" << " is not controlled" << endl;
  } else {
    delete parser;
  }

  if ((parser = parser_list->createChildIfControlled("test1/cvs")) == NULL) {
    cout << "test1/cvs" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new Node("test1/cvs/nofile");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/filenew.c");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/filemod.o");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/fileutd.h");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/dirutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/dirbad");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/CVS");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    if ((parser2 = parser->createChildIfControlled("test1/cvs/CVS")) == NULL) {
      cout << "test1/cvs/CVS" << " is not controlled" << endl;
    } else {
      node = new Node("test1/cvs/CVS/Entries");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      node = new Node("test1/cvs/CVS/Root");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      node = new Node("test1/cvs/CVS/Whatever");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      delete parser2;
    }
    // Use parser so directory appears as child of controlled directory
    if ((parser2 = parser->createChildIfControlled("test1/cvs/dirbad")) == NULL) {
      cout << "test1/cvs/dirbad" << " is not controlled" << endl;
    } else {
      delete parser2;
    }
    delete parser;
  }

  if ((parser = parser_list->createChildIfControlled("test1/cvs/dirutd")) == NULL) {
    cout << "test1/cvs/dirutd" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new Node("test1/cvs/dirutd/fileutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/dirutd/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete parser;
    delete node;
  }

  if ((parser = parser_list->createChildIfControlled("test1/cvs/diroth")) == NULL) {
    cout << "test1/cvs/diroth" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new Node("test1/cvs/diroth/fileutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/diroth/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    delete parser;
  }

  /* CVS directory */
  if ((parser = parser_list->createChildIfControlled("test1/cvs/CVS")) == NULL) {
    cout << "test1/cvs/CVS" << " is not controlled" << endl;
  } else {
    node = new Node("test1/cvs/CVS/Entries");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/CVS/Root");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/CVS/Whatever");
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

  if ((parser = parser_list->createChildIfControlled("test1")) == NULL) {
    cout << "test1" << " is not controlled" << endl;
  } else {
    delete parser;
  }

  if ((parser = parser_list->createChildIfControlled("test1/cvs")) == NULL) {
    cout << "test1/cvs" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new Node("test1/cvs/nofile");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/filenew.c");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/filemod.o");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/fileutd.h");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/dirutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/dirbad");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/CVS");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    if ((parser2 = parser->createChildIfControlled("test1/cvs/CVS")) == NULL) {
      cout << "test1/cvs/CVS" << " is not controlled" << endl;
    } else {
      node = new Node("test1/cvs/CVS/Entries");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      node = new Node("test1/cvs/CVS/Root");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      node = new Node("test1/cvs/CVS/Whatever");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      delete parser2;
    }
    // Use parser so directory appears as child of controlled directory
    if ((parser2 = parser->createChildIfControlled("test1/cvs/dirbad")) == NULL) {
      cout << "test1/cvs/dirbad" << " is not controlled" << endl;
    } else {
      delete parser2;
    }
    delete parser;
  }

  if ((parser = parser_list->createChildIfControlled("test1/cvs/dirutd")) == NULL) {
    cout << "test1/cvs/dirutd" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new Node("test1/cvs/dirutd/fileutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/dirutd/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete parser;
    delete node;
  }

  if ((parser = parser_list->createChildIfControlled("test1/cvs/diroth")) == NULL) {
    cout << "test1/cvs/diroth" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new Node("test1/cvs/diroth/fileutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/diroth/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    delete parser;
  }

  /* CVS directory */
  if ((parser = parser_list->createChildIfControlled("test1/cvs/CVS")) == NULL) {
    cout << "test1/cvs/CVS" << " is not controlled" << endl;
  } else {
    node = new Node("test1/cvs/CVS/Entries");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/CVS/Root");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/CVS/Whatever");
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

  if ((parser = parser_list->createChildIfControlled("test1")) == NULL) {
    cout << "test1" << " is not controlled" << endl;
  } else {
    delete parser;
  }

  if ((parser = parser_list->createChildIfControlled("test1/cvs")) == NULL) {
    cout << "test1/cvs" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new Node("test1/cvs/nofile");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/filenew.c");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/filemod.o");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/fileutd.h");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/dirutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/dirbad");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/CVS");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    if ((parser2 = parser->createChildIfControlled("test1/cvs/CVS")) == NULL) {
      cout << "test1/cvs/CVS" << " is not controlled" << endl;
    } else {
      node = new Node("test1/cvs/CVS/Entries");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      node = new Node("test1/cvs/CVS/Root");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      node = new Node("test1/cvs/CVS/Whatever");
      cout << node->path() << " will";
      if (! parser2->ignore(*node)) {
        cout << " not";
      }
      cout << " be ignored" << endl;
      delete node;
      delete parser2;
    }
    // Use parser so directory appears as child of controlled directory
    if ((parser2 = parser->createChildIfControlled("test1/cvs/dirbad")) == NULL) {
      cout << "test1/cvs/dirbad" << " is not controlled" << endl;
    } else {
      delete parser2;
    }
    delete parser;
  }

  if ((parser = parser_list->createChildIfControlled("test1/cvs/dirutd")) == NULL) {
    cout << "test1/cvs/dirutd" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new Node("test1/cvs/dirutd/fileutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/dirutd/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete parser;
    delete node;
  }

  if ((parser = parser_list->createChildIfControlled("test1/cvs/diroth")) == NULL) {
    cout << "test1/cvs/diroth" << " is not controlled" << endl;
  } else {
    /* Nodes */
    node = new Node("test1/cvs/diroth/fileutd");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/diroth/fileoth");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    delete parser;
  }

  /* CVS directory */
  if ((parser = parser_list->createChildIfControlled("test1/cvs/CVS")) == NULL) {
    cout << "test1/cvs/CVS" << " is not controlled" << endl;
  } else {
    node = new Node("test1/cvs/CVS/Entries");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/CVS/Root");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    node = new Node("test1/cvs/CVS/Whatever");
    cout << node->path() << " will";
    if (! parser->ignore(*node)) {
      cout << " not";
    }
    cout << " be ignored" << endl;
    delete node;
    delete parser;
  }

  dlclose(file_handle);
  return 0;
}
