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

#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

namespace hbackup {

// The configuration tree, line per line
class ConfigLine {
  vector<string>    _params;
  unsigned int      _line_no;
  list<ConfigLine*> _children;
public:
  ConfigLine(
    unsigned int          line_no = 0) : _line_no(line_no) {}
  ConfigLine(
    const vector<string>& params,
    unsigned int          line_no = 0) : _params(params), _line_no(line_no) {}
  ~ConfigLine();
  // Number of parameters
  size_t size() const { return _params.size(); }
  // Parameter
  const string& operator[](size_t index) const { return _params[index]; }
  // Get line no
  unsigned int lineNo(void) const { return _line_no;          }
  // Add a child
  void add(ConfigLine* child);
  // Sort children back into config file line order
  void sortChildren();
  // Clear config lines
  void clear();
  // Debug
  void show(int level = 0) const;
  // Iterator boundaries
  list<ConfigLine*>::const_iterator begin() const { return _children.begin(); }
  list<ConfigLine*>::const_iterator end() const { return _children.end();   }
};

class ConfigCounter;

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
  void show() const;
};

class ConfigErrors {
  list<ConfigError> _children;
public:
  void push_back(const ConfigError& error) { _children.push_back(error); }
  void sort() { _children.sort(); }
  void clear() { _children.clear(); }
  void show() const {
    for (list<ConfigError>::const_iterator i = _children.begin();
        i != _children.end(); i++) {
      i->show();
    }
  }
};

class ConfigItem {
  string            _keyword;
  unsigned int      _min_occurrences;
  unsigned int      _max_occurrences;
  unsigned int      _min_params;
  unsigned int      _max_params;
  list<ConfigItem*> _children;
  ConfigItem(const hbackup::ConfigItem&) {}
public:
  ConfigItem(
    const string&   keyword,
    unsigned int    min_occurrences = 0,
    unsigned int    max_occurrences = 0,
    unsigned int    min_params = 0,
    int             max_params = 0) :
      _keyword(keyword),
      _min_occurrences(min_occurrences),
      _max_occurrences(max_occurrences),
      _min_params(min_params),
      _max_params(max_params) {
    if (max_params < 0) {
      _max_params = 0;
    } else
    if (max_params == 0) {
      _max_params = _min_params;
    }
  }
  ~ConfigItem();
  // Parameters accessers
  const string& keyword() const { return _keyword; }
  unsigned int min_occurrences() const { return _min_occurrences; }
  unsigned int max_occurrences() const { return _max_occurrences; }
  unsigned int min_params() const { return _min_params; }
  unsigned int max_params() const { return _max_params; }
  // Add a child
  void add(ConfigItem* child);
  // Find a child
  const ConfigItem* find(string& keyword) const;
  // Check children occurrences
  bool isValid(
    const list<ConfigCounter> counters,
    ConfigErrors*             errors  = NULL,
    int                       line_no = -1) const;
  // Debug
  void show(int level = 0) const;
};

class ConfigObject {
public:
  virtual ConfigObject* configChildFactory(
    const vector<string>& params,
    const char*           file_path = NULL,
    size_t                line_no   = 0) = 0;
};

class ConfigSyntax : public ConfigItem {
public:
  ConfigSyntax() : ConfigItem("") {}
};

class Config {
  ConfigLine        _lines_top;
public:
  // Read file, using Stream's flags as given
  ssize_t read(
    const char*     path,
    unsigned char   flags,
    ConfigSyntax&   syntax,
    ConfigObject*   root = NULL,
    ConfigErrors*   errors = NULL);
  // Write configuration to file
  int write(
    const char*     path) const;
  // Lines tree accessor
  int line(
    ConfigLine**    params,
    bool            reset = false) const;
  // Clear config lines
  void clear();
  // Debug
  void show() const;
  // Treat escape characters ('\') as normal characters
  static const unsigned char flags_no_escape    = 0x1;
  // Do not escape last char (and fail to find ending quote) in an end-of-line
  // parameter such as "c:\foo\" (-> c:\foo\ and not c:\foo")
  static const unsigned char flags_dos_catch    = 0x2;
  // Treat spaces as field separators
  static const unsigned char flags_empty_params = 0x4;
  // Extract parameters from given line
  // Returns 1 if missing ending quote, 0 otherwise
  static int extractParams(
    const char*     line,
    vector<string>& params,
    unsigned char   flags      = 0,
    unsigned int    max_params = 0,     // Number of parameters to decode
    const char*     delims     = "\t ", // Default: tabulation and space
    const char*     quotes     = "'\"", // Default: single and double quotes
    const char*     comments   = "#");  // Default: hash
};

}

#endif
