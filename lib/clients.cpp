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
    hlog_debug_arrow(1, "%s", command.c_str());
    _mounted = "";
    return system(command.c_str());
  }
  return 0;
}

int Client::readConfig(
    const char*     config_path) {
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
  // parser plugins
  config_syntax.add(new ConfigItem("parsers_dir", 0, 0, 1));
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

  hlog_debug_arrow(1, "Reading client configuration file for '%s'",
    internalName().c_str());
  ConfigErrors errors;
  bool failed = false;
  _own_parsers = false;
  if (_config.read(config_path, Config::flags_dos_catch, config_syntax, this,
        &errors) < 0) {
    errors.show();
    failed = true;
  } else {
    if (! _own_parsers) {
      _parsers_manager.loadPlugins("/usr/local/lib/hbackup");
      _parsers_manager.loadPlugins("/usr/lib/hbackup");
    }
    show(1);
  }
  return failed ? -1 : 0;
}

Client::Client(
    const string&     name,
    const Attributes& attributes,
    ParsersManager&   parsers_manager,
    const string&     subset)
    : _attributes(attributes), _parsers_manager(parsers_manager),
      _own_parsers(false) {
  _name = name;
  _subset_server = subset;
  if (_subset_server.empty()) {
    _internal_name = _name;
  } else {
    _internal_name = _name + "." + _subset_server;
  }
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
  if (keyword == "parsers_dir") {
    _parsers_manager.loadPlugins(params[1].c_str());
    _own_parsers = true;
    co = this;
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

const string& Client::internalName() const {
  return _internal_name;
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
    path = new ClientPath((_home_path + &name[1]).c_str(), _attributes,
      _parsers_manager);
  } else {
    path = new ClientPath(name.c_str(), _attributes, _parsers_manager);
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
    hlog_info("Trying client '%s' using protocol '%s'",
      internalName().c_str(), _protocol.c_str());
  }

  if (_list_file.length() != 0) {
    string config_path;
    if (mountPath(string(_list_file.dirname()), config_path, mount_point)) {
      switch (errno) {
        case ETIMEDOUT:
          if (! _timeout_nowarning) {
            hlog_warning("%s connecting to client '%s'", strerror(errno),
              internalName().c_str());
            return 1;
          } else {
            hlog_info("%s connecting to client '%s'", strerror(errno),
              internalName().c_str());
            return 0;
          }
          break;
        default:
          hlog_error("%s '%s' for '%s'", strerror(errno), _protocol.c_str(),
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
    } else
    if (_subset_client != _subset_server) {
      hlog_info("Subsets don't match in server and client configuration files: "
        "'%s' != '%s', skipping",
        _subset_server.c_str(), _subset_client.c_str());
      failed = true;
    } else
    {
      // Save configuration
      Directory dir(Path(db.path(), ".configs"));
      if (dir.create() < 0) {
        hlog_error("%s creating configuration directory '%s'", strerror(errno),
          dir.path());
      } else {
        _config.write(Path(dir.path(), internalName().c_str()));
      }
    }
  }
  if (! failed) {
    // Do not print this if in user-mode backup
    if (_home_path.length() == 0) {
      hlog_info("Backing up client '%s'", internalName().c_str());
    }
    // Backup
    if (_paths.empty()) {
      hlog_warning("No paths specified fot '%s'", internalName().c_str());
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
          hlog_info("Backing up path '%s'", (*i)->path());

          if (mountPath((*i)->path(), backup_path, mount_point)) {
            if (! first_mount_try) {
              hlog_error("%s connecting to client '%s', aborting client",
                strerror(errno), internalName().c_str());
            } else
            if (! _timeout_nowarning) {
              hlog_warning("%s connecting to client '%s'", strerror(errno),
                internalName().c_str());
            } else {
              hlog_info("%s connecting to client '%s'", strerror(errno),
                internalName().c_str());
            }
            abort = true;
          } else {
            first_mount_try = false;
            if ((*i)->parse(db, backup_path.c_str(), internalName().c_str())) {
              // prepare_share sets errno
              if (! aborting()) {
                hlog_error("Aborting client");
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
  hlog_debug_arrow(level++, "Client: %s", _name.c_str());
  if (! _subset_server.empty()) {
    hlog_debug_arrow(level, "Required subset: %s", _subset_server.c_str());
  }
  if (! _subset_client.empty()) {
    hlog_debug_arrow(level, "Subset: %s", _subset_client.c_str());
  }
    hlog_debug_arrow(level, "Protocol: %s", _protocol.c_str());
  if (_host_or_ip != _name) {
    hlog_debug_arrow(level, "Hostname: %s", _host_or_ip.c_str());
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
    hlog_debug_arrow(level, "Options: %s", s.str().c_str());
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
    hlog_debug_arrow(level, "Users: %s", s.str().c_str());
  }
  if (_own_parsers) {
    _parsers_manager.show(level);
  }
  if (_timeout_nowarning) {
    hlog_debug_arrow(level, "No warning on time out");
  }
  if (_list_file.length() != 0) {
    hlog_debug_arrow(level, "Config: %s", _list_file.c_str());
  }
  {
    if (_expire >= 0) {
      hlog_debug_arrow(level, "Expiry: %d second(s) / %d day(s)",
        _expire, _expire / 86400);
    } else {
      hlog_debug_arrow(level, "Expiry: none");
    }
  }
  _attributes.show(level);
  if (_paths.size() > 0) {
    hlog_debug_arrow(level, "Paths:");
    for (list<ClientPath*>::const_iterator i = _paths.begin();
        i != _paths.end(); i++) {
      (*i)->show(level + 1);
    }
  }
}
