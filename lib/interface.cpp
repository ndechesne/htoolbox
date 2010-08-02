/*
     Copyright (C) 2007-2010  Herve Fache

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

#include <string>
#include <list>

#include <stdio.h>
#include <errno.h>

using namespace std;

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

#define DEFAULT_DB_PATH "/backup"

static int        _aborted  = 0;
static progress_f _copy_progress = NULL;
static progress_f _list_progress = NULL;

void hbackup::setCopyProgressCallback(progress_f progress) {
  _copy_progress = progress;
}

void hbackup::setListProgressCallback(progress_f progress) {
  _list_progress = progress;
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
  // Normal case
  if ((_aborted & 0xFFFF0000) == 0) {
    return _aborted != 0;
  }
  // Test case
  if ((_aborted >> 16) == test) {
    hlog_debug("Killing trigger reached: %u", test);
    _aborted = 1;
    return true;
  }
  return false;
}

struct HBackup::Private : ConfigObject {
  // db
  Database*               db;
  OpData::CompressionMode db_compress_mode;
  // clients
  std::list<string>       selected_clients;
  std::list<Client*>      clients;
  // attributes
  Attributes              attributes;
  // log
  string                  log_file_name;
  size_t                  log_max_lines;
  size_t                  log_backups;
  Level                   log_level;
  Private() : db(NULL), log_max_lines(0), log_backups(0), log_level(info) {
    log_file_name = "";
  }
  ~Private() {
    if (db != NULL) {
      delete db;
    }
    for (std::list<Client*>::iterator i = clients.begin(); i != clients.end();
        ++i) {
      delete *i;
    }
  }
  virtual ConfigObject* configChildFactory(
      const vector<string>& params,
      const char*           file_path = NULL,
      size_t                line_no   = 0);
};

ConfigObject* HBackup::Private::configChildFactory(
    const vector<string>& params,
    const char*           file_path,
    size_t                line_no) {
  ConfigObject* co = NULL;
  const string& keyword = params[0];
  if (keyword == "log") {
    log_file_name = params[1];
    co = this;
  } else
  if (keyword == "max_lines") {
    log_max_lines = atoi(params[1].c_str());
    co = this;
  } else
  if (keyword == "backups") {
    log_backups = atoi(params[1].c_str());
    co = this;
  } else
  if (keyword == "level") {
    co = this;
    if (Report::stringToLevel(params[1].c_str(), &log_level) != 0) {
      hlog_error("Wrong name for log level: '%s'", params[1].c_str());
      co = NULL;
    }
  } else
  if (keyword == "db") {
    if (db != NULL) {
      hlog_error("Keyword '%s' redefined in '%s'",
        keyword.c_str(), params[1].c_str());
    } else {
      db = new Database(params[1].c_str());
      co = this;
    }
  } else
  if (keyword == "compress") {
    if (params[1] == "always") {
      db_compress_mode = OpData::always;
      co = this;
    } else
    if (params[1] == "auto") {
      db_compress_mode = OpData::auto_now;
      co = this;
    } else
    if (params[1] == "later") {
      db_compress_mode = OpData::auto_later;
      co = this;
    } else
    if (params[1] == "never") {
      db_compress_mode = OpData::never;
      co = this;
    } else
    {
      hlog_error("Unsupported DB compression mode in '%s', at line %zu",
        file_path, line_no);
    }
  } else
  // Backwards compatibility
  if (keyword == "compress_auto") {
    db_compress_mode = OpData::auto_now;
    co = this;
  } else
  if (keyword == "client") {
    Client* client = new Client(params[1], attributes,
      (params.size() > 2) ? params[2] : "");
    // Clients MUST BE in alphabetic order
    int cmp = 1;
    std::list<Client*>::iterator i = clients.begin();
    while (i != clients.end()) {
      cmp = client->internalName().compare((*i)->internalName());
      if (cmp <= 0) {
        break;
      }
      i++;
    }
    if (cmp == 0) {
      hlog_error("Client already selected: '%s'",
        client->internalName().c_str());
      delete client;
    } else {
      clients.insert(i, client);
      co = client;
    }
  } else
  {
    co = attributes.configChildFactory(params, file_path, line_no);
  }
  return co;
}

HBackup::HBackup() : _d(new Private) {
  _d->selected_clients.clear();
  _d->clients.clear();
}

HBackup::~HBackup() {
  close();
  delete _d;
}

int HBackup::addClient(const char* name) {
  int cmp = 1;
  std::list<string>::iterator client = _d->selected_clients.begin();
  while ((client != _d->selected_clients.end())
      && ((cmp = client->compare(name)) < 0)) {
    client++;
  }
  if (cmp == 0) {
    hlog_error("Client already selected '%s'", name);
    return -1;
  }
  _d->selected_clients.insert(client, name);
  return 0;
}

int HBackup::readConfig(const char* config_path) {
  // Set up config syntax and grammar
  ConfigSyntax config_syntax;

  // log
  {
    ConfigItem* log = new ConfigItem("log", 0, 1, 1);
    config_syntax.add(log);
    // max lines per file
    log->add(new ConfigItem("max_lines", 0, 1, 1, 1));
    // max backups to keep
    log->add(new ConfigItem("backups", 0, 1, 1, 1));
    // log level
    log->add(new ConfigItem("level", 0, 1, 1, 1));
  }
  // db
  {
    ConfigItem* db = new ConfigItem("db", 0, 1, 1);
    config_syntax.add(db);
    // compress [always, auto_now, auto_later, never]
    db->add(new ConfigItem("compress", 0, 1, 1, 1));
    // old keyword for compress auto
    db->add(new ConfigItem("compress_auto", 0, 1));
  }
  // filter
  {
    ConfigItem* filter = new ConfigItem("filter", 0, 0, 2);
    config_syntax.add(filter);
    // condition
    filter->add(new ConfigItem("condition", 1, 0, 2));
  }
  // ignore
  config_syntax.add(new ConfigItem("ignore", 0, 1, 1));
  // report_copy_error_once
  config_syntax.add(new ConfigItem("report_copy_error_once", 0, 1));
  // client
  {
    ConfigItem* client = new ConfigItem("client", 1, 0, 1, 2);
    config_syntax.add(client);
    // hostname
    client->add(new ConfigItem("hostname", 0, 1, 1));
    // protocol
    client->add(new ConfigItem("protocol", 1, 1, 1));
    // option
    client->add(new ConfigItem("option", 0, 0, 1, 2));
    // timeout_nowarning
    client->add(new ConfigItem("timeout_nowarning", 0, 1));
    // report_copy_error_once
    client->add(new ConfigItem("report_copy_error_once", 0, 1));
    // config
    client->add(new ConfigItem("config", 0, 1, 1));
    // expire
    client->add(new ConfigItem("expire", 0, 1, 1));
    // users
    client->add(new ConfigItem("users", 0, 1, 1, -1));
    // ignore
    client->add(new ConfigItem("ignore", 0, 1, 1));
    // filter
    {
      ConfigItem* filter = new ConfigItem("filter", 0, 0, 2);
      client->add(filter);
      // condition
      filter->add(new ConfigItem("condition", 1, 0, 2));
    }
    // path
    {
      ConfigItem* path = new ConfigItem("path", 0, 0, 1);
      client->add(path);
      // parser
      path->add(new ConfigItem("parser", 0, 0, 2));
      // filter
      {
        ConfigItem* filter = new ConfigItem("filter", 0, 0, 2);
        path->add(filter);
        // condition
        filter->add(new ConfigItem("condition", 1, 0, 2));
      }
      // report_copy_error_once
      path->add(new ConfigItem("report_copy_error_once", 0, 1));
      // ignore
      path->add(new ConfigItem("ignore", 0, 1, 1));
      // compress
      path->add(new ConfigItem("compress", 0, 1, 1));
      // no_compress
      path->add(new ConfigItem("no_compress", 0, 1, 1));
    }
  }

  /* Read configuration file */
  hlog_debug_arrow(1, "Reading configuration file '%s'", config_path);
  Config config;
  ConfigErrors errors;
  ssize_t rc = config.read(config_path, 0, config_syntax, _d, &errors);
  if (rc < 0) {
    errors.show();
    return -1;
  }

  // Log to file if required
  if (_d->log_file_name != "") {
    report.startFileLog(_d->log_file_name.c_str(), _d->log_max_lines,
      _d->log_backups);
    report.setFileLogLevel(_d->log_level);
  }

  // Use default DB path if none specified
  if (_d->db == NULL) {
    _d->db = new Database(DEFAULT_DB_PATH);
  }
  return 0;
}

int HBackup::open(
    const char*   path,
    bool          user_mode,
    bool          check_config) {
  _d->db_compress_mode = OpData::auto_later;

  bool failed = false;
  if (user_mode) {
    // Give 'selected' client name
    addClient("localhost");
    // Set-up client info
    Client* client = new Client("localhost", _d->attributes);
    client->setProtocol("file");
    client->setListfile(Path(path, ".hbackup/config"));
    client->setBasePath(path);
    if (check_config) {
      failed = client->readConfig(client->listfile()) < 0;
    } else {
      _d->clients.push_back(client);
      // Set-up DB
      _d->db = new Database(Path(path, ".hbackup"));
      _d->db->setCopyProgressCallback(_copy_progress);
      _d->db->setListProgressCallback(_list_progress);
    }
  } else {
    failed = (readConfig(path) < 0);
    if (! failed) {
      _d->db->setCopyProgressCallback(_copy_progress);
      _d->db->setListProgressCallback(_list_progress);
    }
  }
  return failed ? -1 : 0;
}

void HBackup::close() {
  if (_d->db != NULL) {
    delete _d->db;
    _d->db = NULL;
  }
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
    // Set auto-compression mode
    _d->db->setCompressionMode(_d->db_compress_mode);

    Directory mount_dir(Path(_d->db->path(), ".mount"));
    if (mount_dir.create() < 0) {
      return -1;
    }
    bool failed = false;
    for (std::list<Client*>::iterator client = _d->clients.begin();
        client != _d->clients.end(); client++) {
      if (aborting()) {
        break;
      }
      // Skip unrequested clients
      if (_d->selected_clients.size() != 0) {
        bool found = false;
        for (std::list<string>::iterator i = _d->selected_clients.begin();
          i != _d->selected_clients.end(); i++) {
          if (*i == (*client)->internalName()) {
            found = true;
            break;
          }
        }
        if (! found) {
          continue;
        }
      }
      Directory mount_point(Path(mount_dir.path(),
        (*client)->internalName().c_str()));
      // Check that mount dir exists, if not create it
      if (mount_point.create() < 0) {
        return -1;
      }
      if ((*client)->backup(*_d->db, mount_point.path())) {
        failed = true;
      }
      mount_point.remove();
    }
    mount_dir.remove();
    _d->db->close();
    return failed ? -1 : 0;
  }
  return -1;
}

int HBackup::list_or_restore(
    const char*         dest,
    std::list<string>*  names,
    LinkType            links,
    const char*         path,
    time_t              date) {
  bool failed = false;
  if (_d->db->open(true) < 0) {
    return -1;
  }
  if ((dest == NULL) && (names == NULL)) {
    hlog_error("Neither destination nor list given");
    return -1;
  }
  std::list<string>& records = *names;
  if (_d->selected_clients.empty()) {
    if (dest == NULL) {
      failed = (_d->db->getClients(records) != 0);
    } else {
      hlog_error("No client given");
      failed = true;
    }
  } else {
    for (std::list<string>::iterator client = _d->selected_clients.begin();
        client != _d->selected_clients.end(); client++) {
      if (_d->db->openClient(client->c_str())) {
        failed = true;
      } else {
        if (dest == NULL) {
          if (_d->db->getRecords(*names, path, date) != 0) {
            failed = true;
          }
        } else {
          if (_d->db->restore(dest, links, path, date) != 0) {
            failed = true;
          }
        }
        _d->db->closeClient();
      }
    }
  }
  _d->db->close();
  return failed ? -1 : 0;
}

void HBackup::show(int level) const {
  if (_d->log_file_name == "") {
    hlog_debug_arrow(level, "No file logging");
  } else {
    hlog_debug_arrow(level, "Log path: '%s'", _d->log_file_name.c_str());
    if (_d->log_max_lines == 0) {
      hlog_debug_arrow(level + 1, "No size limit");
    } else {
      hlog_debug_arrow(level + 1, "Max lines per log: %zu", _d->log_max_lines);
    }
    if (_d->log_backups == 0) {
      hlog_debug_arrow(level + 1, "No log file backup");
    } else {
      hlog_debug_arrow(level + 1, "Backup(s): %zu", _d->log_backups);
    }
    hlog_debug_arrow(level + 1, "Level: %s", Report::levelString(_d->log_level));
  }
  if (_d->db == NULL) {
    hlog_debug_arrow(level, "DB path not set");
  } else {
    hlog_debug_arrow(level, "DB path: '%s'", _d->db->path());
  }
  const char* comp_str;
  switch (_d->db_compress_mode) {
    case OpData::always:
      comp_str = "always";
      break;
    case OpData::auto_now:
      comp_str = "automatic";
      break;
    case OpData::auto_later:
      comp_str = "later";
      break;
    case OpData::never:
      comp_str = "never";
      break;
  }
  hlog_debug_arrow(level, "DB compression: %s", comp_str);
  if (! _d->selected_clients.empty()) {
    hlog_debug_arrow(level, "Selected clients:");
    for (std::list<string>::iterator client = _d->selected_clients.begin();
        client != _d->selected_clients.end(); client++) {
      hlog_debug_arrow(level + 1, "%s", client->c_str());
    }
  }
  if (! _d->clients.empty()) {
    hlog_debug_arrow(level, "Clients:");
    for (std::list<Client*>::iterator client = _d->clients.begin();
        client != _d->clients.end(); client++) {
      (*client)->show(level + 1);
    }
  }
  _d->attributes.show(level);
}
