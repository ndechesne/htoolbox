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

#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <errno.h>

using namespace std;

#include "strings.h"
#include "files.h"
#include "filters.h"
#include "parsers.h"
#include "cvs_parser.h"
#include "db.h"
#include "paths.h"
#include "clients.h"
#include "hbackup.h"

using namespace hbackup;

struct Client::Private {
  list<Path*>  paths;
};

int Client::mountPath(
    string        backup_path,
    string        *path) {
  string command = "mount ";
  string share;

  /* Determine share and path */
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

  /* Check what is mounted */
  if (_mounted != "") {
    if (_mounted != share) {
      /* Different share mounted: unmount */
      umount();
    } else {
      /* Same share mounted: nothing to do */
      return 0;
    }
  }

  /* Check that mount dir exists, if not create it */
  if (Directory("").create(_mount_point.c_str())) {
    return 2;
  }

  /* Build mount command */
  if (_protocol == "file") {
    return 0;
  } else
  if (_protocol == "nfs") {
    command += "-t nfs -o ro,noatime,nolock";
  } else
  if (_protocol == "smb") {
    // codepage=858
    command += "-t cifs -o ro,noatime,nocase";
  }
  // Additional options
  for (list<Option>::iterator i = _options.begin(); i != _options.end(); i++ ){
    command += "," + i->option();
  }
  // Paths
  command += " " + share + " " + _mount_point;

  /* Issue mount command */
  if (verbosity() > 0) {
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
    string command = "umount ";

    command += _mount_point;
    if (verbosity() > 0) {
      cout << " -> " << command << endl;
    }
    _mounted = "";
    return system(command.c_str());
  }
  return 0;
}

int Client::readListFile(const string& list_path) {
  string  buffer;
  int     line    = 0;
  int     failed  = 0;

  /* Open list file */
  ifstream list_file(list_path.c_str());

  /* Open list file */
  if (! list_file.is_open()) {
//     cerr << "List file not found " << list_path << endl;
    failed = 2;
  } else {
    if (verbosity() > 1) {
      cout << " -> Reading backup list file" << endl;
    }

    /* Read list file */
    Path *path = NULL;
    while (! list_file.eof() && ! failed) {
      getline(list_file, buffer);
      unsigned int pos = buffer.find("\r");
      if (pos != string::npos) {
        buffer.erase(pos);
      }
      list<string> params;

      line++;
      if (Stream::decodeLine(buffer, params)) {
        errno = EUCLEAN;
        cerr << "Warning: in list file " << list_path << ", line " << line
          << " missing single- or double-quote," << endl;
        cerr << "make sure double-quoted Windows paths do not end in '\\'."
          << endl;
      }
      if (params.size() > 0) {
        list<string>::iterator current = params.begin();
        string                 keyword = *current++;
        if (keyword == "path") {
          // Expect exactly two parameters
          if (params.size() != 2) {
            cerr << "Error: in list file " << list_path << ", line " << line
              << " 'path' takes exactly one argument" << endl;
            failed = 1;
          } else {
            /* New backup path */
            path = new Path(current->c_str());
            if (verbosity() > 1) {
              cout << " --> Path: " << path->path() << endl;
            }
            list<Path*>::iterator i = _d->paths.begin();
            while ((i != _d->paths.end())
                && (pathCompare((*i)->path(), path->path()) < 0)) {
              i++;
            }
            list<Path*>::iterator j = _d->paths.insert(i, path);
            // Check that we don't have a path inside another, such as
            // '/home' and '/home/user'
            int jlen = strlen((*j)->path());
            if (i != _d->paths.end()) {
              int ilen = strlen((*i)->path());
              if ((ilen > jlen)
               && (pathCompare((*i)->path(), (*j)->path(), jlen) == 0)) {
                cout << "Path inside another: this is not supported"
                  << endl;
                failed = 1;
              }
              i = j;
              i--;
            }
            if (i != _d->paths.end()) {
              int ilen = strlen((*i)->path());
              if ((ilen < jlen)
               && (pathCompare((*i)->path(), (*j)->path(), ilen) == 0)) {
                cout << "Path inside another. This is not supported yet"
                  << endl;
                failed = 1;
              }
            }
          }
        } else if (path != NULL) {
          string type;
          int rc;

          if (params.size() > 1) {
            type = *current++;
          }
          // Path attributes
          if ((keyword == "ignore") || (keyword == "ignand")) {
            // Expect exactly three parameters
            if (params.size() != 3) {
              cerr << "Error: in list file " << list_path << ", line " << line
                << " 'filter' takes exactly two arguments" << endl;
              failed = 1;
            } else
            if ((rc = path->addFilter(type, *current, keyword
             == "ignand"))) {
              switch (rc) {
                case 1:
                  cerr << "Error: in list file " << list_path << ", line "
                   << line << " unsupported filter type: " << type << endl;
                  failed = 1;
                  break;
                case 2:
                  cerr << "Error: in list file " << list_path << ", line "
                   << line << " unsupported value: '" << *current
                   << "' for filter type " << type << endl;
                  failed = 1;
                  break;
                case 3:
                  cerr << "Error: in list file " << list_path << ", line "
                   << line << " cannot append to nothing" << endl;
                  failed = 1;
                  break;
              }
            }
          } else
          if (keyword == "parser") {
            // Expect exactly three parameters
            if (params.size() != 3) {
              cerr << "Error: in list file " << list_path << ", line " << line
                << " 'parser' takes exactly two arguments" << endl;
              failed = 1;
            } else
            if ((rc = path->addParser(type, *current))) {
              switch (rc) {
                case 1:
                  cerr << "Error: in list file " << list_path << ", line "
                   << line << " unsupported parser type: " << type << endl;
                  failed = 1;
                  break;
                case 2:
                  cerr << "Error: in list file " << list_path << ", line "
                   << line << " unsupported parser mode: " << *current << endl;
                  failed = 1;
                  break;
              }
            }
          } else
          if (keyword == "expire") {
            int time_out;
            if ((sscanf(type.c_str(), "%d", &time_out) != 0)
             && (time_out != 0)) {
              path->setExpiration(time_out * 3600 * 24);
            }
          } else {
            // What was that?
            cerr << "Error: in list file " << list_path << ", line " << line
              << " unknown keyword: " << keyword << endl;
            failed = 1;
          }
        } else {
          // What?
          cerr << "Error: in list file " << list_path << ", line " << line
            << " unexpected keyword at this level: " << keyword << endl;
          failed = 1;
        }
      }
    }
    /* Close list file */
    list_file.close();
  }
  return failed;
}

Client::Client(string value) {
  _d            = new Private;
  _name         = value;
  _host_or_ip   = _name;
  _protocol     = "";
  _mount_point  = "";
  _mounted      = "";
  _initialised  = false;

  if (verbosity() > 1) {
    cout << " --> Client: " << _name << endl;
  }
}

Client::~Client() {
  for (list<Path*>::iterator i = _d->paths.begin(); i != _d->paths.end();
      i++) {
    delete *i;
  }
  delete _d;
}

void Client::setHostOrIp(string value) {
  _host_or_ip = value;
}

void Client::setProtocol(string value) {
  _protocol = value;
}

void Client::setListfile(const char* value) {
  _listfile = value;
  _listfile.toUnix();
}

int Client::backup(
    Database& db,
    bool      config_check) {
  int     failed = 0;
  int     clientfailed  = 0;
  string  share;
  string  list_path;

  if (mountPath(_listfile.dirname().c_str(), &list_path)) {
    switch (errno) {
      case EPROTONOSUPPORT:
        cerr << "Protocol not supported: " << _protocol << endl;
        return 1;
      case ETIMEDOUT:
        if (verbosity() > 0) {
          cout << "Client unreachable '" << _name
            << "' using protocol '" << _protocol << "' ("
            <<  strerror(errno) << ")" << endl;
        }
        return 0;
    }
  }
  if (list_path.size() != 0) {
    list_path += "/";
  }
  list_path += _listfile.basename().c_str();

  if (verbosity() > 0) {
    cout << "Backup client '" << _name
      << "' using protocol '" << _protocol << "'" << endl;
  }

  if (! readListFile(list_path)) {
    setInitialised();
    /* Backup */
    if (_d->paths.empty()) {
      failed = 1;
    } else if (! config_check) {
      db.setPrefix(prefix().c_str());
      for (list<Path*>::iterator i = _d->paths.begin(); i != _d->paths.end(); i++) {
        if (terminating() || clientfailed) {
          break;
        }
        string  backup_path;

        if (verbosity() > 0) {
          cout << "Backup path '" << (*i)->path() << "'" << endl;
        }

        if (mountPath((*i)->path(), &backup_path)) {
          cerr << "clients: backup: mount failed for " << (*i)->path() << endl;
          failed = 1;
        } else
        if ((*i)->parse(db, backup_path.c_str())) {
          // prepare_share sets errno
          if (! terminating()) {
            cerr << "clients: backup: list creation failed" << endl;
          }
          failed        = 1;
          clientfailed  = 1;
        }
      }
    }
  } else {
    // errno set by functions called
    switch (errno) {
      case ENOENT:
        cerr << "List file not found " << list_path << endl;
        break;
      case EUCLEAN:
        cerr << "List file corrupted " << list_path << endl;
    }
    failed = 1;
  }
  umount(); // does not change errno
  return failed;
}

void Client::show() {
  cout << "Client: " << _name << endl;
  cout << "-> " << _protocol << "://" << _host_or_ip << " "
    << _listfile.c_str() << endl;
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
    for (list<Path*>::iterator i = _d->paths.begin(); i != _d->paths.end(); i++) {
      cout << " -> " << (*i)->path() << endl;
    }
  }
}
