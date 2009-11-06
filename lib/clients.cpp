/*
     Copyright (C) 2006-2009  Herve Fache

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
#include "report.h"
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

struct Client::Private {
  list<ClientPath*> paths;
  string            subset_client;
  string            name;
  string            subset_server;
  string            host_or_ip;
  Path              list_file;
  string            protocol;
  list<Option>      options;
  bool              timeout_nowarning;
  list<string>      users;
  string            home_path;
  Config            config;
  string            mounted;
  bool              classic_mount;
  bool              fuse_mount;
  int               expire;
};

int Client::mountPath(
    const string&   backup_path,
    string&         path,
    const char*     mount_point) {
  string command;
  string share;
  errno = EINVAL;
  _d->classic_mount = false;
  _d->fuse_mount = false;

  // Determine share and path
  if (_d->protocol == "file") {
    command = "";
    share = "";
    path  = backup_path;
    return 0;
  } else
  if (_d->protocol == "nfs") {
    _d->classic_mount = true;
    command = "mount -t nfs -o ro,noatime,nolock,soft,timeo=30,intr";
    share = _d->host_or_ip + ":" + backup_path;
    path  = mount_point;
  } else
  if (_d->protocol == "smb") {
    _d->classic_mount = true;
    command = "mount -t cifs -o ro,nocase";
    if (backup_path.size() < 3) {
      return -1;
    }
    char drive_letter = backup_path[0];
    if (! isupper(drive_letter)) {
      return -1;
    }
    share = "//" + _d->host_or_ip + "/" + drive_letter + "$";
    path  = mount_point;
    path += "/" +  backup_path.substr(3);
  } else
  if (_d->protocol == "ssh") {
    _d->fuse_mount = true;
    const string username = getOption("username");
    const string password = getOption("password");
    if ((password == "") || (username == "")) {
      errno = EINVAL;
      return -1;
    }
    // echo <password> | sshfs -o password_stdin <user>@<server>:<path> <mount path>
    command = "echo " + password + " | sshfs -o password_stdin";
    share   = username + "@" + _d->host_or_ip + ":" + backup_path;
    path    = mount_point;
  } else {
    errno = EPROTONOSUPPORT;
    return -1;
  }
  errno = 0;

  // Unmount previous share if different
  if (_d->mounted != "") {
    if (_d->mounted != share) {
      // Different share mounted: unmount
      umount(mount_point);
    } else {
      // Same share mounted: nothing to do
      return 0;
    }
  }

  if (_d->classic_mount) {
    // Add mount options to command
    for (list<Option>::iterator i = _d->options.begin(); i != _d->options.end();
        i++ ){
      command += "," + i->option();
    }
  }
  // Paths
  command += " " + share + " " + mount_point;

  // Issue mount command
  out(debug, msg_standard, command.c_str(), 1);
  command += " > /dev/null 2>&1";

  int result = system(command.c_str());
  if (result != 0) {
    errno = ETIMEDOUT;
  } else {
    errno = 0;
    _d->mounted = share;
  }
  return result;
}

int Client::umount(
    const char*     mount_point) {
  if (_d->mounted != "") {
    string command;
    if (_d->classic_mount) {
      command = "umount -fl ";
    } else
    if (_d->fuse_mount) {
      command = "fusermount -u ";
    }
    command += mount_point;
    out(debug, msg_standard, command.c_str(), 1);
    _d->mounted = "";
    return system(command.c_str());
  }
  return 0;
}

int Client::readConfig(
    const char*     config_path,
    const Filters&  global_filters) {
  bool   failed  = false;

  // Open client configuration file
  Stream config_file(config_path);

  // Open client configuration file
  if (config_file.open(O_RDONLY)) {
    out(error, msg_standard, "Client configuration file not found", -1,
      config_path);
    return -1;
  }
  // Set up config syntax and grammar
  _d->config.clear();

  ConfigSyntax config_syntax;

  // subset
  config_syntax.add(new ConfigItem("subset", 0, 1, 1));
  // expire
  config_syntax.add(new ConfigItem("expire", 0, 1, 1));
  // users
  config_syntax.add(new ConfigItem("users", 0, 1, 1, -1));
  // ignore
  config_syntax.add(new ConfigItem("ignore", 0, 1, 1));
  // timeout_nowarning
  config_syntax.add(new ConfigItem("report_copy_error_once", 0, 1));
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
  if (_d->config.read(config_file,
                      Stream::flags_dos_catch | Stream::flags_accept_cr_lf,
                      config_syntax,
                      &errors) >= 0) {
    // Read client configuration file
    ClientPath* path   = NULL;
    Attributes* attr   = &attributes;
    ConfigLine* params;
    while (_d->config.line(&params) >= 0) {
      if ((*params)[0] == "subset") {
        _d->subset_client = (*params)[1];
      } else
      if ((*params)[0] == "expire") {
        int expire;
        if ((sscanf((*params)[1].c_str(), "%d", &expire) != 0)
        &&  (expire >= 0)) {
          _d->expire = expire * 3600 * 24;
        } else {
          out(error, msg_number, "Wrong expiration value",
            (*params).lineNo(), config_path);
          failed = true;
        }
      } else
      if ((*params)[0] == "users") {
        for (unsigned int i = 1; i < params->size(); i++) {
          addUser((*params)[i]);
        }
      } else
      if ((*params)[0] == "ignore") {
        Filter* filter = NULL;
        if (path != NULL) {
          filter = path->attributes.findFilter((*params)[1]);
        }
        if (filter == NULL) {
          filter = attributes.findFilter((*params)[1]);
        }
        if (filter == NULL) {
          filter = global_filters.find((*params)[1]);
        }
        if (filter == NULL) {
          out(error, msg_number, "Filter not found", (*params).lineNo(),
            config_path);
          failed = true;
        } else {
          attr->addIgnore(filter);
        }
      } else
      if ((*params)[0] == "report_copy_error_once") {
        attr->setReportCopyErrorOnce();
      } else
      if ((*params)[0] == "path") {
        // New backup path
        path = addClientPath((*params)[1]);
        if (path == NULL) {
          out(error, msg_number, "Path inside another", (*params).lineNo(),
            config_path);
          failed = true;
        } else {
          attr = &path->attributes;
          // Inherit some attributes when set
          *attr = attributes;
        }
      } else
      if ((*params)[0] == "filter") {
        if (attr->addFilter((*params)[1], (*params)[2]) == NULL) {
          out(error, msg_number, "Unsupported filter type",
            (*params).lineNo(), config_path);
          failed = true;
        }
      } else
      if ((*params)[0] == "condition") {
        string filter_type;
        bool   negated;
        if ((*params)[1][0] == '!') {
          filter_type = (*params)[1].substr(1);
          negated     = true;
        } else {
          filter_type = (*params)[1];
          negated     = false;
        }

        // Add specified filter
        if (filter_type == "filter") {
          Filter* subfilter = NULL;
          if (path != NULL) {
            subfilter = path->attributes.findFilter((*params)[2]);
          }
          if (subfilter == NULL) {
            subfilter = attributes.findFilter((*params)[2]);
          }
          if (subfilter == NULL) {
            subfilter = global_filters.find((*params)[2]);
          }
          if (subfilter == NULL) {
            out(error, msg_number, "Filter not found", (*params).lineNo(),
              config_path);
            failed = 2;
          } else {
            attr->addFilterCondition(new Condition(Condition::filter,
              subfilter, negated));
          }
        } else {
          switch (attr->addFilterCondition(filter_type, (*params)[2].c_str(),
              negated)) {
            case -2:
              out(error, msg_number, "Unsupported condition type",
                (*params).lineNo(), config_path);
              failed = true;
              break;
            case -1:
              out(error, msg_number, "Failed to add condition",
                (*params).lineNo(), config_path);
              failed = true;
          }
        }
      } else
      if (path != NULL) {
        // Path attributes
        if (((*params)[0] == "compress")
        ||  ((*params)[0] == "no_compress")) {
          Filter* filter = path->attributes.findFilter((*params)[1]);
          if (filter == NULL) {
            filter = attributes.findFilter((*params)[1]);
          }
          if (filter == NULL) {
            filter = global_filters.find((*params)[1]);
          }
          if (filter == NULL) {
            out(error, msg_number, "Filter not found", (*params).lineNo(),
              config_path);
            failed = true;
          } else {
            if ((*params)[0] == "compress") {
              path->setCompress(filter);
            } else
            if ((*params)[0] == "no_compress") {
              path->setNoCompress(filter);
            }
          }
        } else
        if ((*params)[0] == "parser") {
          switch (path->addParser((*params)[1], (*params)[2])) {
            case 1:
              out(error, msg_number, "Unsupported parser type",
                (*params).lineNo(), config_path);
              failed = true;
              break;
            case 2:
              out(error, msg_number, "Unsupported parser mode",
                (*params).lineNo(), config_path);
              failed = true;
              break;
          }
        }
      } else {
        out(error, msg_number, "Keyword unexpected ouside of path block",
          (*params).lineNo(), config_path);
        failed = true;
      }
    }
    // Close client configuration file
    config_file.close();
  } else {
    errors.show();
  }
  if (! failed) {
    show(1);
  }
  return failed ? -1 : 0;
}

Client::Client(const string& name, const string& subset) : _d(new Private) {
  _d->name                   = name;
  _d->subset_server          = subset;
  _d->host_or_ip             = name;
  _d->expire                 = -1;
  _d->timeout_nowarning      = false;
}

Client::~Client() {
  for (list<ClientPath*>::iterator i = _d->paths.begin(); i != _d->paths.end();
      i++) {
    delete *i;
  }
  delete _d;
}

const char* Client::name() const {
  return _d->name.c_str();
}

string Client::internalName() const {
  if (_d->subset_server.empty()) {
    return _d->name;
  } else {
    return _d->name + "." + _d->subset_server;
  }
}

void Client::addOption(const string& value) {
  _d->options.push_back(Option("", value));
}

void Client::addOption(const string& name, const string& value) {
  _d->options.push_back(Option(name, value));
}

const string Client::getOption(const string& name) const {
  for (list<Option>::const_iterator i = _d->options.begin();
      i != _d->options.end(); i++ ) {
    if (i->name() == name) {
      return i->value();
    }
  }
  return "";
}

void Client::addUser(const string& user) {
  _d->users.push_back(user);
}

void Client::setHostOrIp(string value) {
  _d->host_or_ip = value;
}

bool Client::setProtocol(string value) {
  _d->protocol = value;
  return value != "file";
}

void Client::setTimeOutNoWarning() {
  _d->timeout_nowarning = true;
}

void Client::setListfile(const char* value) {
  _d->list_file = value;
  _d->list_file.fromDos();
  _d->list_file.noTrailingSlashes();
}

const char* Client::listfile() const {
  return _d->list_file;
}

void Client::setExpire(int expire) {
  _d->expire = expire;
}

void Client::setBasePath(const string& home_path) {
  _d->home_path = home_path;
}

ClientPath* Client::addClientPath(const string& name) {
  ClientPath* path;
  if (name[0] == '~') {
    path = new ClientPath((_d->home_path + &name[1]).c_str());
  } else {
    path = new ClientPath(name.c_str());
  }
  list<ClientPath*>::iterator i = _d->paths.begin();
  while ((i != _d->paths.end())
      && (Path::compare((*i)->path(), path->path()) < 0)) {
    i++;
  }
  list<ClientPath*>::iterator j = _d->paths.insert(i, path);
  // Check that we don't have a path inside another, such as
  // '/home' and '/home/user'
  int jlen = strlen((*j)->path());
  if (i != _d->paths.end()) {
    int ilen = strlen((*i)->path());
    if ((ilen > jlen)
    && (Path::compare((*i)->path(), (*j)->path(), jlen) == 0)) {
      return NULL;
    }
    i = j;
    i--;
  }
  if (i != _d->paths.end()) {
    int ilen = strlen((*i)->path());
    if ((ilen < jlen)
    && (Path::compare((*i)->path(), (*j)->path(), ilen) == 0)) {
      return NULL;
    }
  }
  return path;
}

int Client::backup(
    Database&       db,
    const Filters&  global_filters,
    const char*     mount_point,
    bool            config_check) {
  bool    first_mount_try = true;
  bool    failed = false;
  string  share;

  // Do not print this if in user-mode backup
  if (_d->protocol != "file") {
    stringstream s;
    s << "Trying client '" << internalName() << "' using protocol '"
      << _d->protocol << "'";
    out(info, msg_standard, s.str().c_str());
  }

  if (_d->list_file.length() != 0) {
    string config_path;
    if (mountPath(string(_d->list_file.dirname()), config_path, mount_point)) {
      switch (errno) {
        case ETIMEDOUT:
          if (! _d->timeout_nowarning) {
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
          out(error, msg_errno, _d->protocol.c_str(), errno,
            internalName().c_str());
      }
      return -1;
    }
    first_mount_try = false;
    if (config_path.size() != 0) {
      config_path += "/";
    }
    config_path += Path::basename(_d->list_file);

    if (readConfig(config_path.c_str(), global_filters) != 0) {
      failed = true;
    } else if (_d->subset_client !=  _d->subset_server) {
      stringstream s;
      s << "Subsets don't match in server and client configuration files: '"
        << _d->subset_server << "' != '" << _d->subset_client << "', skipping";
      out(info, msg_standard, s.str().c_str());
      failed = true;
    } else {
      // Save configuration
      Directory dir(Path(db.path(), ".configs"));
      if (dir.create() < 0) {
        out(error, msg_errno, "creating configuration directory", errno);
      } else {
        Stream stream(Path(dir.path(), internalName().c_str()));
        if (stream.open(O_WRONLY) >= 0) {
          if (_d->config.write(stream) < 0) {
            out(error, msg_errno, "writing configuration file", errno);
          }
          stream.close();
        } else {
          out(error, msg_errno, "creating configuration file", errno);
        }
      }
    }
  }
  if (! failed) {
    // Do not print this if in user-mode backup
    if (_d->home_path.length() == 0) {
      out(info, msg_standard, internalName().c_str(), -1, "Backing up client");
    }
    // Backup
    if (_d->paths.empty()) {
      out(warning, msg_standard, "No paths specified", -1,
        internalName().c_str());
      failed = true;
    } else if (! config_check) {
      if (db.openClient(internalName().c_str(), _d->expire) >= 0) {
        bool abort = false;
        for (list<ClientPath*>::iterator i = _d->paths.begin();
            i != _d->paths.end(); i++) {
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
            if (! _d->timeout_nowarning) {
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
                out(error, msg_standard, "Aborting client");
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
  out(debug, msg_standard, _d->name.c_str(), level++, "Client");
  if (! _d->subset_server.empty()) {
    out(debug, msg_standard, _d->subset_server.c_str(), level,
      "Required subset");
  }
  if (! _d->subset_client.empty()) {
    out(debug, msg_standard, _d->subset_client.c_str(), level, "Subset");
  }
  out(debug, msg_standard, _d->protocol.c_str(), level, "Protocol");
  if (_d->host_or_ip != _d->name) {
    out(debug, msg_standard, _d->host_or_ip.c_str(), level, "Hostname");
  }
  if (! _d->options.empty()) {
    stringstream s;
    bool         first = true;
    for (list<Option>::const_iterator i = _d->options.begin();
        i != _d->options.end(); i++ ) {
      if (first) {
        first = false;
      } else {
        s << ", ";
      }
      s << i->option();
    }
    out(debug, msg_standard, s.str().c_str(), level, "Options");
  }
  if (! _d->users.empty()) {
    stringstream s;
    bool         first = true;
    for (list<string>::const_iterator i = _d->users.begin();
        i != _d->users.end(); i++ ) {
      if (first) {
        first = false;
      } else {
        s << ", ";
      }
      s << *i;
    }
    out(debug, msg_standard, s.str().c_str(), level, "Users");
  }
  if (_d->timeout_nowarning) {
    out(debug, msg_standard, "No warning on time out", level);
  }
  if (_d->list_file.length() != 0) {
    out(debug, msg_standard, _d->list_file, level, "Config");
  }
  {
    stringstream s;
    if (_d->expire >= 0) {
      s << _d->expire << "s (" << _d->expire / 86400 << "d)";
    } else {
      s << "none";
    }
    out(debug, msg_standard, s.str().c_str(), level, "Expiry");
  }
  attributes.show(level);
  if (_d->paths.size() > 0) {
    out(debug, msg_standard, "", level, "Paths");
    for (list<ClientPath*>::iterator i = _d->paths.begin();
        i != _d->paths.end(); i++) {
      (*i)->show(level + 1);
    }
  }
}
