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

#include <list>
#include <string>
#include <sstream>

using namespace std;

#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include "hbackup.h"
#include "files.h"
#include "hreport.h"
#include "configuration.h"
#include "conditions.h"
#include "filters.h"
#include "parsers.h"
#include "opdata.h"
#include "db.h"
#include "attributes.h"
#include "paths.h"
#include "clients.h"

using namespace hbackup;
using namespace hreport;

int Client::mountPath(
    const string&   backup_path,
    string&         path,
    const char*     mount_point) {
  string command;
  string share;
  errno = EINVAL;
  _classic_mount = false;
  _fuse_mount = false;

  // Determine share and path
  if (_protocol == "file") {
    command = "";
    share = "";
    path  = backup_path;
    return 0;
  } else
  if (_protocol == "nfs") {
    _classic_mount = true;
    command = "mount -t nfs -o ro,noatime,nolock,soft,timeo=30,intr";
    share = _host_or_ip + ":" + backup_path;
    path  = mount_point;
  } else
  if (_protocol == "smb") {
    _classic_mount = true;
    command = "mount -t cifs -o ro,nocase";
    if (backup_path.size() < 3) {
      return -1;
    }
    char drive_letter = backup_path[0];
    if (! isupper(drive_letter)) {
      return -1;
    }
    share = "//" + _host_or_ip + "/" + drive_letter + "$";
    path  = mount_point;
    path += "/" +  backup_path.substr(3);
  } else
  if (_protocol == "ssh") {
    _fuse_mount = true;
    const string username = getOption("username");
    if (username != "") {
      share = username + "@";
    }
    const string password = getOption("password");
    if (password != "") {
      command = "echo " + password + " | sshfs -o password_stdin";
    } else {
      command = "sshfs";
    }
    // echo <password> | sshfs -o password_stdin <user>@<server>:<path> <mount path>
    share   += _host_or_ip + ":" + backup_path;
    path    = mount_point;
  } else {
    errno = EPROTONOSUPPORT;
    return -1;
  }
  errno = 0;

  // Unmount previous share if different
  if (_mounted != "") {
    if (_mounted != share) {
      // Different share mounted: unmount
      umount(mount_point);
    } else {
      // Same share mounted: nothing to do
      return 0;
    }
  }

  if (_classic_mount) {
    // Add mount options to command
    for (list<Option>::iterator i = _options.begin(); i != _options.end();
        i++ ){
      command += "," + i->option();
    }
  }
  // Paths
  command += " " + share + " " + mount_point;

  // Issue mount command
  out(debug, msg_standard, command.c_str(), 1, NULL);
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

int Client::umount(
    const char*     mount_point) {
  if (_mounted != "") {
    string command;
    if (_classic_mount) {
      command = "umount -fl ";
    } else
    if (_fuse_mount) {
      command = "fusermount -u ";
    }
    command += mount_point;
    out(debug, msg_standard, command.c_str(), 1, NULL);
    _mounted = "";
    return system(command.c_str());
  }
  return 0;
}

int Client::readConfig(
    const char*     config_path) {
  // Open client configuration file
  Stream config_file(config_path);

  // Open client configuration file
  if (config_file.open(O_RDONLY)) {
    out(error, msg_standard, "Client configuration file not found", -1,
      config_path);
    return -1;
  }
  // Set up config syntax and grammar
  _config.clear();

  ConfigSyntax config_syntax;

  // subset
  config_syntax.add(new ConfigItem("subset", 0, 1, 1));
  // expire
  config_syntax.add(new ConfigItem("expire", 0, 1, 1));
  // users
  config_syntax.add(new ConfigItem("users", 0, 1, 1, -1));
  // timeout_nowarning
  config_syntax.add(new ConfigItem("report_copy_error_once", 0, 1));
  // ignore
  config_syntax.add(new ConfigItem("ignore", 0, 1, 1));
  // filter
  {
    ConfigItem* filter = new ConfigItem("filter", 0, 0, 2);
    config_syntax.add(filter);

    // condition
    filter->add(new ConfigItem("condition", 1, 0, 2));
  }
  // path
  {
    ConfigItem* path = new ConfigItem("path", 1, 0, 1);
    config_syntax.add(path);
    // parser
    path->add(new ConfigItem("parser", 0, 0, 2));
    // filter
    {
      ConfigItem* filter = new ConfigItem("filter", 0, 0, 2);
      path->add(filter);
      // condition
      filter->add(new ConfigItem("condition", 1, 0, 2));
    }
    // timeout_nowarning
    path->add(new ConfigItem("report_copy_error_once", 0, 1));
    // ignore
    path->add(new ConfigItem("ignore", 0, 1, 1));
    // compress
    path->add(new ConfigItem("compress", 0, 1, 1));
    // no_compress
    path->add(new ConfigItem("no_compress", 0, 1, 1));
  }

  out(debug, msg_standard, internalName().c_str(), 1,
    "Reading client configuration file");
  ConfigErrors errors;
  bool failed = false;
  if (_config.read(config_file,
        Stream::flags_dos_catch | Stream::flags_accept_cr_lf, config_syntax,
        this, &errors) < 0) {
    errors.show();
    failed = true;
  } else {
    show(1);
  }
  // Close client configuration file
  config_file.close();
  return failed ? -1 : 0;
}

Client::Client(const string& name, const Attributes& a, const string& subset)
    : _attributes(a) {
  _name = name;
  _subset_server = subset;
  _host_or_ip = name;
  _expire = -1;
  _timeout_nowarning = false;
}

Client::~Client() {
  for (list<ClientPath*>::iterator i = _paths.begin(); i != _paths.end();
      i++) {
    delete *i;
  }
}

ConfigObject* Client::configChildFactory(
    const vector<string>& params,
    const char*           file_path,
    size_t                line_no) {
  ConfigObject* co = NULL;
  const string& keyword = params[0];
  if (keyword == "subset") {
    _subset_client = params[1];
    co = this;
  } else
  if (keyword == "expire") {
    int expire;
    if ((sscanf(params[1].c_str(), "%d", &expire) != 0) && (expire >= 0)) {
      _expire = expire * 3600 * 24;
      co = this;
    } else {
      hlog_error("%s:%zd incorrect expiration value '%s'",
        file_path, line_no, params[1].c_str());
    }
  } else
  if (keyword == "users") {
    for (size_t i = 1; i < params.size(); i++) {
      _users.push_back(params[i]);
    }
    co = this;
  } else
  if (keyword == "hostname") {
    _host_or_ip = params[1];
    co = this;
  } else
  if (keyword == "protocol") {
    _protocol = params[1];
    co = this;
  } else
  if (keyword == "option") {
    if (params.size() == 2) {
      _options.push_back(Option("", params[1]));
    } else {
      _options.push_back(Option(params[1], params[2]));
    }
    co = this;
  } else
  if (keyword == "timeout_nowarning") {
    _timeout_nowarning = true;
    co = this;
  } else
  if (keyword == "config") {
    setListfile(params[1].c_str());
    co = this;
  } else
  if (keyword == "path") {
    co = addClientPath(params[1]);
    if (co == NULL) {
      hlog_error("%s:%zd path inside another '%s'",
        file_path, line_no, params[1].c_str());
    }
  } else
  {
    co = _attributes.configChildFactory(params, file_path, line_no);
  }
  return co;
}

string Client::internalName() const {
  if (_subset_server.empty()) {
    return _name;
  } else {
    return _name + "." + _subset_server;
  }
}

const string Client::getOption(const string& name) const {
  for (list<Option>::const_iterator i = _options.begin();
      i != _options.end(); i++ ) {
    if (i->name() == name) {
      return i->value();
    }
  }
  return "";
}

void Client::setListfile(const char* value) {
  _list_file = value;
  _list_file.fromDos();
  _list_file.noTrailingSlashes();
}

ClientPath* Client::addClientPath(const string& name) {
  ClientPath* path;
  if (name[0] == '~') {
    path = new ClientPath((_home_path + &name[1]).c_str(), _attributes);
  } else {
    path = new ClientPath(name.c_str(), _attributes);
  }
  list<ClientPath*>::iterator i = _paths.begin();
  while ((i != _paths.end())
      && (Path::compare((*i)->path(), path->path()) < 0)) {
    i++;
  }
  list<ClientPath*>::iterator j = _paths.insert(i, path);
  // Check that we don't have a path inside another, such as
  // '/home' and '/home/user'
  size_t jlen = strlen((*j)->path());
  if (i != _paths.end()) {
    size_t ilen = strlen((*i)->path());
    if ((ilen > jlen)
    && (Path::compare((*i)->path(), (*j)->path(), jlen) == 0)) {
      return NULL;
    }
    i = j;
    i--;
  }
  if (i != _paths.end()) {
    size_t ilen = strlen((*i)->path());
    if ((ilen < jlen)
    && (Path::compare((*i)->path(), (*j)->path(), ilen) == 0)) {
      return NULL;
    }
  }
  return path;
}

int Client::backup(
    Database&       db,
    const char*     mount_point,
    bool            config_check) {
  bool    first_mount_try = true;
  bool    failed = false;
  string  share;

  // Do not print this if in user-mode backup
  if (_protocol != "file") {
    stringstream s;
    s << "Trying client '" << internalName() << "' using protocol '"
      << _protocol << "'";
    out(info, msg_standard, s.str().c_str(), -1, NULL);
  }

  if (_list_file.length() != 0) {
    string config_path;
    if (mountPath(string(_list_file.dirname()), config_path, mount_point)) {
      switch (errno) {
        case ETIMEDOUT:
          if (! _timeout_nowarning) {
            out(warning, msg_errno, "connecting to client", errno,
              internalName().c_str());
            return 1;
          } else {
            out(info, msg_errno, "connecting to client", errno,
              internalName().c_str());
            return 0;
          }
          break;
        default:
          out(error, msg_errno, _protocol.c_str(), errno,
            internalName().c_str());
      }
      return -1;
    }
    first_mount_try = false;
    if (config_path.size() != 0) {
      config_path += "/";
    }
    config_path += Path::basename(_list_file);

    if (readConfig(config_path.c_str()) != 0) {
      failed = true;
    } else if (_subset_client !=  _subset_server) {
      stringstream s;
      s << "Subsets don't match in server and client configuration files: '"
        << _subset_server << "' != '" << _subset_client << "', skipping";
      out(info, msg_standard, s.str().c_str(), -1, NULL);
      failed = true;
    } else {
      // Save configuration
      Directory dir(Path(db.path(), ".configs"));
      if (dir.create() < 0) {
        out(error, msg_errno, "creating configuration directory", errno, NULL);
      } else {
        Stream stream(Path(dir.path(), internalName().c_str()));
        if (stream.open(O_WRONLY) >= 0) {
          if (_config.write(stream) < 0) {
            out(error, msg_errno, "writing configuration file", errno, NULL);
          }
          stream.close();
        } else {
          out(error, msg_errno, "creating configuration file", errno, NULL);
        }
      }
    }
  }
  if (! failed) {
    // Do not print this if in user-mode backup
    if (_home_path.length() == 0) {
      out(info, msg_standard, internalName().c_str(), -1, "Backing up client");
    }
    // Backup
    if (_paths.empty()) {
      out(warning, msg_standard, "No paths specified", -1,
        internalName().c_str());
      failed = true;
    } else if (! config_check) {
      if (db.openClient(internalName().c_str(), _expire) >= 0) {
        bool abort = false;
        for (list<ClientPath*>::iterator i = _paths.begin();
            i != _paths.end(); i++) {
          if (aborting()) {
            break;
          }
          string backup_path;
          out(info, msg_standard, (*i)->path(), -1, "Backing up path");

          if (mountPath((*i)->path(), backup_path, mount_point)) {
            if (! first_mount_try) {
              out(error, msg_errno, "- aborting client", errno,
                internalName().c_str());
            } else
            if (! _timeout_nowarning) {
              out(warning, msg_errno, "connecting to client", errno,
                internalName().c_str());
            } else {
              out(info, msg_errno, "connecting to client", errno,
                internalName().c_str());
            }
            abort = true;
          } else {
            first_mount_try = false;
            if ((*i)->parse(db, backup_path.c_str(), internalName().c_str())) {
              // prepare_share sets errno
              if (! aborting()) {
                out(error, msg_standard, "Aborting client", -1, NULL);
              }
              abort  = true;
              failed = true;
            }
          }
          if (abort) {
            break;
          }
        }
        db.closeClient(abort);
      }
    }
  }
  umount(mount_point); // does not change errno
  return failed ? -1 : 0;
}

void Client::show(int level) const {
  out(debug, msg_standard, _name.c_str(), level++, "Client");
  if (! _subset_server.empty()) {
    out(debug, msg_standard, _subset_server.c_str(), level,
      "Required subset");
  }
  if (! _subset_client.empty()) {
    out(debug, msg_standard, _subset_client.c_str(), level, "Subset");
  }
  out(debug, msg_standard, _protocol.c_str(), level, "Protocol");
  if (_host_or_ip != _name) {
    out(debug, msg_standard, _host_or_ip.c_str(), level, "Hostname");
  }
  if (! _options.empty()) {
    stringstream s;
    bool         first = true;
    for (list<Option>::const_iterator i = _options.begin();
        i != _options.end(); i++ ) {
      if (first) {
        first = false;
      } else {
        s << ", ";
      }
      s << i->option();
    }
    out(debug, msg_standard, s.str().c_str(), level, "Options");
  }
  if (! _users.empty()) {
    stringstream s;
    bool         first = true;
    for (list<string>::const_iterator i = _users.begin();
        i != _users.end(); i++ ) {
      if (first) {
        first = false;
      } else {
        s << ", ";
      }
      s << *i;
    }
    out(debug, msg_standard, s.str().c_str(), level, "Users");
  }
  if (_timeout_nowarning) {
    out(debug, msg_standard, "No warning on time out", level, NULL);
  }
  if (_list_file.length() != 0) {
    out(debug, msg_standard, _list_file, level, "Config");
  }
  {
    stringstream s;
    if (_expire >= 0) {
      s << _expire << "s (" << _expire / 86400 << "d)";
    } else {
      s << "none";
    }
    out(debug, msg_standard, s.str().c_str(), level, "Expiry");
  }
  _attributes.show(level);
  if (_paths.size() > 0) {
    out(debug, msg_standard, "", level, "Paths");
    for (list<ClientPath*>::const_iterator i = _paths.begin();
        i != _paths.end(); i++) {
      (*i)->show(level + 1);
    }
  }
}
