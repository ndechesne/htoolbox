/*
     Copyright (C) 2008-2010  Herve Fache

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

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "hreport.h"
#include "parsers.h"
// Make it dynamic!
#include "cvs_parser.h"
#include "svn_parser.h"

using namespace hbackup;
using namespace hreport;

Parsers hbackup::parsers_registered;

Parsers::~Parsers() {
  list<Parser*>::iterator i;
  for (i = _children.begin(); i != _children.end(); i++) {
    delete *i;
  }
  _children.clear();
}

Parser* Parsers::createParserIfControlled(const string& dir_path) const {
  Parser *parser;
  list<Parser*>::const_iterator i;
  for (i = _children.begin(); i != _children.end(); i++) {
    parser = (*i)->createChildIfControlled(dir_path);
    if (parser != NULL) {
      return parser;
    }
  }
  return NULL;
}

Parser* Parsers::createParser(const string& name, const string& mode_str) {
  Parser::Mode mode;

  /* Determine mode */
  switch (mode_str[0]) {
    case 'c':
      // All controlled files
      mode = Parser::controlled;
      break;
    case 'l':
      // Local files
      mode = Parser::modifiedandothers;
      break;
    case 'm':
      // Modified controlled files
      mode = Parser::modified;
      break;
    case 'o':
      // Non controlled files
      mode = Parser::others;
      break;
    default:
      out(error, msg_standard, "Undefined parser mode", -1, mode_str.c_str());
      return NULL;
  }

  /* FIXME Create list of parsers */
  if (empty()) {
    push_back(new CvsParser);
    push_back(new SvnParser);
  }

  /* Create instance of specified parser */
  list<Parser*>::iterator i;
  for (i = _children.begin(); i != _children.end(); i++) {
    if ((*i)->code() == name) {
      return (*i)->createInstance(mode);
    }
  }
  out(error, msg_standard, "Unsupported parser", -1, name.c_str());
  return NULL;
}

void Parsers::show(int level) const {
  list<Parser*>::const_iterator i;
  for (i = _children.begin(); i != _children.end(); i++) {
    out(debug, msg_standard, (*i)->name(), level, "Parser");
  }
}
