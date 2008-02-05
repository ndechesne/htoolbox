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

#include <iostream>
#include <list>
#include <string>
#include <errno.h>

using namespace std;

#include "files.h"
#include "configuration.h"
#include "conditions.h"
#include "filters.h"
#include "parsers.h"
#include "db.h"
#include "paths.h"
#include "clients.h"
#include "hbackup.h"

using namespace hbackup;

struct Client::Private {
  list<ClientPath*> paths;
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
    return 2;
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
    if (Directory(_mount_point.c_str()).create()) {
      return 2;
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
  if (verbosity() > 1) {
    cout << " -> " << command << endl;
  }
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
    if (verbosity() > 1) {
      cout << " -> " << command << endl;
    }
    _mounted = "";
    return system(command.c_str());
  }
  return 0;
}

int Client::readListFile(
    const string&   list_path,
    const Filters&  global_filters) {
  bool   failed  = false;

  // Open client configuration file
  Stream config_file(list_path.c_str());

  // Open client configuration file
  if (config_file.open("r")) {
    cerr << "Client configuration file not found " << list_path << endl;
    return -1;
  }
  // Set up config syntax and grammar
  Config config;

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

  if (verbosity() > 1) {
    cout << " -> Reading client configuration file" << endl;
  }
  if (config.read(config_file,
      Stream::flags_dos_catch | Stream::flags_accept_cr_lf) >= 0) {
    // Read client configuration file
    ClientPath* path   = NULL;
    Filter*     filter = NULL;
    ConfigLine* params;
    while (config.line(&params) >= 0) {
      if ((*params)[0] == "expire") {
        int expire;
        if ((sscanf((*params)[1].c_str(), "%d", &expire) != 0) && (expire >= 0)) {
          _expire = expire * 3600 * 24;
          if (verbosity() > 1) {
            cout << " --> expiry " << expire << " day(s)" << endl;
          }
        } else {
          cerr << "Error: in client configuration file " << list_path
            << ", line " << (*params).lineNo() << " wrong expiration value: "
            << (*params)[1] << endl;
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
        if (verbosity() > 1) {
          cout << " --> Path: " << path->path() << endl;
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
            errno = EUCLEAN;
            cout << "Path inside another: this is not supported" << endl;
            failed = true;
          }
          i = j;
          i--;
        }
        if (i != _d->paths.end()) {
          int ilen = strlen((*i)->path());
          if ((ilen < jlen)
            && (Path::compare((*i)->path(), (*j)->path(), ilen) == 0)) {
            errno = EUCLEAN;
            cout << "Path inside another. This is not supported" << endl;
            failed = true;
          }
        }
      } else
      if ((*params)[0] == "filter") {
        if (path == NULL) {
          // Client-wide filter
          filter = addFilter((*params)[1], (*params)[2]);
          if (verbosity() > 1) {
            cout << " --> client-wide filter " << (*params)[1] << " " << (*params)[2]
              << endl;
          }
        } else {
          // Path-wide filter
          filter = path->addFilter((*params)[1], (*params)[2]);
          if (verbosity() > 1) {
            cout << " --> path-wide filter " << (*params)[1] << " " << (*params)[2]
              << endl;
          }
        }
        if (filter == NULL) {
          cerr << "Error: in client configuration file " << list_path
            << ", line " << (*params).lineNo() << " unsupported filter type: "
            << (*params)[1] << endl;
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
            cerr << "Error: in client configuration file " << list_path
              << ", line " << (*params).lineNo() << " filter not found: "
              << (*params)[2] << endl;
            failed = 2;
          } else {
            if (verbosity() > 1) {
              cout << " ---> condition ";
              if (negated) {
                cout << "not ";
              }
              cout << filter_type << " " << subfilter->name() << endl;
            }
            filter->add(new Condition(Condition::subfilter, subfilter,
              negated));
          }
        } else {
          switch (filter->add(filter_type, (*params)[2], negated)) {
            case 1:
              cerr << "Error: in client configuration file " << list_path
                << ", line " << (*params).lineNo()
                << " unsupported condition type: " << (*params)[1] << endl;
              failed = true;
              break;
            case 2:
              cerr << "Error: in client configuration file " << list_path
                << ", line " << (*params).lineNo() << " no filter defined" << endl;
              failed = true;
              break;
            default:
              if (verbosity() > 1) {
                cout << " ---> condition ";
                if (negated) {
                  cout << "not ";
                }
                cout << filter_type << " " << (*params)[2] << endl;
              }
          }
        }
      } else
      // Path attributes
      if ((*params)[0] == "ignore") {
        Filter* filter = path->findFilter((*params)[1], &_filters,
          &global_filters);
        if (filter == NULL) {
          cerr << "Error: in client configuration file " << list_path
            << ", line " << (*params).lineNo()
            << ": filter for ignoring not found: " << (*params)[1] << endl;
          failed = true;
        } else {
          path->setIgnore(filter);
        }
      } else
      if ((*params)[0] == "compress") {
        Filter* filter = path->findFilter((*params)[1], &_filters,
          &global_filters);
        if (filter == NULL) {
          cerr << "Error: in client configuration file " << list_path
            << ", line " << (*params).lineNo()
            << ": filter for compression not found: " << (*params)[1] << endl;
          failed = true;
        } else {
          path->setCompress(filter);
        }
      } else
      if ((*params)[0] == "parser") {
        switch (path->addParser((*params)[1], (*params)[2])) {
          case 1:
            cerr << "Error: in client configuration file " << list_path
              << ", line " << (*params).lineNo() << " unsupported parser type: "
              << (*params)[1] << endl;
            failed = true;
            break;
          case 2:
            cerr << "Error: in client configuration file " << list_path
              << ", line " << (*params).lineNo() << " unsupported parser mode: "
              << (*params)[2] << endl;
            failed = true;
            break;
        }
      }
    }
    // Close client configuration file
    config_file.close();
  }
  return failed ? -1 : 0;
}

Client::Client(string value) {
  _d            = new Private;
  _name         = value;
  _host_or_ip   = _name;
  _list_file    = NULL;
  _protocol     = "";
  _home_path    = "";
  _mount_point  = "";
  _mounted      = "";
  _initialised  = false;
  _expire       = -1;

  if (verbosity() > 1) {
    cout << " --> Client: " << _name << endl;
  }
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
  // Add suffix so that dual booted clients can use the same name twice
  char suffix;
  if (_protocol == "smb") {
    suffix = '$';
  } else {
    suffix = '#';
  }
  return _name + suffix;
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

  if (mountPath(dir, &list_path)) {
    switch (errno) {
      case EPROTONOSUPPORT:
        cerr << "Protocol not supported: " << _protocol << endl;
        return 1;
      case ETIMEDOUT:
        if (verbosity() > 0) {
          cerr << "Warning: cannot connect to client: " << _name
            << " (using the " << _protocol << " protocol). "
            <<  strerror(errno) << endl;
        }
        return 0;
    }
  }
  free(dir);
  if (list_path.size() != 0) {
    list_path += "/";
  }
  list_path += Path::basename(_list_file);

  if ((_home_path.length() == 0) && (verbosity() > 0)) {
    cout << "Backup client '" << _name
      << "' using protocol '" << _protocol << "'" << endl;
  }

  if (! readListFile(list_path, global_filters)) {
    setInitialised();
    // Backup
    if (_d->paths.empty()) {
      failed = true;
    } else if (! config_check) {
      db.setClient(internal_name().c_str(), _expire);
      for (list<ClientPath*>::iterator i = _d->paths.begin();
          i != _d->paths.end(); i++) {
        if (terminating()) {
          break;
        }
        string backup_path;

        if (verbosity() > 0) {
          cout << "Backup path '" << (*i)->path() << "'" << endl;
        }

        bool abort = false;
        if (mountPath((*i)->path(), &backup_path)) {
          cerr << "Warning: mount failed, skipping client backup" << endl;
          abort = true;
        } else
        if ((*i)->parse(db, backup_path.c_str())) {
          // prepare_share sets errno
          if (! terminating()) {
            cerr << "Error: backup failed, aborting client backup" << endl;
          }
          abort  = true;
          failed = true;
        }
        if (abort) {
          db.failedClient();
          break;
        }
      }
    }
  } else {
    failed = true;
  }
  umount(); // does not change errno
  return failed ? -1 : 0;
}

void Client::show() {
  cout << "Client: " << _name << endl;
  cout << "-> " << _protocol << "://" << _host_or_ip << " " << _list_file
    << endl;
  if (_options.size() > 0) {
    cout << "Options:";
    for (list<Option>::iterator i = _options.begin(); i != _options.end();
     i++ ) {
      cout << " " + i->option();
    }
    cout << endl;
  }
  if (_d->paths.size() > 0) {
    cout << "Paths:" << endl;
    for (list<ClientPath*>::iterator i = _d->paths.begin();
        i != _d->paths.end(); i++) {
      cout << " -> " << (*i)->path() << endl;
    }
  }
}
