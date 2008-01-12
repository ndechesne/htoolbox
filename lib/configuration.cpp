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
#include <string>
#include <list>

using namespace std;

#include <files.h>
#include <configuration.h>

using namespace hbackup;

ConfigItem::~ConfigItem() {
  list<ConfigItem*>::iterator i;
  for (i = _children.begin(); i != _children.end(); i = _children.erase(i)) {
    delete *i;
  }
  free(_keyword);
}

void ConfigItem::debug(int level) {
  list<ConfigItem*>::iterator i;
  for (i = _children.begin(); i != _children.end(); i = _children.erase(i)) {
    for (int j = 0; j < level; j++) {
      cout << " ";
    }
    cout << (*i)->_keyword;
    cout << ", occ.: ";
    if ((*i)->_min_occurrences != 0) {
      cout << "min = " << (*i)->_min_occurrences;
    } else {
      cout << "optional";
    }
    cout << ", ";
    if ((*i)->_max_occurrences != 0) {
      cout << "max = " << (*i)->_max_occurrences;
    } else {
      cout << "no max";
    }
    cout << "; params: ";
    if ((*i)->_min_params != 0) {
      cout << "min params = " << (*i)->_min_params;
    } else {
      cout << "no min";
    }
    cout << ", ";
    if ((*i)->_max_params != 0) {
      cout << "max params = " << (*i)->_max_params;
    } else {
      cout << "no max";
    }
    cout << endl;
    (*i)->debug(level + 2);
  }
}

int Config::read(
    Stream&         stream) {
  list<string> params;
  while (stream.getParams(params) > 0) {
    if (params.size() > 0) {
      cout << "line: " << *params.begin() << ", " << params.size() << endl;
    }
  }
  return 0;
}
