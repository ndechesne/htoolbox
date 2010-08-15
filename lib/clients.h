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

#ifndef _CLIENT_H
#define _CLIENT_H

namespace hbackup {

class Option {
  string _name;
  string _value;
public:
  Option(const string& name, const string& value) :
    _name(name), _value(value) {}
  const string& name() const { return _name;  }
  const string& value() const { return _value; }
  string option() const {
    if (_name.empty())
      return _value;
    else
      return _name + "=" + _value;
  }
};

class Client : public ConfigObject {
  Attributes        _attributes;
  ParsersManager&   _parsers_manager;
  bool              _own_parsers;
  list<ClientPath*> _paths;
  string            _subset_client;
  string            _name;
  string            _subset_server;
  string            _internal_name;
  string            _host_or_ip;
  Path              _list_file;
  string            _protocol;
  list<Option>      _options;
  bool              _timeout_nowarning;
  list<string>      _users;
  string            _home_path;
  Config            _config;
  string            _mounted;
  bool              _classic_mount;
  bool              _fuse_mount;
  int               _expire;
  int mountPath(
    const string&   backup_path,
    string&         path,
    const char*     mount_point);
  int umount(
    const char*     mount_point);
public:
  Client(
    const string&     name,
    const Attributes& attributes,
    ParsersManager&   parsers_manager,
    const string&     sub_name = "");
  ~Client();
  virtual ConfigObject* configChildFactory(
    const vector<string>& params,
    const char*           file_path = NULL,
    size_t                line_no   = 0);
  const char* name() const {
    return _name.c_str();
  }
  const string& internalName() const;
  void addOption(const string& value) {
    _options.push_back(Option("", value));
  }
  void addOption(const string& name, const string& value) {
    _options.push_back(Option(name, value));
  }
  const string getOption(const string& name) const;
  void setHostOrIp(string value) {
    _host_or_ip = value;
  }
  bool setProtocol(string value) {
    _protocol = value;
    return value != "file";
  }
  void setListfile(const char* value);
  const char* listfile() const {
    return _list_file;
  }
  //
  bool initialised() const;
  void setInitialised();
  void setBasePath(const string& home_path) {
    _home_path = home_path;
  }
  ClientPath* addClientPath(const string& path);
  int readConfig(const char* list_path);
  int backup(
    Database&       db,
    const char*     mount_point,
    bool            config_check = false);
  void show(int level = 0) const;
};

}

#endif
