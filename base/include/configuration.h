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

namespace htools {

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

class ConfigObject {
public:
  virtual ConfigObject* configChildFactory(
    const vector<string>& params,
    const char*           file_path = NULL,
    size_t                line_no   = 0) = 0;
};

class ConfigItem;

class Config {
public:
  // Callback for errors
  typedef void (*config_error_cb_f)(
    const char*     message,
    const char*     value,
    size_t          line_no);
private:
  ConfigItem*       _syntax;
  ConfigLine        _lines_top;
  config_error_cb_f _config_error_cb;
public:
  Config(config_error_cb_f config_error_cb = NULL);
  ~Config();
  // Root of config syntax
  ConfigItem* syntaxRoot() { return _syntax; }
  // Add a config item
  ConfigItem* syntaxAdd(
    ConfigItem*     parent,
    const string&   keyword,
    size_t          min_occurrences = 0,
    size_t          max_occurrences = 0,
    size_t          min_params = 0,
    size_t          max_params = 0);
  // Show syntax
  void syntaxShow(int level = 0) const;
  // Read file, using Stream's flags as given
  ssize_t read(
    const char*     path,
    unsigned char   flags,
    ConfigObject*   root = NULL);
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
