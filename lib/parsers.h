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

/*
 * ParsersManager is the entity managing the parsers.  It loads them from the
 * file system, and gets the master object.  It then gives a main instance when
 * required given the name of the parser and the selection mode by calling the
 * createInstance(mode) method on the master object.
 * The instance is created by passing the mode and no path, which forces the
 * _noparsing boolean to true.  It is a good idea to store all instances in a
 * Parsers object.
 * These instances are then used when crawling directories to find out
 * controlled trees, using createParserIfControlled(path) from Parsers.
 * When a parser is in use, we check that sub-dirs are also under control using
 * createChildIfControlled(path).
 * Then to check whether to take into account a file or ignore it is just a call
 * to ignore(node) away.
 */

#ifndef PARSERS_H
#define PARSERS_H

#include <list>
#include <string>

#include "hbackup.h"
#include "files.h"

namespace hbackup {

class IParser {
public:
  enum Mode {
    master            = 0,  //!< master object
    controlled        = 1,  //!< controlled files only
    modified          = 2,  //!< controlled modified files only
    modifiedandothers = 3,  //!< controlled modified files and non-controlled files
    others            = 4   //!< non-controlled files
  };
protected:
  Mode                  _mode;        // What kind of nodes to backup
  bool                  _no_parsing;
public:
  // Constructor
  // Note: all parsers MUST INHERIT this constructor as sole constructor, see
  // IgnoreParser below as an example
  IParser(Mode mode = master, const string& dir_path = "") : _mode(mode) {
    _no_parsing = (dir_path == "");
  }
  // Need a virtual destructor
  virtual ~IParser() {};
  // Tell them who we are
  virtual const char* name() const = 0;
  virtual const char* code() const = 0;
  // Factory
  virtual IParser* createInstance(Mode mode) {
    (void) mode;
    return NULL;
  }
  // This will create an appropriate parser for the directory if relevant
  virtual IParser* createChildIfControlled(const string& dir_path) = 0;
  // That tells use whether to ignore the file, i.e. not back it up
  virtual bool ignore(const Node& node) = 0;
  // For debug purposes
  virtual void show(int level = 0) {
    (void) level;
  }
};

class IgnoreParser : public IParser {
public:
  // Only need default constructor here in fact, but rules are rules
  IgnoreParser(Mode mode = master, const string& dir_path = "") :
    IParser(mode, dir_path) {}
  // Tell them who we are
  const char* name() const { return "ignore"; };
  const char* code() const { return "ign"; };
  // Fail on directory
  IParser* createChildIfControlled(const string& dir_path) {
    (void) dir_path;
    return NULL;
  };
  // Ignore all files
  bool ignore(const Node& node) {
    (void) node;
    return true;
  };
};

class Parsers {
  std::list<IParser*> _children;
public:
  ~Parsers();
  // Create new controlling parser if justified
  IParser* createParserIfControlled(const string& dir_path) const;
  // List management
  bool empty() const { return _children.empty(); }
  size_t size() const { return _children.size(); }
  void push_back(IParser* parser) { _children.push_back(parser); }
  // For verbosity
  void show(int level = 0) const;
};

class ParsersManager {
  std::list<void*> _dls;
  std::list<IParser*> _children;
public:
  ~ParsersManager();
  // Load parser plugins
  int loadPlugins(const char* path);
  // create new managing parser of given name with given mode
  IParser* createParser(const string& name, const string& mode) const;
  // For verbosity
  void show(int level = 0) const;
};

// Plugins need to provide an object of this type called 'manifest'
struct ParserManifest {
  IParser* parser;
};

}

#endif
