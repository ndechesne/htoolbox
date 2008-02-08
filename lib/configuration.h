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

// The configuration tree, line per line
class ConfigLine : public vector<string> {
  list<ConfigLine*> _children;
  unsigned int      _line_no;
public:
  ConfigLine() : _line_no(0) {}
  ~ConfigLine();
  // Get line no
  unsigned int lineNo(void) const                 { return _line_no;          }
  // Set line no
  void setLineNo(unsigned int line_no)            { _line_no = line_no;       }
  // Add a child
  void add(ConfigLine* child);
  // Sort children back into config file line order
  void sortChildren();
  // Clear config lines
  void clear();
  // Debug
  void debug(int level = 0) const;
  // Iterator boundaries
  list<ConfigLine*>::const_iterator begin() const { return _children.begin(); }
  list<ConfigLine*>::const_iterator end() const   { return _children.end();   }
};

// To check occurrences
class ConfigCounter {
  const string&     _keyword;
  int               _occurrences;
public:
  ConfigCounter(const string& keyword) : _keyword(keyword), _occurrences(1) {}
  bool operator<(const ConfigCounter& counter) const;
  const string& keyword() const { return _keyword;    }
  int occurrences()        const { return _occurrences; }
  void increment()              { _occurrences++;      }
};

// To re-order the error messages
class ConfigError {
  string            _message;
  int               _line_no;
  int               _type;
public:
  ConfigError(
    const string    message,
    int             line_no = -1,
    unsigned int    type = 0) :
        _message(message),
        _line_no(line_no),
        _type(type) {}
  bool operator<(const ConfigError& error) const;
  void print() const;
};

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
  void add(ConfigItem* child);
  // Find a child
  const ConfigItem* find(string& keyword) const;
  // Check children occurrences
  bool isValid(
    const list<ConfigCounter> counters,
    list<ConfigError>&        errors,
    int                       line_no = -1) const;
  // Debug
  void debug(int level = 0) const;
};

class Config {
  ConfigItem        _items_top;
  ConfigLine        _lines_top;
public:
  Config() : _items_top("") {}
  // Read file, using Stream's flags as given
  int read(
    Stream&         stream,
    char            flags = 0);
  // Add a config item
  void add(ConfigItem* child)    { _items_top.add(child); }
  // Lines tree accessor
  int line(
    ConfigLine**    params,
    bool            reset = false) const;
  // Clear config lines
  void clear();
  // Debug
  void debug() const;
};

}

#endif
