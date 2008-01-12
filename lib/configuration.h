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

#ifndef _CONFIG_H
#define _CONFIG_H

namespace hbackup {

class ConfigItem {
  char*             _keyword;
  int               _min_occurrences;
  int               _max_occurrences;
  int               _min_params;
  int               _max_params;
  list<ConfigItem*> _children;
public:
  ConfigItem(
    const char*     keyword,
    int             min_occurrences = 0,
    int             max_occurrences = 0,
    int             min_params = 0,
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
  void add(ConfigItem* child) {
    _children.push_back(child);
  }
  // Debug
  void debug(int level = 0);
};

class Config : public ConfigItem {
public:
  Config() :
    ConfigItem("HEAD") {}
  int read(
    Stream&         stream);
};

}

#endif
