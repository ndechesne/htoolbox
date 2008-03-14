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
#include "db.h"
#include "paths.h"
#include "clients.h"

using namespace hbackup;

struct Client::Private {
  list<ClientPath*> paths;
  string            subset_client;
};

int Client::mountPath(
    string        backup_path,
    string        *path) {
  string command = "mount ";
  string share;

  // Determine share and path
  if (_protocol == "file") {
    share = "";
    *path = backup_path;
  } else
  if (_protocol == "nfs") {
    share = _host_or_ip + ":" + backup_path;
    *path = _mount_point;
  } else
  if (_protocol == "smb") {
    share = "//" + _host_or_ip + "/" + backup_path.substr(0,1) + "$";
    *path = _mount_point + "/" +  backup_path.substr(3);
  } else {
    errno = EPROTONOSUPPORT;
    return -1;
  }

  // Check what is mounted
  if (_mounted != "") {
    if (_mounted != share) {
      // Different share mounted: unmount
      umount();
    } else {
      // Same share mounted: nothing to do
      return 0;
    }
  }

  // Build mount command
  if (_protocol == "file") {
    return 0;
  } else {
    // Check that mount dir exists, if not create it
    if (Directory(_mount_point.c_str()).create() < 0) {
      return -1;
    }

    // Set protocol and default options
    if (_protocol == "nfs") {
      command += "-t nfs -o ro,noatime,nolock,soft,timeo=30,intr";
    } else
    if (_protocol == "smb") {
      // codepage=858
      command += "-t cifs -o ro,noatime,nocase";
    }
  }
  // Additional options
  for (list<Option>::iterator i = _options.begin(); i != _options.end(); i++ ){
    command += "," + i->option();
  }
  // Paths
  command += " " + share + " " + _mount_point;

  // Issue mount command
  out(verbose, msg_standard, command.c_str(), 1);
  command += " > /dev/null 2>&1";

  int result = system(command.c_str());
  if (result != 0) {
    errno = ETIMEDOUT;
  } else {
    _mounted = share;
  }
  return result;
}

int Client::umount() {
  if (_mounted != "") {
    string command = "umount -fl ";

    command += _mount_point;
    out(verbose, msg_standard, command.c_str(), 1);
    _mounted = "";
    return system(command.c_str());
  }
  return 0;
}

int Client::readConfig(
    const string&   list_path,
    const Filters&  global_filters) {
  bool   failed  = false;

  // Open client configuration file
  Stream config_file(list_path.c_str());

  // Open client configuration file
  if (config_file.open("r")) {
    out(error, msg_standard, "Client configuration file not found", -1,
      list_path.c_str());
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

  out(verbose, msg_standard, "Reading client configuration file", 1);
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
          _expire = expire * 3600 * 24;
        } else {
          out(error, msg_line_no, "Wrong expiration value",
            (*params).lineNo(), list_path.c_str());
          failed = true;
        }
      } else
      if ((*params)[0] == "path") {
        filter = NULL;
        // New backup path
        if ((*params)[1][0] == '~') {
          path = new ClientPath((_home_path + &(*params)[1][1]).c_str());
        } else {
          path = new ClientPath((*params)[1].c_str());
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
            out(error, msg_line_no, "Path inside another", (*params).lineNo(),
              list_path.c_str());
            failed = true;
          }
          i = j;
          i--;
        }
        if (i != _d->paths.end()) {
          int ilen = strlen((*i)->path());
          if ((ilen < jlen)
            && (Path::compare((*i)->path(), (*j)->path(), ilen) == 0)) {
            out(error, msg_line_no, "Path inside another", (*params).lineNo(),
              list_path.c_str());
            failed = true;
          }
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
          out(error, msg_line_no, "Unsupported filter type",
            (*params).lineNo(), list_path.c_str());
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
            out(error, msg_line_no, "Filter not found", (*params).lineNo(),
              list_path.c_str());
            failed = 2;
          } else {
            filter->add(new Condition(Condition::filter, subfilter,
              negated));
          }
        } else {
          switch (filter->add(filter_type, (*params)[2], negated)) {
            case -2:
              out(error, msg_line_no, "Unsupported condition type",
                (*params).lineNo(), list_path.c_str());
              failed = true;
              break;
            case -1:
              out(error, msg_line_no, "Failed to add condition",
                (*params).lineNo(), list_path.c_str());
              failed = true;
          }
        }
      } else
      // Path attributes
      if ((*params)[0] == "ignore") {
        Filter* filter = path->findFilter((*params)[1], &_filters,
          &global_filters);
        if (filter == NULL) {
          out(error, msg_line_no, "Filter not found", (*params).lineNo(),
            list_path.c_str());
          failed = true;
        } else {
          path->setIgnore(filter);
        }
      } else
      if ((*params)[0] == "compress") {
        Filter* filter = path->findFilter((*params)[1], &_filters,
          &global_filters);
        if (filter == NULL) {
          out(error, msg_line_no, "Filter not found", (*params).lineNo(),
            list_path.c_str());
          failed = true;
        } else {
          path->setCompress(filter);
        }
      } else
      if ((*params)[0] == "parser") {
        switch (path->addParser((*params)[1], (*params)[2])) {
          case 1:
            out(error, msg_line_no, "Unsupported parser type",
              (*params).lineNo(), list_path.c_str());
            failed = true;
            break;
          case 2:
            out(error, msg_line_no, "Unsupported parser mode",
              (*params).lineNo(), list_path.c_str());
            failed = true;
            break;
        }
      }
    }
    // Close client configuration file
    config_file.close();
  }
  if (! failed) {
    out(verbose, msg_standard, "Configuration:");
    show(1);
  }
  return failed ? -1 : 0;
}

Client::Client(const string& name, const string& subset) :
    _name(name), _subset_server(subset), _host_or_ip(name), _list_file(NULL),
    _initialised(false), _expire(-1) {
  _d = new Private;
}

Client::~Client() {
  free(_list_file);
  for (list<ClientPath*>::iterator i = _d->paths.begin(); i != _d->paths.end();
      i++) {
    delete *i;
  }
  delete _d;
}

string Client::internal_name() const {
  if (_subset_server.empty()) {
    return _name;
  } else {
    return _name + "." + _subset_server;
  }
}

void Client::setHostOrIp(string value) {
  _host_or_ip = value;
}

void Client::setProtocol(string value) {
  _protocol = value;
}

void Client::setListfile(const char* value) {
  _list_file = Path::fromDos(value);
  _list_file = Path::noTrailingSlashes(_list_file);
}

int Client::backup(
    Database&       db,
    const Filters&  global_filters,
    bool            config_check) {
  bool    failed = false;
  string  share;
  string  list_path;
  char*   dir = Path::dirname(_list_file);

  if (_home_path.length() == 0) {
    stringstream s;
    s <<"Trying client '" << _name << "' using protocol '" << _protocol << "'";
    out(info, msg_standard, s.str().c_str());
  }

  if (mountPath(dir, &list_path)) {
    switch (errno) {
      case EPROTONOSUPPORT:
        out(error, msg_errno, _protocol.c_str(), errno);
        return 1;
      case ETIMEDOUT:
        out(error, msg_errno, "Connecting to client", errno);
        return 0;
    }
  }
  free(dir);
  if (list_path.size() != 0) {
    list_path += "/";
  }
  list_path += Path::basename(_list_file);

  if (readConfig(list_path, global_filters) != 0) {
    failed = true;
  } else if (_d->subset_client !=  _subset_server) {
    stringstream s;
    s << "Subsets don't match in server and client configuration files: '"
      << _subset_server << "' != '" << _d->subset_client << "', skipping";
    out(info, msg_standard, s.str().c_str());
  } else {
    out(info, msg_standard, "Backing up client");
    setInitialised();
    // Backup
    if (_d->paths.empty()) {
      failed = true;
    } else if (! config_check) {
      if (db.openClient(internal_name().c_str(), _expire) >= 0) {
        bool abort = false;
        for (list<ClientPath*>::iterator i = _d->paths.begin();
            i != _d->paths.end(); i++) {
          if (aborting()) {
            break;
          }
          string backup_path;
          out(info, msg_standard, (*i)->path(), -1, "Backup path");

          if (mountPath((*i)->path(), &backup_path)) {
            out(error, msg_errno, "Skipping client", errno);
            abort = true;
          } else
          if ((*i)->parse(db, backup_path.c_str())) {
            // prepare_share sets errno
            if (! aborting()) {
              out(error, msg_standard, "Aborting client");
            }
            abort  = true;
            failed = true;
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
  out(verbose, msg_standard, _name.c_str(), level, "Client");
  if (! _d->subset_client.empty()) {
    out(verbose, msg_standard, _d->subset_client.c_str(), level + 1, "Subset");
  }
  out(verbose, msg_standard, _protocol.c_str(), level + 1, "Protocol");
  if (_host_or_ip != _name) {
    out(verbose, msg_standard, _host_or_ip.c_str(), level + 1, "Hostname");
  }
  if (_options.size() > 0) {
    stringstream s;
    for (list<Option>::const_iterator i = _options.begin();
        i != _options.end(); i++ ) {
      s << i->option() << " ";
    }
    out(verbose, msg_standard, s.str().c_str(), level + 1, "Options");
  }
  out(verbose, msg_standard, _list_file, level + 1, "Config");
  {
    stringstream s;
    if (_expire >= 0) {
      s << _expire << "s (" << _expire / 86400 << "d)";
    } else {
      s << "none";
    }
    out(verbose, msg_standard, s.str().c_str(), level + 1, "Expiry");
  }
  _filters.show(level + 1);
  if (_d->paths.size() > 0) {
    out(verbose, msg_standard, "", level + 1, "Paths");
    for (list<ClientPath*>::iterator i = _d->paths.begin();
        i != _d->paths.end(); i++) {
      (*i)->show(level + 2);
    }
  }
}
