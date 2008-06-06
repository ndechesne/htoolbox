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
  string option() const {
    if (_name.empty())
      return _value;
    else
      return _name + "=" + _value;
  }
};

class Client {
  struct            Private;
  Private* const    _d;
  int mountPath(
    const string&   backup_path,
    string&         path,
    const char*     mount_point);
  int umount(
    const char*     mount_point);
public:
  Client(const string& name, const string& sub_name = "");
  ~Client();
  const char* name() const;
  const string& subset() const;
  string internalName() const;
  void addOption(const string& value);
  void addOption(const string& name, const string& value);
  void setHostOrIp(string value);
  bool setProtocol(string value);     // return true if remote protocol
  void setTimeOutNoWarning();
  void setReportCopyErrorOnce();
  void setListfile(const char* value);
  const char* listfile() const;
  void setExpire(int expire);
  //
  bool initialised() const;
  void setInitialised();
  void setBasePath(const string& home_path);
  ClientPath* addClientPath(const string& path);
  Filter* addFilter(const string& type, const string& name);
  Filter* findFilter(const string& name) const;
  int  readConfig(
    const char*     list_path,
    const Filters&  global_filters);
  int  backup(
    Database&       db,
    const Filters&  global_filters,
    const char*     mount_point,
    bool            config_check = false);
  void show(int level = 0) const;
};

}

#endif
