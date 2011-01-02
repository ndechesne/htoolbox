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

using namespace std;

#include <stdlib.h>
#include <errno.h>

#include "report.h"
#include "share.h"

using namespace htoolbox;

const string Share::getOption(
    const list<Option>  options,
    const string&       name) const {
  for (list<Option>::const_iterator i = options.begin(); i != options.end();
       i++) {
    if (i->name() == name) {
      return i->value();
    }
  }
  return "";
}

int Share::mount(
    const string&       protocol,
    const list<Option>  options,
    const string&       hostname,
    const string&       backup_path,
    string&             path) {
  string command;
  string share;
  errno = EINVAL;
  _classic_mount = false;
  _fuse_mount = false;

  // Determine share and path
  if (protocol == "file") {
    command = "";
    share = "";
    path  = backup_path;
    return 0;
  } else
  if (protocol == "nfs") {
    _classic_mount = true;
    command = "mount -t nfs -o ro,noatime,nolock,soft,timeo=30,intr";
    share = hostname + ":" + backup_path;
    path  = _point;
  } else
  if (protocol == "smb") {
    _classic_mount = true;
    command = "mount -t cifs -o ro,nocase";
    if (backup_path.size() < 3) {
      return -1;
    }
    char drive_letter = backup_path[0];
    if (! isupper(drive_letter)) {
      return -1;
    }
    share = "//" + hostname + "/" + drive_letter + "$";
    path  = _point;
    path += "/" +  backup_path.substr(3);
  } else
  if (protocol == "ssh") {
    _fuse_mount = true;
    const string username = getOption(options, "username");
    if (username != "") {
      share = username + "@";
    }
    const string password = getOption(options, "password");
    if (password != "") {
      command = "echo " + password + " | sshfs -o password_stdin";
    } else {
      command = "sshfs";
    }
    // echo <password> | sshfs -o password_stdin <user>@<server>:<path> <mount path>
    share   += hostname + ":" + backup_path;
    path    = _point;
  } else {
    errno = EPROTONOSUPPORT;
    return -1;
  }
  errno = 0;

  // Unmount previous share if different
  if (! _mounted.empty()) {
    if (_mounted != share) {
      // Different share mounted: unmount
      umount();
    } else {
      // Same share mounted: nothing to do
      return 0;
    }
  }

  if (_classic_mount) {
    // Add mount options to command
    for (list<Option>::const_iterator i = options.begin(); i != options.end();
         i++) {
      command += "," + i->option();
    }
  }
  // Paths
  command += " " + share + " " + _point;

  // Issue mount command
  hlog_debug_arrow(1, "%s", command.c_str());
  command += " > /dev/null 2>&1";

  int result = system(command.c_str());
  if (result != 0) {
    errno = ETIMEDOUT;
  } else {
    errno = 0;
    _mounted = share;
  }
  return result;
}

int Share::umount() {
  if (! _mounted.empty()) {
    string command;
    if (_classic_mount) {
      command = "umount -fl ";
    } else
    if (_fuse_mount) {
      command = "fusermount -u ";
    }
    command += _point;
    hlog_debug_arrow(1, "%s", command.c_str());
    _mounted.erase();
    return system(command.c_str());
  }
  return 0;
}
