/*
     Copyright (C) 2007-2008  Herve Fache

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

#include "hbackup.h"
#include "files.h"
#include "report.h"
#include "configuration.h"
#include "conditions.h"
#include "filters.h"
#include "db.h"
#include "clients.h"

using namespace hbackup;

#define DEFAULT_DB_PATH "/backup"

static int        _aborted  = 0;
static progress_f _progress = NULL;

void hbackup::setMessageCallback(message_f message) {
  Report::self()->setMessageCallback(message);
}

void hbackup::setProgressCallback(progress_f progress) {
  _progress = progress;
}

void hbackup::setVerbosityLevel(VerbosityLevel level) {
  Report::self()->setVerbosityLevel(level);
}

void hbackup::abort(unsigned short test) {
  if (test == 0xffff) {
    // Reset
    _aborted = 0;
  } else {
    // Set
    _aborted = (test << 16) + 1;
  }
}

bool hbackup::aborting(unsigned short test) {
  unsigned short _test = _aborted >> 16;
  // Normal case
  if (_test == 0) {
    return _aborted != 0;
  }
  // Test case
  if (_test == test) {
    out(debug, msg_line_no, NULL, _test, "Killing trigger reached");
    _aborted = 1;
    return true;
  }
  return false;
}

struct HBackup::Private {
  Database*         db;
  string            mount_point;
  list<string>      selected_clients;
  list<Client*>     clients;
  Filters           filters;
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
    out(error, msg_standard, "Client already selected", -1, name);
    return -1;
  }
  _d->selected_clients.insert(client, name);
  return 0;
}

int HBackup::readConfig(const char* config_path) {
  /* Open configuration file */
  Stream config_file(config_path);

  if (config_file.open("r")) {
    out(error, msg_errno, "Opening file", errno, config_path);
    return -1;
  }
  // Set up config syntax and grammar
  Config config;

  // db
  config.add(new ConfigItem("db", 0, 1, 1));
  // filter
  {
    ConfigItem* filter = new ConfigItem("filter", 0, 0, 2);
    config.add(filter);
    // condition
    filter->add(new ConfigItem("condition", 1, 0, 2));
  }
  // client
  {
    ConfigItem* client = new ConfigItem("client", 1, 0, 1, 2);
    config.add(client);
    // hostname
    client->add(new ConfigItem("hostname", 0, 1, 1));
    // protocol
    client->add(new ConfigItem("protocol", 1, 1, 1));
    // option
    client->add(new ConfigItem("option", 0, 0, 1, 2));
    // config
    client->add(new ConfigItem("config", 1, 1, 1));
    // expire
    client->add(new ConfigItem("expire", 0, 1, 1));
    // filter
    {
      ConfigItem* filter = new ConfigItem("filter", 0, 0, 2);
      client->add(filter);
      // condition
      filter->add(new ConfigItem("condition", 1, 0, 2));
    }
  }

  /* Read configuration file */
  out(verbose, msg_standard, config_path, 1, "Reading configuration file");
  int rc = config.read(config_file, Stream::flags_accept_cr_lf);
  config_file.close();

  if (rc < 0) {
    return -1;
  }

  Client* client = NULL;
  Filter* filter = NULL;
  ConfigLine* params;
  while (config.line(&params) >= 0) {
    if ((*params)[0] == "db") {
      _d->db = new Database((*params)[1].c_str());
      _d->mount_point = (*params)[1] + "/.mount";
    } else
    if ((*params)[0] == "filter") {
      filter = _d->filters.add((*params)[1], (*params)[2]);
      if (filter == NULL) {
        out(error, msg_line_no, "Unsupported filter type", (*params).lineNo(),
          config_path);
        return -1;
      }
    } else
    if ((*params)[0] == "condition") {
      string  filter_type;
      bool    negated;
      if ((*params)[1][0] == '!') {
        filter_type = (*params)[1].substr(1);
        negated     = true;
      } else {
        filter_type = (*params)[1];
        negated     = false;
      }

      /* Add specified filter */
      if (filter_type == "filter") {
        Filter* subfilter = _d->filters.find((*params)[2]);
        if (subfilter == NULL) {
          out(error, msg_line_no, "Filter not found",
            (*params).lineNo(), config_path);
          return -1;
        }
        filter->add(new Condition(Condition::filter, subfilter, negated));
      } else {
        switch (filter->add(filter_type, (*params)[2], negated)) {
          case 1:
            out(error, msg_line_no, "Unsupported condition type",
              (*params).lineNo(), config_path);
            return -1;
            break;
          case 2:
            out(error, msg_line_no, "No filter defined",
              (*params).lineNo(), config_path);
            return -1;
        }
      }
    } else
    if ((*params)[0] == "client") {
      client = new Client((*params)[1],
        (params->size() > 2) ? (*params)[2] : "");

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
        out(error, msg_standard, "Client already selected", -1,
          client->internal_name().c_str());
        return -1;
      }
      _d->clients.insert(i, client);
    } else
    if (client != NULL) {
      if ((*params)[0] == "hostname") {
        client->setHostOrIp((*params)[1]);
      } else
      if ((*params)[0] == "protocol") {
        client->setProtocol((*params)[1]);
      } else
      if ((*params)[0] == "option") {
        if ((*params).size() == 2) {
          client->addOption((*params)[1]);
        } else {
          client->addOption((*params)[1], (*params)[2]);
        }
      } else
      if ((*params)[0] == "config") {
        client->setListfile((*params)[1].c_str());
      } else
      if ((*params)[0] == "expire") {
        unsigned int expire;
        if (sscanf((*params)[1].c_str(), "%u", &expire) != 1) {
          out(error, msg_line_no, "Expected decimal argument",
            (*params).lineNo(), config_path);
          return -1;
        }
        client->setExpire(expire * 3600 * 24);
      }
    }
  }

  // Use default DB path if none specified
  if (_d->db == NULL) {
    _d->db = new Database(DEFAULT_DB_PATH);
    _d->mount_point = DEFAULT_DB_PATH "/.mount";
  }
  out(verbose, msg_standard, "Configuration:");
  show(1);
  return 0;
}

int HBackup::open(
    const char*   path,
    bool          user_mode,
    bool          check_config) {
  bool failed = false;
  if (user_mode) {
    // Set-up client info
    Client* client = new Client("localhost");
    client->setProtocol("file");
    client->setListfile(Path(path, ".hbackup/config").c_str());
    client->setBasePath(path);
    if (check_config) {
      failed = (client->readConfig(client->listfile(), _d->filters) < 0);
    } else {
      _d->clients.push_back(client);
      // Set-up DB
      _d->db = new Database(Path(path, ".hbackup").c_str());
      _d->db->setProgressCallback(_progress);
    }
  } else {
    failed = (readConfig(path) < 0);
    if (! failed) {
      _d->db->setProgressCallback(_progress);
    }
  }
  return failed ? -1 : 0;
}

void HBackup::close() {
  delete _d->db;
  _d->db = NULL;
}

int HBackup::fix() {
  if (_d->db->open()) {
    return -1;
  }
  if (_d->db->close()) {
    return -1;
  }
  return 0;
}

int HBackup::scan(bool remove) {
  bool failed = true;
  if (! _d->db->open()) {
    // Corrupted files ge removed from DB
    if (! _d->db->scan(remove)) {
      failed = false;
    }
    _d->db->close();
  }
  return failed ? -1 : 0;
}

int HBackup::check() {
  bool failed = true;
  if (! _d->db->open(true)) {
    // Corrupted files ge removed from DB
    if ((_d->db->check(false) < 0)) {
      failed = false;
    }
    _d->db->close();
  }
  return failed ? -1 : 0;
}

int HBackup::backup(
  bool              initialize) {
  if (! _d->db->open(false, initialize)) {
    bool failed = false;

    for (list<Client*>::iterator client = _d->clients.begin();
        client != _d->clients.end(); client++) {
      if (aborting()) {
        break;
      }
      // Skip unrequested clients
      if (_d->selected_clients.size() != 0) {
        bool found = false;
        for (list<string>::iterator i = _d->selected_clients.begin();
          i != _d->selected_clients.end(); i++) {
          if (*i == (*client)->internal_name()) {
            found = true;
            break;
          }
        }
        if (! found) {
          continue;
        }
      }
      (*client)->setMountPoint(_d->mount_point);
      if ((*client)->backup(*_d->db, _d->filters)) {
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
  bool failed = false;
  if (_d->db->open(true)) {
    return -1;
  }
  if ((client == NULL) || (client[0] == '\0')) {
    failed = (_d->db->getClients(records) != 0);
  } else {
    if (_d->db->openClient(client)) {
      failed = true;
    } else {
      failed = (_d->db->getRecords(records, path, date) != 0);
    }
    _d->db->closeClient();
  }
  _d->db->close();
  return failed ? -1 : 0;
}

int HBackup::restore(
    const char*     dest,
    const char*     client,
    const char*     path,
    time_t          date) {
  bool failed = true;
  if (_d->db->open(true) >= 0) {
    if (_d->db->openClient(client) >= 0) {
      Path where(dest);
      where.noTrailingSlashes();

      bool failed = (_d->db->restore(where.c_str(), path, date) != 0);
      _d->db->closeClient();
      failed = false;;
    }
    _d->db->close();
  }
  return failed ? -1 : 0;
}

void HBackup::show(int level) const {
  if (_d->db == NULL) {
    out(verbose, msg_standard, "DB path not set", level);
  } else {
    out(verbose, msg_standard, _d->db->path(), level, "DB path");
  }
  if (! _d->selected_clients.empty()) {
    out(verbose, msg_standard, "Selected clients:", level);
    for (list<string>::iterator client = _d->selected_clients.begin();
        client != _d->selected_clients.end(); client++) {
      out(verbose, msg_standard, client->c_str(), level + 1);
    }
  }
  if (! _d->clients.empty()) {
    out(verbose, msg_standard, "Clients:", level);
    for (list<Client*>::iterator client = _d->clients.begin();
        client != _d->clients.end(); client++) {
      (*client)->show(level + 1);
      if (! (*client)->subset().empty()) {
        out(verbose, msg_standard, (*client)->subset().c_str(), level + 2,
          "Required subset");
      }
    }
  }
  _d->filters.show(level);
}
