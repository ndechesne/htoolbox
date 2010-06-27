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
static progress_f _progress = NULL;

void hbackup::setProgressCallback(progress_f progress) {
  _progress = progress;
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
    out(debug, msg_number, NULL, test, "Killing trigger reached");
    _aborted = 1;
    return true;
  }
  return false;
}

struct HBackup::Private {
  // db
  Database*               db;
  OpData::CompressionMode db_compress_mode;
  // clients
  std::list<string>       selected_clients;
  std::list<Client*>      clients;
  bool                    remote_clients;
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
};

HBackup::HBackup() : _d(new Private) {
  _d->selected_clients.clear();
  _d->clients.clear();
}

HBackup::~HBackup() {
  for (std::list<Client*>::iterator client = _d->clients.begin();
      client != _d->clients.end(); client++){
    delete *client;
  }
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
    out(error, msg_standard, "Client already selected", -1, name);
    return -1;
  }
  _d->selected_clients.insert(client, name);
  return 0;
}

int HBackup::readConfig(const char* config_path) {
  /* Open configuration file */
  Stream config_file(config_path);

  if (config_file.open(O_RDONLY)) {
    out(error, msg_errno, "opening file", errno, config_path);
    return -1;
  }
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
  // timeout_nowarning
  config_syntax.add(new ConfigItem("timeout_nowarning", 0, 1));
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

      // ignore
      path->add(new ConfigItem("ignore", 0, 1, 1));

      // compress
      path->add(new ConfigItem("compress", 0, 1, 1));

      // no_compress
      path->add(new ConfigItem("no_compress", 0, 1, 1));
    }
  }

  /* Read configuration file */
  out(debug, msg_standard, config_path, 1, "Reading configuration file");
  Config       config;
  ConfigErrors errors;
  int rc = config.read(config_file, Stream::flags_accept_cr_lf, config_syntax,
    NULL, &errors);
  config_file.close();

  if (rc < 0) {
    errors.show();
    return -1;
  }

  Client*     client = NULL;
  ClientPath* c_path = NULL;
  Attributes* attr   = &_d->attributes;
  ConfigLine* params;
  while (config.line(&params) >= 0) {
    if ((*params)[0] == "log") {
      _d->log_file_name = (*params)[1];
    } else
    if ((*params)[0] == "max_lines") {
      _d->log_max_lines = atoi((*params)[1].c_str());
    } else
    if ((*params)[0] == "backups") {
      _d->log_backups = atoi((*params)[1].c_str());
    } else
    if ((*params)[0] == "level") {
      switch ((*params)[1][0]) {
        case 'A':
        case 'a':
          _d->log_level = hreport::alert;
          break;
        case 'E':
        case 'e':
          _d->log_level = hreport::error;
          break;
        case 'W':
        case 'w':
          _d->log_level = hreport::warning;
          break;
        case 'I':
        case 'i':
          _d->log_level = hreport::info;
          break;
        case 'V':
        case 'v':
          _d->log_level = hreport::verbose;
          break;
        case 'D':
        case 'd':
          _d->log_level = hreport::debug;
          break;
        default:
          hlog_error("wrong name for log level: '%s'", (*params)[1].c_str());
          return -1;
      }
    } else
    if ((*params)[0] == "db") {
      _d->db = new Database((*params)[1].c_str());
    } else
    if ((*params)[0] == "filter") {
      // Add filter at the right level (global, client, path)
      if (attr->addFilter((*params)[1], (*params)[2]) == NULL) {
        out(error, msg_number, "Unsupported filter type", (*params).lineNo(),
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
        Filter* subfilter = NULL;
        if (c_path != NULL) {
          subfilter = c_path->findFilter((*params)[2]);
        } else
        if (client != NULL) {
          subfilter = client->findFilter((*params)[2]);
        } else
        {
          subfilter = _d->attributes.findFilter((*params)[2]);
        }
        if (subfilter == NULL) {
          out(error, msg_number, "Filter not found", (*params).lineNo(),
            config_path);
          return -1;
        } else {
          attr->addFilterCondition(new Condition(Condition::filter, subfilter,
            negated));
        }
      } else {
        switch (attr->addFilterCondition(filter_type, (*params)[2].c_str(),
            negated)) {
          case 1:
            out(error, msg_number, "Unsupported condition type",
              (*params).lineNo(), config_path);
            return -1;
            break;
          case 2:
            out(error, msg_number, "No filter defined",
              (*params).lineNo(), config_path);
            return -1;
        }
      }
    } else
    if ((*params)[0] == "ignore") {
      Filter* filter = NULL;
      if (c_path != NULL) {
        filter = c_path->findFilter((*params)[1]);
      } else
      if (client != NULL) {
        filter = client->findFilter((*params)[1]);
      } else
      {
        filter = _d->attributes.findFilter((*params)[1]);
      }
      if (filter == NULL) {
        out(error, msg_number, "Filter not found", (*params).lineNo(),
          config_path);
        return -1;
      } else {
        attr->addIgnore(filter);
      }
    } else
    if ((*params)[0] == "client") {
      c_path = NULL;
      client = new Client(_d->attributes, (*params)[1],
        (params->size() > 2) ? (*params)[2] : "");

      // Clients MUST BE in alphabetic order
      int cmp = 1;
      std::list<Client*>::iterator i = _d->clients.begin();
      while (i != _d->clients.end()) {
        cmp = client->internalName().compare((*i)->internalName());
        if (cmp <= 0) {
          break;
        }
        i++;
      }
      if (cmp == 0) {
        out(error, msg_standard, "Client already selected", -1,
          client->internalName().c_str());
        return -1;
      }
      _d->clients.insert(i, client);
      attr = &client->attributes;
      // Inherit some attributes when set
      *attr = _d->attributes;
    } else
    if (client != NULL) {
      if ((*params)[0] == "hostname") {
        client->setHostOrIp((*params)[1]);
      } else
      if ((*params)[0] == "protocol") {
        if (client->setProtocol((*params)[1])) {
          _d->remote_clients = true;
        }
      } else
      if ((*params)[0] == "option") {
        if ((*params).size() == 2) {
          client->addOption((*params)[1]);
        } else {
          client->addOption((*params)[1], (*params)[2]);
        }
      } else
      if ((*params)[0] == "users") {
        for (unsigned int i = 1; i < params->size(); i++) {
          client->addUser((*params)[i]);
        }
      } else
      if ((*params)[0] == "timeout_nowarning") {
        client->setTimeOutNoWarning();
      } else
      if ((*params)[0] == "report_copy_error_once") {
        attr->setReportCopyErrorOnce();
      } else
      if ((*params)[0] == "config") {
        client->setListfile((*params)[1].c_str());
      } else
      if ((*params)[0] == "expire") {
        unsigned int expire;
        if (sscanf((*params)[1].c_str(), "%u", &expire) != 1) {
          out(error, msg_number, "Expected decimal argument",
            (*params).lineNo(), config_path);
          return -1;
        }
        client->setExpire(expire * 3600 * 24);
      } else
      if ((*params)[0] == "path") {
        c_path = client->addClientPath((*params)[1]);
        attr = &c_path->attributes;
        // Inherit some attributes when set
        *attr = client->attributes;
      } else
      if (c_path != NULL) {
        if (((*params)[0] == "compress")
         || ((*params)[0] == "no_compress")) {
          Filter* filter = c_path->findFilter((*params)[1]);
          if (filter == NULL) {
            out(error, msg_number, "Filter not found", (*params).lineNo(),
              config_path);
            return -1;
          } else {
            if ((*params)[0] == "compress") {
              c_path->setCompress(filter);
            } else
            if ((*params)[0] == "no_compress") {
              c_path->setNoCompress(filter);
            }
          }
        } else
        if ((*params)[0] == "parser") {
          switch (c_path->addParser((*params)[1], (*params)[2])) {
            case 1:
              out(error, msg_number, "Unsupported parser type",
                (*params).lineNo(), config_path);
              return -1;
            case 2:
              out(error, msg_number, "Unsupported parser mode",
                (*params).lineNo(), config_path);
              return -1;
          }
        }
      }
    } else {
      if ((*params)[0] == "compress") {
        if ((*params)[1] == "always") {
          _d->db_compress_mode = OpData::always;
        } else
        if ((*params)[1] == "auto") {
          _d->db_compress_mode = OpData::auto_now;
        } else
        if ((*params)[1] == "later") {
          _d->db_compress_mode = OpData::auto_later;
        } else
        if ((*params)[1] == "never") {
          _d->db_compress_mode = OpData::never;
        } else
        {
          out(error, msg_number, "Unsupported DB compression mode",
            (*params).lineNo(), config_path);
          return -1;
        }
      } else
      // Backwards compatibility
      if ((*params)[0] == "compress_auto") {
        _d->db_compress_mode = OpData::auto_now;
      } else
      if ((*params)[0] == "report_copy_error_once") {
        _d->attributes.setReportCopyErrorOnce();
      }
    }
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
  _d->remote_clients   = false;
  _d->db_compress_mode = OpData::auto_later;

  bool failed = false;
  if (user_mode) {
    // Give 'selected' client name
    addClient("localhost");
    // Set-up client info
    Client* client = new Client(_d->attributes, "localhost");
    client->setProtocol("file");
    client->setListfile(Path(path, ".hbackup/config"));
    client->setBasePath(path);
    if (check_config) {
      failed = client->readConfig(client->listfile()) < 0;
    } else {
      _d->clients.push_back(client);
      // Set-up DB
      _d->db = new Database(Path(path, ".hbackup"));
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
    // Set auto-compression mode
    _d->db->setCompressionMode(_d->db_compress_mode);

    Directory mount_dir(Path(_d->db->path(), ".mount"));
    if (_d->remote_clients && (mount_dir.create() < 0)) {
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
      if (_d->remote_clients && (mount_point.create() < 0)) {
        return -1;
      }
      if ((*client)->backup(*_d->db, mount_point.path())) {
        failed = true;
      }
      if (_d->remote_clients) {
        mount_point.remove();
      }
    }
    if (_d->remote_clients) {
      mount_dir.remove();
    }
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
    out(error, msg_standard, "Neither destination nor list given", -1, NULL);
    return -1;
  }
  std::list<string>& records = *names;
  if (_d->selected_clients.empty()) {
    if (dest == NULL) {
      failed = (_d->db->getClients(records) != 0);
    } else {
      out(error, msg_standard, "No client given", -1, NULL);
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
    out(debug, msg_standard, "No file logging", level, NULL);
  } else {
    out(debug, msg_standard, _d->log_file_name.c_str(), level, "Log path");
    if (_d->log_max_lines == 0) {
      out(debug, msg_standard, "No size limit", level + 1, NULL);
    } else {
      char number[32];
      sprintf(number, "%u", _d->log_max_lines);
      out(debug, msg_standard, number, level + 1, "Max lines per log");
    }
    if (_d->log_backups == 0) {
      out(debug, msg_standard, "No log file backup", level + 1, NULL);
    } else {
      char number[32];
      sprintf(number, "%u", _d->log_backups);
      out(debug, msg_standard, number, level + 1, "Backup(s)");
    }
    out(debug, msg_standard, Report::levelString(_d->log_level), level + 1,
      "Level");
  }
  if (_d->db == NULL) {
    out(debug, msg_standard, "DB path not set", level, NULL);
  } else {
    out(debug, msg_standard, _d->db->path(), level, "DB path");
  }
  switch (_d->db_compress_mode) {
    case OpData::always:
      out(debug, msg_standard, "DB compression: always", level, NULL);
      break;
    case OpData::auto_now:
      out(debug, msg_standard, "DB compression: automatic", level, NULL);
      break;
    case OpData::auto_later:
      out(debug, msg_standard, "DB compression: later", level, NULL);
      break;
    case OpData::never:
      out(debug, msg_standard, "DB compression: never", level, NULL);
      break;
  }
  if (! _d->selected_clients.empty()) {
    out(debug, msg_standard, "Selected clients:", level, NULL);
    for (std::list<string>::iterator client = _d->selected_clients.begin();
        client != _d->selected_clients.end(); client++) {
      out(debug, msg_standard, client->c_str(), level + 1, NULL);
    }
  }
  if (! _d->clients.empty()) {
    out(debug, msg_standard, "Clients:", level, NULL);
    for (std::list<Client*>::iterator client = _d->clients.begin();
        client != _d->clients.end(); client++) {
      (*client)->show(level + 1);
    }
  }
  _d->attributes.show(level);
}
