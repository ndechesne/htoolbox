/*
     Copyright (C) 2010-2011  Herve Fache

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

#ifndef _SHARE_H
#define _SHARE_H

#include <string>
#include <list>

namespace htoolbox {

class Share {
public:
  class Option {
    std::string _name;
    std::string _value;
  public:
    Option(const std::string& name, const std::string& value) :
      _name(name), _value(value) {}
    const std::string& name() const { return _name;  }
    const std::string& value() const { return _value; }
    std::string option() const {
      if (_name.empty())
        return _value;
      else
        return _name + "=" + _value;
    }
  };
private:
  bool        _classic_mount;
  bool        _fuse_mount;
  std::string _mounted;
  std::string _point;
  const std::string getOption(
    const std::list<Option> options,
    const std::string&      name) const;
public:
  Share(const char* point) : _point(point) {}
  ~Share() { umount(); }
  int mount(
    const std::string&      protocol,
    const std::list<Option> options,
    const std::string&      hostname,
    const std::string&      backup_path,
    std::string&            path);
  int umount();
};

}

#endif // _SHARE_H
