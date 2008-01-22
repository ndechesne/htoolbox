/*
     Copyright (C) 2008  Herve Fache

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

#ifndef _CONFIG_H
#define _CONFIG_H

namespace hbackup {

class ConfigItem {
  char*             _keyword;
  unsigned int      _min_occurrences;
  unsigned int      _max_occurrences;
  unsigned int      _min_params;
  unsigned int      _max_params;
  list<ConfigItem*> _children;
  ConfigItem(const hbackup::ConfigItem&) {}
public:
  ConfigItem(
    const char*     keyword,
    unsigned int    min_occurrences = 0,
    unsigned int    max_occurrences = 0,
    unsigned int    min_params = 0,
    int             max_params = -1) :
      _keyword(strdup(keyword)),
      _min_occurrences(min_occurrences),
      _max_occurrences(max_occurrences),
      _min_params(min_params),
      _max_params(max_params) {
    if (max_params == -1) {
      _max_params = _min_params;
    }
  }
  ~ConfigItem();
  // Parameters accessers
  string keyword() const                { return _keyword;          }
  unsigned int min_occurrences() const  { return _min_occurrences;  }
  unsigned int max_occurrences() const  { return _max_occurrences;  }
  unsigned int min_params() const       { return _min_params;       }
  unsigned int max_params() const       { return _max_params;       }
  // Add a child
  void add(ConfigItem* child) {
    _children.push_back(child);
  }
  const ConfigItem* find(string& keyword) const;
  // Debug
  void debug(int level = 0) const;
};

class ConfigLine : public vector<string> {
  list<ConfigLine*> _children;
public:
  ~ConfigLine();
  // Add a child
  void add(ConfigLine* child) {
    _children.push_back(child);
  }
  // Debug
  void debug(int level = 0) const;
};

class Config {
  ConfigItem        _items_root;
  ConfigLine        _lines_root;
public:
  Config() : _items_root("") {}
  int read(
    Stream&         stream);
  void add(ConfigItem* child)           { _items_root.add(child);   }
  void debug() const;
};

}

#endif
