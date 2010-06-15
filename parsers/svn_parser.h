/*
     Copyright (C) 2006-2008  Herve Fache

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

#ifndef SVN_PARSER_H
#define SVN_PARSER_H

#include "parsers.h"

namespace hbackup {

class SvnParser : public Parser {
public:
  // Constructor
  SvnParser(Mode mode = master, const string& dir_path = "");
  // Tell them who we are
  const char* name() const { return "Subversion"; };
  const char* code() const { return "svn"; };
  // Factory
  Parser* createInstance(Mode mode) { return new SvnParser(mode); }
  // This will create an appropriate parser for the directory if relevant
  Parser* isControlled(const string& dir_path) const;
  // That tells us whether to ignore the file, i.e. not back it up
  bool ignore(const Node& node);
  // For debug purposes
  void show(int level = 0);
};

}

#endif
