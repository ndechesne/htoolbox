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

#ifndef PARSERS_H
#define PARSERS_H

#include <list>

#include "hbackup.h"
#include "files.h"
#include "report.h"

namespace hbackup {

class Parser {
public:
  enum Mode {
    master            = 0,  //!< master object
    controlled        = 1,  //!< controlled files only
    modified          = 2,  //!< controlled modified files only
    modifiedandothers = 3,  //!< controlled modified files and non-controlled files
    others            = 4   //!< non-controlled files
  };
protected:
  // Declare list stuff here to overcome apparent bug in GCC
  list<Node>            _files;       // Files under control in current dir
  Mode                  _mode;        // What kind of nodes to backup
  bool                  _no_parsing;
public:
  // Constructor
  // Note: all parsers MUST INHERIT this constructor as sole constructor, see
  // IgnoreParser below as an example
  Parser(Mode mode = master, const string& dir_path = "") : _mode(mode) {
    _no_parsing = (dir_path == "");
  }
  // Need a virtual destructor
  virtual ~Parser() {};
  // Tell them who we are
  virtual const char* name() const = 0;
  virtual const char* code() const = 0;
  // Factory
  virtual Parser* createInstance(Mode mode) {
    (void) mode;
    return NULL;
  }
  // This will create an appropriate parser for the directory if relevant
  virtual Parser* createChildIfControlled(const string& dir_path) const = 0;
  // That tells use whether to ignore the file, i.e. not back it up
  virtual bool ignore(const Node& node) = 0;
  // For debug purposes
  virtual void show(int level = 0) = 0;
};

class IgnoreParser : public Parser {
public:
  // Only need default constructor here in fact, but rules are rules
  IgnoreParser(Mode mode = master, const string& dir_path = "") :
    Parser(mode, dir_path) {}
  // Tell them who we are
  const char* name() const { return "ignore"; };
  const char* code() const { return "ign"; };
  // Fail on directory
  Parser* createChildIfControlled(const string& dir_path) const {
    (void) dir_path;
    return NULL;
  };
  // Ignore all files
  bool ignore(const Node& node) {
    (void) node;
    return true;
  };
  // For debug purposes
  void show(int level = 0) {
    (void) level;
  }
};

class Parsers : public list<Parser*> {
public:
  ~Parsers();
  // Create new controlling parser if justified
  Parser* createParserIfControlled(const string& dir_path) const;
  // create new managing parser of given name with given mode
  Parser* createParser(const string& name, const string& mode);
  // For verbosity
  void show(int level = 0) const;
};

/* List of registered parsers */
extern Parsers parsers_registered;

}

#endif
