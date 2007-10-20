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

#ifndef _CLIENT_H
#define _CLIENT_H

namespace hbackup {

class Option {
  string _name;
  string _value;
public:
  Option(const string& name, const string& value) :
    _name(name), _value(value) {}
  string name()   { return _name;  }
  string value()  { return _value; }
  string option() {
    if (_name.empty())
      return _value;
    else
      return _name + "=" + _value;
  }
};

class Client {
  struct        Private;
  Private*      _d;
  string        _name;
  string        _host_or_ip;
  StrPath       _listfile;
  string        _protocol;
  list<Option>  _options;
  //
  bool          _initialised;
  string        _home_path;
  string        _mount_point;
  string        _mounted;
  Filters       _filters;
  int mountPath(string  backup_path, string  *path);
  int umount();
  int readListFile(
    const string&   list_path,
    const Filters&  global_filters);
public:
  Client(string name);
  ~Client();
  string prefix() const { return _protocol + "://" + _name; }
  string name() const   { return _name; }
  void addOption(const string& value) {
    _options.push_back(Option("", value));
  }
  void addOption(const string& name, const string& value) {
    _options.push_back(Option(name, value));
  }
  void setHostOrIp(string value);
  void setProtocol(string value);
  void setListfile(const char* value);
  //
  bool initialised() const { return _initialised; }
  void setInitialised() { _initialised = true; }
  void setBasePath(const string& home_path) {
    _home_path = home_path;
  }
  void setMountPoint(const string& mount_point) {
    _mount_point = mount_point;
  }
  Filter* addFilter(const string& type, const string& name) {
    return _filters.add(type, name);
  }
  Filter* findFilter(const string& name) const {
    return _filters.find(name);
  }
  int  backup(
    Database&       db,
    const Filters&  global_filters,
    bool            config_check = false);
  void show();
};

}

#endif
