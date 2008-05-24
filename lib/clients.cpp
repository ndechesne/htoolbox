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

#include <list>
#include <string>
#include <sstream>
#include <errno.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "report.h"
#include "configuration.h"
#include "conditions.h"
#include "filters.h"
#include "parsers.h"
#include "opdata.h"
#include "db.h"
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
  bool              timeout_nowarning;
  list<Option>      options;
  //
  bool              initialised;
  int               expire;
  string            home_path;
  string            mount_point;
  string            mounted;
  Filters           filters;
};

int Client::mountPath(
    const string&   backup_path,
    string&         path) {
  string command = "mount ";
  string share;
  errno = EINVAL;

  // Determine share and path
  if (_d->protocol == "file") {
    share = "";
    path  = backup_path;
  } else
  if (_d->protocol == "nfs") {
    share = _d->host_or_ip + ":" + backup_path;
    path  = _d->mount_point;
  } else
  if (_d->protocol == "smb") {
    if (backup_path.size() < 3) {
      return -1;
    }
    char drive_letter = backup_path[0];
    if ((drive_letter < 'A') || (drive_letter > 'Z')) {
      return -1;
    }
    share = "//" + _d->host_or_ip + "/" + drive_letter + "$";
    path  = _d->mount_point + "/" +  backup_path.substr(3);
  } else {
    errno = EPROTONOSUPPORT;
    return -1;
  }
  errno = 0;

  // Check what is mounted
  if (_d->mounted != "") {
    if (_d->mounted != share) {
      // Different share mounted: unmount
      umount();
    } else {
      // Same share mounted: nothing to do
      return 0;
    }
  }

  // Build mount command
  if (_d->protocol == "file") {
    return 0;
  } else {
    // Check that mount dir exists, if not create it
    if (Directory(_d->mount_point.c_str()).create() < 0) {
      return -1;
    }

    // Set protocol and default options
    if (_d->protocol == "nfs") {
      command += "-t nfs -o ro,noatime,nolock,soft,timeo=30,intr";
    } else
    if (_d->protocol == "smb") {
      // codepage=858
      command += "-t cifs -o ro,noatime,nocase";
    }
  }
  // Additional options
  for (list<Option>::iterator i = _d->options.begin(); i != _d->options.end();
      i++ ){
    command += "," + i->option();
  }
  // Paths
  command += " " + share + " " + _d->mount_point;

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

int Client::umount() {
  if (_d->mounted != "") {
    string command = "umount -fl ";

    command += _d->mount_point;
    out(debug, msg_standard, command.c_str(), 1);
    _d->mounted = "";
    return system(command.c_str());
  }
  return 0;
}

int Client::readConfig(
    const char*     list_path,
    const Filters&  global_filters) {
  bool   failed  = false;

  // Open client configuration file
  Stream config_file(list_path);

  // Open client configuration file
  if (config_file.open("r")) {
    out(error, msg_standard, "Client configuration file not found", -1,
      list_path);
    return -1;
  }
  // Set up config syntax and grammar
  Config config;

  // subset
  config.add(new ConfigItem("subset", 0, 1, 1));
  // expire
  config.add(new ConfigItem("expire", 0, 1, 1));
  // filter
  {
    ConfigItem* filter = new ConfigItem("filter", 0, 0, 2);
    config.add(filter);

    // condition
    filter->add(new ConfigItem("condition", 1, 0, 2));
  }
  // path
  {
    ConfigItem* path = new ConfigItem("path", 1, 0, 1);
    config.add(path);
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
  }

  out(debug, msg_standard, internalName().c_str(), 1,
    "Reading client configuration file");
  if (config.read(config_file,
      Stream::flags_dos_catch | Stream::flags_accept_cr_lf) >= 0) {
    // Read client configuration file
    ClientPath* path   = NULL;
    Filter*     filter = NULL;
    ConfigLine* params;
    while (config.line(&params) >= 0) {
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
            (*params).lineNo(), list_path);
          failed = true;
        }
      } else
      if ((*params)[0] == "path") {
        filter = NULL;
        // New backup path
        path = addClientPath((*params)[1]);
        if (path == NULL) {
          out(error, msg_number, "Path inside another", (*params).lineNo(),
            list_path);
          failed = true;
        }
      } else
      if ((*params)[0] == "filter") {
        if (path == NULL) {
          // Client-wide filter
          filter = addFilter((*params)[1], (*params)[2]);
        } else {
          // Path-wide filter
          filter = path->addFilter((*params)[1], (*params)[2]);
        }
        if (filter == NULL) {
          out(error, msg_number, "Unsupported filter type",
            (*params).lineNo(), list_path);
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
            subfilter = path->findFilter((*params)[2]);
          }
          if (subfilter == NULL) {
            subfilter = findFilter((*params)[2]);
          }
          if (subfilter == NULL) {
            subfilter = global_filters.find((*params)[2]);
          }
          if (subfilter == NULL) {
            out(error, msg_number, "Filter not found", (*params).lineNo(),
              list_path);
            failed = 2;
          } else {
            filter->add(new Condition(Condition::filter, subfilter,
              negated));
          }
        } else {
          switch (filter->add(filter_type, (*params)[2].c_str(), negated)) {
            case -2:
              out(error, msg_number, "Unsupported condition type",
                (*params).lineNo(), list_path);
              failed = true;
              break;
            case -1:
              out(error, msg_number, "Failed to add condition",
                (*params).lineNo(), list_path);
              failed = true;
          }
        }
      } else
      // Path attributes
      if (((*params)[0] == "ignore") || ((*params)[0] == "compress")) {
        Filter* filter = path->findFilter((*params)[1]);
        if (filter == NULL) {
          filter = _d->filters.find((*params)[1]);
        }
        if (filter == NULL) {
          filter = global_filters.find((*params)[1]);
        }
        if (filter == NULL) {
          out(error, msg_number, "Filter not found", (*params).lineNo(),
            list_path);
          failed = true;
        } else {
          if ((*params)[0] == "ignore") {
            path->setIgnore(filter);
          } else {
            path->setCompress(filter);
          }
        }
      } else
      if ((*params)[0] == "parser") {
        switch (path->addParser((*params)[1], (*params)[2])) {
          case 1:
            out(error, msg_number, "Unsupported parser type",
              (*params).lineNo(), list_path);
            failed = true;
            break;
          case 2:
            out(error, msg_number, "Unsupported parser mode",
              (*params).lineNo(), list_path);
            failed = true;
            break;
        }
      }
    }
    // Close client configuration file
    config_file.close();
  }
  if (! failed) {
    show(1);
  }
  return failed ? -1 : 0;
}

Client::Client(const string& name, const string& subset) : _d(new Private) {
  _d->name              = name;
  _d->subset_server     = subset;
  _d->host_or_ip        = name;
  _d->initialised       = false;
  _d->expire            = -1;
  _d->timeout_nowarning = false;
}

Client::~Client() {
  for (list<ClientPath*>::iterator i = _d->paths.begin(); i != _d->paths.end();
      i++) {
    delete *i;
  }
  delete _d;
}

const string& Client::name() const {
  return _d->name;
}

const string& Client::subset() const {
  return _d->subset_server;
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

void Client::setHostOrIp(string value) {
  _d->host_or_ip = value;
}

void Client::setProtocol(string value) {
  _d->protocol = value;
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

bool Client::initialised() const {
  return _d->initialised;
}

void Client::setInitialised() {
  _d->initialised = true;
}

void Client::setBasePath(const string& home_path) {
  _d->home_path = home_path;
}

void Client::setMountPoint(const string& mount_point) {
  _d->mount_point = mount_point;
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

Filter* Client::addFilter(const string& type, const string& name) {
  return _d->filters.add(type, name);
}

Filter* Client::findFilter(const string& name) const {
  return _d->filters.find(name);
}

int Client::backup(
    Database&       db,
    const Filters&  global_filters,
    bool            config_check) {
  bool    first_mount_try = true;
  bool    failed = false;
  string  share;

  // Do not print this if in user-mode backup
  if (_d->home_path.length() == 0) {
    stringstream s;
    s << "Trying client '" << internalName() << "' using protocol '"
      << _d->protocol << "'";
    out(info, msg_standard, s.str().c_str());
  }

  if (_d->list_file.length() != 0) {
    string list_path;
    if (mountPath(string(_d->list_file.dirname()), list_path)) {
      switch (errno) {
        case EPROTONOSUPPORT:
          out(error, msg_errno, _d->protocol.c_str(), errno,
            internalName().c_str());
          return 1;
        case ETIMEDOUT:
          if (! _d->timeout_nowarning) {
            out(warning, msg_errno, "Connecting to client", errno,
              internalName().c_str());
          } else {
            out(info, msg_errno, "Connecting to client", errno,
              internalName().c_str());
          }
          return 0;
      }
    }
    first_mount_try = false;
    if (list_path.size() != 0) {
      list_path += "/";
    }
    list_path += Path::basename(_d->list_file);

    if (readConfig(list_path.c_str(), global_filters) != 0) {
      failed = true;
    } else if (_d->subset_client !=  _d->subset_server) {
      stringstream s;
      s << "Subsets don't match in server and client configuration files: '"
        << _d->subset_server << "' != '" << _d->subset_client << "', skipping";
      out(info, msg_standard, s.str().c_str());
      failed = true;
    }
  }
  if (! failed) {
    // Do not print this if in user-mode backup
    if (_d->home_path.length() == 0) {
      out(info, msg_standard, internalName().c_str(), -1, "Backing up client");
    }
    setInitialised();
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

          if (mountPath((*i)->path(), backup_path)) {
            if (! first_mount_try) {
              out(error, msg_errno, "Aborting client", errno,
                internalName().c_str());
            } else
            if (! _d->timeout_nowarning) {
              out(warning, msg_errno, "Connecting to client", errno,
                internalName().c_str());
            } else {
              out(info, msg_errno, "Connecting to client", errno,
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
  umount(); // does not change errno
  return failed ? -1 : 0;
}

void Client::show(int level) const {
  out(debug, msg_standard, _d->name.c_str(), level++, "Client");
  if (! _d->subset_client.empty()) {
    out(debug, msg_standard, _d->subset_client.c_str(), level, "Subset");
  }
  out(debug, msg_standard, _d->protocol.c_str(), level, "Protocol");
  if (_d->host_or_ip != _d->name) {
    out(debug, msg_standard, _d->host_or_ip.c_str(), level, "Hostname");
  }
  if (_d->options.size() > 0) {
    stringstream s;
    for (list<Option>::const_iterator i = _d->options.begin();
        i != _d->options.end(); i++ ) {
      s << i->option() << " ";
    }
    out(debug, msg_standard, s.str().c_str(), level, "Options");
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
  _d->filters.show(level);
  if (_d->paths.size() > 0) {
    out(debug, msg_standard, "", level++, "Paths");
    for (list<ClientPath*>::iterator i = _d->paths.begin();
        i != _d->paths.end(); i++) {
      (*i)->show(level);
    }
  }
}
