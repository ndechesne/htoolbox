/*
     Copyright (C) 2007  Herve Fache

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
#include <string>
#include <list>
#include <errno.h>

using namespace std;

#include "files.h"
#include "conditions.h"
#include "filters.h"
#include "db.h"
#include "clients.h"
#include "hbackup.h"

using namespace hbackup;

#define DEFAULT_DB_PATH "/backup"

struct HBackup::Private {
  Database*         db;
  string            mount_point;
  list<string>      selected_clients;
  list<Client*>     clients;
  Filters           filters;
  Filter* addFilter(const string& type, const string& name) {
    return filters.add(type, name);
  }
  Filter* findFilter(const string& name) const {
    return filters.find(name);
  }
};

HBackup::HBackup() {
  _d               = new Private;
  _d->db           = NULL;
  _d->selected_clients.clear();
  _d->clients.clear();
}

HBackup::~HBackup() {
  for (list<Client*>::iterator client = _d->clients.begin();
      client != _d->clients.end(); client++){
    delete *client;
  }
  close();
  delete _d;
}

int HBackup::addClient(const char* name) {
  int cmp = 1;
  list<string>::iterator client = _d->selected_clients.begin();
  while ((client != _d->selected_clients.end())
      && ((cmp = client->compare(name)) < 0)) {
    client++;
  }
  if (cmp == 0) {
    cerr << "Error: client already selected: " << name << endl;
    return -1;
  }
  _d->selected_clients.insert(client, name);
  return 0;
}

int HBackup::setUserPath(const char* home_path) {
  string path = home_path;

  // Set-up DB
  _d->db = new Database(path + "/.hbackup");

  // Set-up client info
  Client* client = new Client("localhost");
  client->setProtocol("file");
  client->setListfile((path + "/.hbackup/config").c_str());
  client->setBasePath(home_path);
  _d->clients.push_back(client);

  return 0;
}

int HBackup::readConfig(const char* config_path) {
  /* Open configuration file */
  Stream config_file(config_path);

  if (config_file.open("r")) {
    cerr << strerror(errno) << ": " << config_path << endl;
    return -1;
  } else {
    /* Read configuration file */
    string  buffer;
    int     line    = 0;

    if (verbosity() > 1) {
      cout << " -> Reading configuration file" << endl;
    }

    Client* client = NULL;
    Filter* filter = NULL;
    while (true) {
      vector<string> params;

      line++;
      int rc = config_file.getParams(params);
      if (rc == -2) {
        errno = EUCLEAN;
        cerr << "Warning: in file " << config_path << ", line " << line
          << " missing single- or double-quote" << endl;
      } else if (rc <= 0) {
        break;
      }
      if (params.size() == 1) {
        errno = EUCLEAN;
        cerr << "Error: in file " << config_path << ", line " << line
          << " unexpected lonely keyword: " << *params.begin() << endl;
        return -1;
      } else if (params.size() > 1) {
        if (params[0] == "db") {
          if (params.size() > 2) {
            cerr << "Error: in file " << config_path << ", line " << line
              << " '" << params[0] << "' takes exactly one argument" << endl;
            return -1;
          } else
          if (_d->db != NULL) {
            cerr << "Error: in file " << config_path << ", line " << line
              << " '" << params[0] << "' seen twice" << endl;
            return -1;
          } else {
            _d->db = new Database(params[1]);
            _d->mount_point = params[1] + "/mount";
          }
        } else
        if (params[0] == "trash") {
        } else
        if (params[0] == "filter") {
          // Expect exactly three parameters
          if (params.size() != 3) {
            cerr << "Error: in file " << config_path << ", line " << line
              << " '" << params[0] << "' takes exactly two arguments" << endl;
            return -1;
          }
          filter = _d->addFilter(params[1], params[2]);
          if (verbosity() > 1) {
            cout << " --> global filter " << params[1] << " " << params[2]
              << endl;
          }
          if (filter == NULL) {
            cerr << "Error: in file " << config_path << ", line " << line
              << " unsupported filter type: " << params[1] << endl;
            return -1;
          }
        } else
        if (params[0] == "condition") {
          if (params.size() != 3) {
            cerr << "Error: in file " << config_path << ", line " << line
              << " '" << params[0] << "' takes exactly two arguments" << endl;
            return -1;
          }
          string  filter_type;
          bool    negated;
          if (params[1][0] == '!') {
            filter_type = params[1].substr(1);
            negated     = true;
          } else {
            filter_type = params[1];
            negated     = false;
          }

          /* Add specified filter */
          if (filter_type == "filter") {
            Filter* subfilter = _d->findFilter(params[2]);
            if (subfilter == NULL) {
              cerr << "Error: in file " << config_path << ", line " << line
                << " filter not found: " << params[2] << endl;
              return -1;
            }
            if (verbosity() > 1) {
              cout << " ---> condition ";
              if (negated) {
                cout << "not ";
              }
              cout << filter_type << " " << subfilter->name() << endl;
            }
            filter->add(new Condition(Condition::subfilter, subfilter,
              negated));
          } else {
            switch (filter->add(filter_type, params[2], negated)) {
              case 1:
                cerr << "Error: in file " << config_path << ", line " << line
                  << " unsupported condition type: " << params[1] << endl;
                return -1;
                break;
              case 2:
                cerr << "Error: in file " << config_path << ", line " << line
                  << " no filter defined" << endl;
                return -1;
                break;
              default:
                if (verbosity() > 1) {
                  cout << " ---> condition ";
                  if (negated) {
                    cout << "not ";
                  }
                  cout << filter_type << " " << params[2] << endl;
                }
            }
          }
        } else
        if (params[0] == "client") {
          if (params.size() != 3) {
            cerr << "Error: in file " << config_path << ", line " << line
              << " '" << params[0] << "' takes exactly two arguments" << endl;
            return -1;
          }
          client = new Client(params[2]);
          client->setProtocol(params[1]);

          // Clients MUST BE in alphabetic order
          int cmp = 1;
          list<Client*>::iterator i = _d->clients.begin();
          while (i != _d->clients.end()) {
            cmp = client->internal_name().compare((*i)->internal_name());
            if (cmp <= 0) {
              break;
            }
            i++;
          }
          if (cmp == 0) {
            cerr << "Error: client already selected: " << params[2] << endl;
            return -1;
          }
          _d->clients.insert(i, client);
        } else if (client != NULL) {
          if (params[0] == "hostname") {
            if (params.size() > 2) {
              cerr << "Error: in file " << config_path << ", line " << line
                << " '" << params[0] << "' takes exactly one argument" << endl;
              return -1;
            }
            client->setHostOrIp(params[1]);
          } else
          if (params[0] == "option") {
            if (params.size() > 3) {
              cerr << "Error: in file " << config_path << ", line " << line
                << " '" << params[0] << "' takes one or two arguments" << endl;
              return -1;
            }
            if (params.size() == 2) {
              client->addOption(params[1]);
            } else {
              client->addOption(params[1], params[2]);
            }
          } else
          if (params[0] == "config") {
            if (params.size() > 2) {
              cerr << "Error: in file " << config_path << ", line " << line
                << " '" << params[0] << "' takes exactly one argument" << endl;
              return -1;
            }
            client->setListfile(params[1].c_str());
            if (verbosity() > 1) {
              cout << " ---> config file: " << params[1] << endl;
            }
          } else
          if (params[0] == "expire") {
            if (params.size() > 2) {
              cerr << "Error: in file " << config_path << ", line " << line
                << " '" << params[0] << "' takes exactly one argument" << endl;
              return -1;
            }
            errno = 0;
            int expire = strtol(params[1].c_str(), NULL, 10);
            if (errno != 0) {
              cerr << "Error: in file " << config_path << ", line " << line
                << " '" << params[0] << "' expects a number as argument" << endl;
              return -1;
            }
            if (verbosity() > 1) {
              cout << " ---> expiry: " << expire << " day(s)" << endl;
            }
            client->setExpire(expire * 3600 * 24);
          } else {
            cerr << "Unrecognised keyword '" << params[0]
              << "' in configuration file, line " << line << endl;
            return -1;
          }
        } else {
          cerr << "Error: in file " << config_path << ", line " << line
            << " keyword before client" << endl;
          return -1;
        }
      }
    }
    config_file.close();
  }

  // Use default DB path if none specified
  if (_d->db == NULL) {
    _d->db = new Database(DEFAULT_DB_PATH);
    _d->mount_point = DEFAULT_DB_PATH "/mount";
  }
  return 0;
}

void HBackup::close() {
  delete _d->db;
  _d->db = NULL;
}

int HBackup::check(bool thorough) {
  if (! _d->db->open_rw()) {
    bool failed = false;

    if (_d->db->scan(thorough)) {
      failed = true;
    }
    _d->db->close();
    if (! failed) {
      return 0;
    }
  }
  return -1;
}

int HBackup::backup(bool config_check) {
  if (! _d->db->open_rw()) {
    bool failed = false;

    for (list<Client*>::iterator client = _d->clients.begin();
        client != _d->clients.end(); client++) {
      if (terminating()) {
        break;
      }
      // Skip unrequested clients
      if (_d->selected_clients.size() != 0) {
        bool found = false;
        for (list<string>::iterator i = _d->selected_clients.begin();
          i != _d->selected_clients.end(); i++) {
          if (*i == (*client)->name().c_str()) {
            found = true;
            break;
          }
        }
        if (! found) {
          continue;
        }
      }
      (*client)->setMountPoint(_d->mount_point);
      if ((*client)->backup(*_d->db, _d->filters, config_check)) {
        failed = true;
      }
    }
    _d->db->close();
    if (failed) {
      return -1;
    }
  }
  return -1;
}

int HBackup::getList(
    list<string>&   records,
    const char*     client,
    const char*     path,
    time_t          date) {
  bool failed;

  if (_d->db->open_ro()) {
    return -1;
  }
  if ((client == NULL) || (client[0] == '\0')) {
    failed = _d->db->getRecords(records, client, path, date) != 0;
    if (! failed) {
      // We got internal client names, get rid of extra character
      list<string>::iterator i;
      for (i = records.begin(); i != records.end(); i++) {
        *i = i->substr(0, i->size() - 1);
      }
      records.unique();
    }
  } else {
    bool failed1, failed2;
    string internal;
    // Get records for client#
    internal = client;
    internal += "#";
    failed1 = _d->db->getRecords(records, internal.c_str(), path, date) != 0;
    // Get records for client$
    internal = client;
    internal += "$";
    failed2 = _d->db->getRecords(records, internal.c_str(), path, date) != 0;
    failed = failed1 && failed2;
  }
  _d->db->close();
  return failed ? -1 : 0;
}

int HBackup::restore(
    const char*     dest,
    const char*     client,
    const char*     path,
    time_t          date) {
  if (_d->db->open_ro()) {
    return -1;
  }
  Path where(dest);
  where.noTrailingSlashes();

  bool failed1, failed2;
  string internal;
  // Get records for client#
  internal = client;
  internal += "#";
  failed1 = _d->db->restore(where.c_str(), internal.c_str(), path, date) != 0;
  // Get records for client$
  internal = client;
  internal += "$";
  failed2 = _d->db->restore(where.c_str(), internal.c_str(), path, date) != 0;
  _d->db->close();
  return (failed1 && failed2) ? -1 : 0;
}
