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
#include "report.h"
#include "configuration.h"
#include "conditions.h"
#include "filters.h"
#include "parsers.h"
#include "db.h"
#include "attributes.h"
#include "paths.h"
#include "clients.h"

using namespace hbackup;
using namespace htools;

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

struct HBackup::Private : Config::Object {
  // db
  Database*                 db;
  Database::CompressionMode db_compress_mode;
  // parsers
  std::list<string>         parser_plugins_paths;
  ParsersManager            parsers_manager;
  // clients
  std::list<string>         selected_clients;
  std::list<Client*>        clients;
  // attributes
  Attributes                attributes;
  // log
  string                    log_file_name;
  size_t                    log_max_lines;
  size_t                    log_backups;
  Level                     log_level;
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
  virtual Object* configChildFactory(
      const vector<string>& params,
      const char*           file_path = NULL,
      size_t                line_no   = 0);
};

Config::Object* HBackup::Private::configChildFactory(
    const vector<string>& params,
    const char*           file_path,
    size_t                line_no) {
  Object* co = NULL;
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
      db_compress_mode = Database::always;
      co = this;
    } else
    if (params[1] == "auto") {
      db_compress_mode = Database::auto_now;
      co = this;
    } else
    if (params[1] == "later") {
      db_compress_mode = Database::auto_later;
      co = this;
    } else
    if (params[1] == "never") {
      db_compress_mode = Database::never;
      co = this;
    } else
    {
      hlog_error("Unsupported DB compression mode in '%s', at line %zu",
        file_path, line_no);
    }
  } else
  // Backwards compatibility
  if (keyword == "compress_auto") {
    db_compress_mode = Database::auto_now;
    co = this;
  } else
  if (keyword == "parsers_dir") {
    parser_plugins_paths.push_back(params[1]);
    parsers_manager.loadPlugins(parser_plugins_paths.back().c_str());
    co = this;
  } else
  if (keyword == "client") {
    Client* client = new Client(params[1], attributes, parsers_manager,
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

static void config_error_cb(
    const char*     message,
    const char*     value,
    size_t          line_no) {
  if (line_no > 0) {
    hlog_error("at line %zu: %s '%s'", line_no, message, value);
  } else {
    hlog_error("global: %s '%s'", message, value);
  }
}

int HBackup::readConfig(const char* config_path) {
  // Set up config syntax and grammar
  Config config(config_error_cb);

  // log
  {
    Config::Item* log = config.syntaxAdd(config.syntaxRoot(), "log", 0, 1, 1);
    // max lines per file
    config.syntaxAdd(log, "max_lines", 0, 1, 1, 1);
    // max backups to keep
    config.syntaxAdd(log, "backups", 0, 1, 1, 1);
    // log level
    config.syntaxAdd(log, "level", 0, 1, 1, 1);
  }
  // db
  {
    Config::Item* db = config.syntaxAdd(config.syntaxRoot(), "db", 0, 1, 1);
    // compress [always, auto_now, auto_later, never]
    config.syntaxAdd(db, "compress", 0, 1, 1, 1);
    // old keyword for compress auto
    config.syntaxAdd(db, "compress_auto", 0, 1);
  }
  // tree
  {
    Config::Item* tree = config.syntaxAdd(config.syntaxRoot(), "tree", 0, 1, 1);
    // links: type of links to create [hard, symbolic]
    config.syntaxAdd(tree, "links", 0, 1, 1, 1);
    // compressed: whether to check for compressed data, or assume one way or
    //             the other [yes, no, check]
    config.syntaxAdd(tree, "compressed", 0, 1, 1, 1);
    // hourly: number of hourly backup to keep
    config.syntaxAdd(tree, "hourly", 0, 1, 1, 1);
    // daily: number of daily backup to keep
    config.syntaxAdd(tree, "daily", 0, 1, 1, 1);
    // weekly: number of weekly backup to keep
    config.syntaxAdd(tree, "weekly", 0, 1, 1, 1);
  }
  // parser plugins
  config.syntaxAdd(config.syntaxRoot(), "parsers_dir", 0, 0, 1);
  // filter
  {
    Config::Item* filter =
      config.syntaxAdd(config.syntaxRoot(), "filter", 0, 0, 2);
    // condition
    config.syntaxAdd(filter, "condition", 1, 0, 2);
  }
  // ignore
  config.syntaxAdd(config.syntaxRoot(), "ignore", 0, 1, 1);
  // report_copy_error_once
  config.syntaxAdd(config.syntaxRoot(), "report_copy_error_once", 0, 1);
  // client
  {
    Config::Item* client =
      config.syntaxAdd(config.syntaxRoot(), "client", 1, 0, 1, 2);
    // hostname
    config.syntaxAdd(client, "hostname", 0, 1, 1);
    // protocol
    config.syntaxAdd(client, "protocol", 0, 1, 1);
    // option
    config.syntaxAdd(client, "option", 0, 0, 1, 2);
    // timeout_nowarning
    config.syntaxAdd(client, "timeout_nowarning", 0, 1);
    // report_copy_error_once
    config.syntaxAdd(client, "report_copy_error_once", 0, 1);
    // config
    config.syntaxAdd(client, "config", 0, 1, 1);
    // expire
    config.syntaxAdd(client, "expire", 0, 1, 1);
    // users
    config.syntaxAdd(client, "users", 0, 1, 1, -1);
    // ignore
    config.syntaxAdd(client, "ignore", 0, 1, 1);
    // filter
    {
      Config::Item* filter = config.syntaxAdd(client, "filter", 0, 0, 2);

      // condition
      config.syntaxAdd(filter, "condition", 1, 0, 2);
    }
    // path
    {
      Config::Item* path = config.syntaxAdd(client, "path", 0, 0, 1);
      // parser
      config.syntaxAdd(path, "parser", 0, 0, 2);
      // filter
      {
        Config::Item* filter = config.syntaxAdd(path, "filter", 0, 0, 2);
        // condition
        config.syntaxAdd(filter, "condition", 1, 0, 2);
      }
      // timeout_nowarning
      config.syntaxAdd(path, "report_copy_error_once", 0, 1);
      // ignore
      config.syntaxAdd(path, "ignore", 0, 1, 1);
      // compress
      config.syntaxAdd(path, "compress", 0, 1, 1);
      // no_compress
      config.syntaxAdd(path, "no_compress", 0, 1, 1);
    }
  }

  /* Read configuration file */
  hlog_debug_arrow(1, "Reading configuration file '%s'", config_path);
  ssize_t rc = config.read(config_path, 0, _d);
  if (rc < 0) {
    return -1;
  }

  // Use default DB path if none specified
  if (_d->db == NULL) {
    _d->db = new Database(DEFAULT_DB_PATH);
  }

  // Add default parser plugins paths if none specified
  if (_d->parser_plugins_paths.empty()) {
    _d->parser_plugins_paths.push_back("/usr/local/lib/hbackup");
    _d->parsers_manager.loadPlugins(_d->parser_plugins_paths.back().c_str());
    _d->parser_plugins_paths.push_back("/usr/lib/hbackup");
    _d->parsers_manager.loadPlugins(_d->parser_plugins_paths.back().c_str());
  }

  return 0;
}

int HBackup::open(
    const char*   path,
    bool          no_log,
    bool          user_mode,
    bool          check_config) {
  _d->db_compress_mode = Database::auto_later;

  bool failed = false;
  if (user_mode) {
    // Give 'selected' client name
    addClient("localhost");
    // Set-up client info
    Client* client = new Client("localhost", _d->attributes, _d->parsers_manager);
    client->setProtocol("file");
    client->setListfile(Path(path, ".hbackup/config").c_str());
    client->setBasePath(path);
    if (check_config) {
      failed = client->readConfig(client->listfile()) < 0;
    } else {
      _d->clients.push_back(client);
      // Set-up DB
      _d->db = new Database(Path(path, ".hbackup").c_str());
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
  // Log to file if required
  if (! no_log && (_d->log_file_name != "")) {
    report.startFileLog(_d->log_file_name.c_str(), _d->log_max_lines,
      _d->log_backups);
    report.setFileLogLevel(_d->log_level);
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

    Path mount_dir_path(_d->db->path(), ".mount");
    Node mount_dir(mount_dir_path);
    if (mount_dir.mkdir() < 0) {
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
      Path mount_point_path(mount_dir.path(),
        (*client)->internalName().c_str());
      Node mount_point(mount_point_path);
      // Check that mount dir exists, if not create it
      if (mount_point.mkdir() < 0) {
        return -1;
      }
      if ((*client)->backup(*_d->db, mount_point.path(), _d->attributes.tree(),
          _d->attributes.treeSymlinks(), false)) {
        failed = true;
      }
      remove(mount_point_path);
    }
    ::remove(mount_dir_path);
    _d->db->close();
    return failed ? -1 : 0;
  }
  return -1;
}

int HBackup::listOrRestore(
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
    case Database::always:
      comp_str = "always";
      break;
    case Database::auto_now:
      comp_str = "automatic";
      break;
    case Database::auto_later:
      comp_str = "later";
      break;
    case Database::never:
      comp_str = "never";
      break;
  }
  hlog_debug_arrow(level, "DB compression: %s", comp_str);
  if (! _d->parser_plugins_paths.empty()) {
    hlog_debug_arrow(level, "Parser plugins paths:");
    for (std::list<string>::iterator path_it = _d->parser_plugins_paths.begin();
        path_it != _d->parser_plugins_paths.end(); path_it++) {
      hlog_debug_arrow(level + 1, "%s", path_it->c_str());
    }
  }
  _d->parsers_manager.show(level);
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
