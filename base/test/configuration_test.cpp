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

#include <stdio.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <list>

using namespace std;

#include <hreport.h>
#include "configuration.h"

using namespace htools;

class MyObject : public ConfigObject {
  string _name;
  string _path;
  size_t _line_no;
  vector<string> _params;
  list<MyObject*> _children;
  MyObject();
public:
  MyObject(string name) : _name(name), _line_no(0) {}
  MyObject(
    const vector<string>& params,
    const char*         file_path = NULL,
    size_t              line_no   = 0)
    : _name(params[0]), _line_no(line_no), _params(params) {
      _path = file_path;
  }
  virtual ConfigObject* configChildFactory(
    const vector<string>& params,
    const char*           file_path = NULL,
    size_t                line_no   = 0) {
#if 0
    hlog_debug("got line below:");
    params.show();
    hlog_debug("object named %s builds object named %s",
      _name.c_str(), params[0].c_str());
#endif
    MyObject* o = new MyObject(params, file_path, line_no);
    _children.push_back(o);
    return o;
  }
  void show(int level = 0) const {
    if (_name == "root") {
      hlog_verbose(" 0: root");
    } else {
      stringstream s;
      for (size_t j = 0; j < _params.size(); j++) {
        if (j != 0) {
          s << " ";
        }
        s << _params[j];
      }
      char format[16];
      sprintf(format, "%%2d:%%%ds%%s", level + 1);
      hlog_verbose(format, _line_no, " ", s.str().c_str());
    }
#if 0
    hlog_debug("object name: %s has %d child(ren)", _name.c_str(), _children.size());
#endif
    for (list<MyObject*>::const_iterator it = _children.begin();
        it != _children.end(); ++it) {
      (*it)->show(level + 2);
    }
  }
  void clear() {
    for (list<MyObject*>::iterator it = _children.begin();
        it != _children.end(); ++it) {
      (*it)->clear();
      delete *it;
    }
    _children.clear();
  }
};

int main(void) {
  cout << endl << "extractParams" << endl;
  string line;
  vector<string> *params;
  vector<string>::iterator i;

  // Start simple: one argument
  line = "a";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Config::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Two arguments, test blanks
  line = " \ta \tb";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Config::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Not single character argument
  line = "\t ab";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Config::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Two of them
  line = "\t ab cd";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Config::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Three, with comment
  line = "\t ab cd\tef # blah";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Config::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Single quotes
  line = "\t 'ab' 'cd'\t'ef' # blah";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Config::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // And double quotes
  line = "\t \"ab\" 'cd'\t\"ef\" # blah";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Config::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // With blanks in quotes
  line = "\t ab cd\tef 'gh ij\tkl' \"mn op\tqr\" \t# blah";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Config::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // With quotes in quotes
  line = "\t ab cd\tef 'gh \"ij\\\'\tkl' \"mn 'op\\\"\tqr\" \t# blah";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Config::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // With escape characters
  line = "\t a\\b cd\tef 'g\\h \"ij\\\'\tkl' \"m\\n 'op\\\"\tqr\" \t# blah";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Config::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Missing ending single quote
  line = "\t a\\b cd\tef 'g\\h \"ij\\\'\tkl";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Config::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Missing ending double quote
  line = "\t a\\b cd\tef 'g\\h \"ij\\\'\tkl' \"m\\n 'op\\\"\tqr";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Config::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // The DOS catch: undealt with
  line = "path \"C:\\Backup\\\"";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Config::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // The DOS catch: ignoring escape characters altogether
  line = "path \"C:\\Backup\\\"";
  params = new vector<string>;
  cout << "readline(" << line << "): "
    << Config::extractParams(line.c_str(), *params, Config::flags_no_escape)
    << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // The DOS catch: dealing with the particular case
  line = "path \"C:\\Backup\\\"";
  params = new vector<string>;
  cout << "readline(" << line << "): "
    << Config::extractParams(line.c_str(), *params, Config::flags_dos_catch)
    << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Extract a line where spaces matter
  line = " this is a path \"C:\\Backup\"   and  spaces matter ";
  params = new vector<string>;
  cout << "readline(" << line << "): "
    << Config::extractParams(line.c_str(), *params, Config::flags_dos_catch
      | Config::flags_empty_params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Extract part of a line where spaces matter
  line = " this is a path \"C:\\Backup\"   and  spaces matter ";
  params = new vector<string>;
  cout << "readline(" << line << "): "
    << Config::extractParams(line.c_str(), *params, Config::flags_dos_catch
      | Config::flags_empty_params, 5) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // And another
  line = "\t\tf\tblah\t''";
  params = new vector<string>;
  cout << "readline(" << line << "): "
    << Config::extractParams(line.c_str(), *params, Config::flags_dos_catch
      | Config::flags_empty_params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // And one missing the ending quote
  line = "\t\tf\tblah\t'";
  params = new vector<string>;
  cout << "readline(" << line << "): "
    << Config::extractParams(line.c_str(), *params, Config::flags_dos_catch
      | Config::flags_empty_params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // And another one missing with empty last argument
  line = "\t\tf\tblah\t\t";
  params = new vector<string>;
  cout << "readline(" << line << "): "
    << Config::extractParams(line.c_str(), *params, Config::flags_dos_catch
      | Config::flags_empty_params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;


  Config*           config;
  ConfigErrors      errors;

  report.setLevel(debug);

  cout << endl << "Test: client configuration" << endl;
  ConfigSyntax client_syntax;

  // expire
  client_syntax.add(new ConfigItem("expire", 0, 1, 1));

  // users
  client_syntax.add(new ConfigItem("users", 0, 1, 1, -1));

  // ignore
  client_syntax.add(new ConfigItem("ignore", 0, 1, 1));

  // filter
  {
    ConfigItem* item = new ConfigItem("filter", 0, 0, 2);
    client_syntax.add(item);

    // condition
    item->add(new ConfigItem("condition", 1, 0, 2));
  }

  // path
  {
    ConfigItem* item = new ConfigItem("path", 1, 0, 1);
    client_syntax.add(item);

    // parser
    item->add(new ConfigItem("parser", 0, 0, 1, 2));

    // filter
    {
      ConfigItem* item2 = new ConfigItem("filter", 0, 0, 2);
      item->add(item2);

      // condition
      item2->add(new ConfigItem("condition", 1, 0, 2));
    }

    // ignore
    item->add(new ConfigItem("ignore", 0, 1, 1));

    // compress
    item->add(new ConfigItem("compress", 0, 1, 1));

    // no_compress
    item->add(new ConfigItem("no_compress", 0, 1, 1));
  }

  // show debug
  cout << "Syntax:" << endl;
  client_syntax.show(2);

  config = new Config;
  if (config->read("etc/localhost.list", 0, client_syntax, NULL, &errors) < 0) {
    cout << "failed to read config!" << endl;
  }
  errors.show();
  errors.clear();
  config->clear();

  cout << "Test: with object" << endl;

  MyObject object("root");

  if (config->read("etc/localhost.list", 0, client_syntax, &object, &errors) < 0) {
    cout << "failed to read config!" << endl;
  }
  errors.show();
  errors.clear();
  object.show();
  object.clear();

  // show debug
  config->show();

  delete config;

  cout << "Test: server configuration" << endl;
  ConfigSyntax syntax;

  // log
  {
    ConfigItem* log = new ConfigItem("log", 0, 1, 1);
    syntax.add(log);
    // max lines per file
    log->add(new ConfigItem("max_lines", 0, 1, 1, 1));
    // max backups to keep
    log->add(new ConfigItem("backups", 0, 1, 1, 1));
    // log level
    log->add(new ConfigItem("level", 0, 1, 1, 1));
  }

  // db
  {
    ConfigItem* item = new ConfigItem("db", 0, 1, 1);
    syntax.add(item);

    // compress
    item->add(new ConfigItem("compress", 0, 1, 1, 1));

    // compress_auto: to be ignored
    item->add(new ConfigItem("compress_auto", 0, 1));
  }

  // tree
  {
    ConfigItem* item = new ConfigItem("tree", 0, 1, 1);
    syntax.add(item);

    // links
    item->add(new ConfigItem("links", 0, 1, 1, 1));

    // compressed
    item->add(new ConfigItem("compressed", 0, 1, 1, 1));

    // hourly
    item->add(new ConfigItem("hourly", 0, 1, 1, 1));

    // daily
    item->add(new ConfigItem("daily", 0, 1, 1, 1));

    // weekly
    item->add(new ConfigItem("weekly", 0, 1, 1, 1));
  }

  // parser plugins
  syntax.add(new ConfigItem("parsers_dir", 0, 0, 1));

  // filter
  {
    ConfigItem* item = new ConfigItem("filter", 0, 0, 2);
    syntax.add(item);

    // condition
    item->add(new ConfigItem("condition", 1, 0, 2));
  }

  // ignore
  syntax.add(new ConfigItem("ignore", 0, 1, 1));

  // timeout_nowarning
  syntax.add(new ConfigItem("timeout_nowarning", 0, 1));

  // report_copy_error_once
  syntax.add(new ConfigItem("report_copy_error_once", 0, 1));

  // client
  {
    ConfigItem* item = new ConfigItem("client", 1, 0, 1, 2);
    syntax.add(item);

    // hostname
    item->add(new ConfigItem("hostname", 0, 1, 1));

    // protocol
    item->add(new ConfigItem("protocol", 1, 1, 1));

    // option
    item->add(new ConfigItem("option", 0, 0, 1, 2));

    // timeout_nowarning
    item->add(new ConfigItem("timeout_nowarning", 0, 1));

    // report_copy_error_once
    item->add(new ConfigItem("report_copy_error_once", 0, 1));

    // config
    item->add(new ConfigItem("config", 0, 1, 1));

    // expire
    item->add(new ConfigItem("expire", 0, 1, 1));

    // users
    item->add(new ConfigItem("users", 0, 1, 1, -1));

    // ignore
    item->add(new ConfigItem("ignore", 0, 1, 1));

    // filter
    {
      ConfigItem* sub_item = new ConfigItem("filter", 0, 0, 2);
      item->add(sub_item);

      // condition
      sub_item->add(new ConfigItem("condition", 1, 0, 2));
    }

    // path
    {
      ConfigItem* sub_item = new ConfigItem("path", 0, 0, 1);
      item->add(sub_item);

      // parser
      sub_item->add(new ConfigItem("parser", 0, 0, 1, 2));

      // filter
      {
        ConfigItem* item2 = new ConfigItem("filter", 0, 0, 2);
        sub_item->add(item2);

        // condition
        item2->add(new ConfigItem("condition", 1, 0, 2));
      }

      // ignore
      sub_item->add(new ConfigItem("ignore", 0, 1, 1));

      // compress
      sub_item->add(new ConfigItem("compress", 0, 1, 1));

      // no_compress
      sub_item->add(new ConfigItem("no_compress", 0, 1, 1));
    }
  }

  // show debug
  cout << "Syntax:" << endl;
  syntax.show(2);

  config = new Config;
  if (config->read("etc/hbackup.conf", 0, syntax, &object, &errors) < 0) {
    cout << "failed to read config!" << endl;
  }
  errors.show();
  errors.clear();
  object.show();
  object.clear();

  // show debug
  config->show();

  cout << "Test: configuration save" << endl;

  // save configuration
  if (config->write("etc/config.saved") < 0) {
    cout << "failed to write config!" << endl;
  }

  // clear list
  config->clear();

  if (config->read("etc/config.saved", 0, syntax, &object, &errors) < 0) {
    cout << "failed to read config!" << endl;
  }
  errors.show();
  errors.clear();
  object.show();
  object.clear();

  // show debug
  config->show();

  // clear list
  config->clear();

  cout << "Test: broken configuration" << endl;
  if (config->read("etc/broken.conf", 0, syntax, &object, &errors) < 0) {
    cout << "failed to read config!" << endl;
  }
  errors.show();
  errors.clear();
  object.show();
  object.clear();

  // show debug
  config->show();

  delete config;

  return 0;
}
