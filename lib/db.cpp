/*
     Copyright (C) 2006-2010  Herve Fache

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

// Compression to use when required: gzip -5 (best speed/ratio)

#include <string>
#include <list>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "hreport.h"
#include "missing.h"
#include "compdata.h"
#include "data.h"
#include "db.h"
#include "owner.h"

namespace hbackup {
  typedef enum {
    no,
    ro,
    rw,
    quiet_rw
  } Access;
};

using namespace hbackup;
using namespace hreport;

struct Database::Private {
  char*                   path;
  Access                  access;
  Data                    data;
  Missing                 missing;
  bool                    load_missing;
  CompressionMode         compress_mode;
  Owner*                  owner;
  progress_f              list_progress;
};

int Database::lock() {
  Path  lock_path(_d->path, ".lock");
  FILE* file;
  bool  failed = false;

  // Set the database path that we just locked as default
  // Try to open lock file for reading: check who's holding the lock
  if ((file = fopen(lock_path, "r")) != NULL) {
    pid_t pid = 0;

    // Lock already taken
    if (fscanf(file, "%d", &pid) < 1) {
      pid = 0;
    }
    fclose(file);
    if (pid != 0) {
      // Find out whether process is still running, if not, reset lock
      kill(pid, 0);
      if (errno == ESRCH) {
        hlog_warning("Database lock reset");
        ::remove(lock_path);
      } else {
        hlog_error("Database lock taken by process with pid %d", pid);
        failed = true;
      }
    } else {
      hlog_error("Database lock taken by an unidentified process!");
      failed = true;
    }
  }

  // Try to open lock file for writing: lock
  if (! failed) {
    if ((file = fopen(lock_path, "w")) != NULL) {
      // Lock taken
      fprintf(file, "%u\n", getpid());
      fclose(file);
    } else {
      // Lock cannot be taken
      hlog_error("%s trying to lock database", strerror(errno));
      failed = true;
    }
  }
  if (failed) {
    return -1;
  }
  return 0;
}

void Database::unlock() {
  File(Path(_d->path, ".lock")).remove();
}

Database::Database(const char* path) : _d(new Private) {
  _d->path     = strdup(path);
  _d->access   = no;
  // Reset progress callback function
  _d->list_progress = NULL;
}

Database::~Database() {
  if (_d->access != no) {
    close();
  }
  free(_d->path);
  delete _d;
}

const char* Database::path() const {
  return _d->path;
}

int Database::open(
    bool            read_only,
    bool            initialize) {

  if (read_only && initialize) {
    hlog_error("Cannot initialize in read-only mode");
    return -1;
  }

  if (! Directory(_d->path).isValid()) {
    if (read_only || ! initialize) {
      hlog_error("Given DB path does not exist '%s'", _d->path);
      return -1;
    } else
    if (mkdir(_d->path, 0755)) {
      hlog_error("Cannot create DB base directory");
      return -1;
    }
  }
  // Try to take lock
  if (! read_only && lock()) {
    return -1;
  }

  // Check DB dir
  switch (_d->data.open(Path(_d->path, ".data"), initialize)) {
    case 1:
      // Creation successful
      File(Path(_d->path, ".checksums")).create();
      hlog_verbose("Database initialized in '%s'", _d->path);
    case 0:
      // Open successful
      break;
    default:
      // Creation failed
      if (initialize) {
        hlog_error("%s creating data directory in '%s'", strerror(errno),
          _d->path);
      } else {
        hlog_error("Given DB path does not contain a database '%s'", _d->path);
        if (! read_only) {
          hlog_info("See help for information on how to initialize the DB");
        }
      }
      if (! read_only) {
        unlock();
      }
      return -1;
  }

  // Auto-compression at scan time
  _d->compress_mode = auto_later;
  // Reset client's data
  _d->owner = NULL;
  // Do not load list of missing items for now
  _d->load_missing = false;

  if (read_only) {
    _d->access = ro;
    hlog_verbose("Database open in read-only mode");
  } else {
    // Open problematic checksums list
    _d->missing.open(Path(_d->path, ".checksums"));
    // Set mode to call openClient for checking only
    _d->access = quiet_rw;
    // Finish off any previous backup
    list<string> clients;
    if (getClients(clients) < 0) {
      unlock();
      return -1;
    }
    bool failed = false;
    for (list<string>::iterator i = clients.begin(); i != clients.end(); i++) {
      if (openClient(i->c_str()) >= 0) {
        closeClient(true);
      } else {
        failed = true;
      }
      if (aborting()) {
        break;
      }
    }
    if (failed) {
      return -1;
    }
    _d->access = rw;
    // Load list of missing items if/when required
    _d->load_missing = true;
    hlog_verbose("Database open in read/write mode");
  }
  return 0;
}

int Database::close() {
  bool failed = false;

  if (_d->access == no) {
    hlog_alert("Cannot close: DB not open!");
  } else {
    if (_d->owner != NULL) {
      closeClient(true);
    }
    // Close data manager
    _d->data.close();
    if (_d->access >= rw) {
      // Save list of missing items
      _d->missing.close();
      // Release lock
      unlock();
    }
  }
  hlog_verbose("Database closed");
  _d->access = no;
  return failed ? -1 : 0;
}

void Database::setCompressionMode(
    CompressionMode mode) {
  _d->compress_mode = mode;
}

void Database::setCopyProgressCallback(progress_f progress) {
  _d->data.setProgressCallback(progress);
}

void Database::setListProgressCallback(progress_f progress) {
  _d->list_progress = progress;
}

int Database::getClients(
    list<string>& clients) const {
  Directory dir(_d->path);
  if (dir.createList() < 0) {
    return -1;
  }
  list<Node*>::iterator i = dir.nodesList().begin();
  while (i != dir.nodesList().end()) {
    if ((*i)->type() == 'd') {
      if (File(Path((*i)->path(), "list")).isValid()) {
        clients.push_back((*i)->name());
      }
    }
    i++;
  }
  return 0;
}

int Database::getRecords(
    list<string>&   records,
    const char*     path,
    time_t          date) {
  size_t path_len = 0;
  if (path != NULL) {
    path_len = strlen(path);
    // Make sure we compare excluding any trailing slashes
    while ((path_len > 0) && (path[path_len - 1] == '/')) path_len--;
  }
  // Treat negative dates as relative from now (leave 0 alone as it's faster)
  if (date < 0) {
    date = time(NULL) + date;
  }
  char*  db_path = NULL;
  Node*  db_node = NULL;
  string last_record;
  int   rc;
  while ((rc = _d->owner->getNextRecord(path, date, &db_path, &db_node)) > 0) {
    if (aborting()) {
      return -1;
    }
    if ((strncmp(path, db_path, path_len) == 0)
    &&  ((db_path[path_len] == '/') || (path_len == 0))) {
      char* slash = strchr(&db_path[path_len + 1], '/');
      if (slash != NULL) {
        *slash = '\0';
      }

      if (last_record != db_path) {
        last_record = db_path;
        string record = last_record;
        if ((slash != NULL)   // Was shortened, therefore is dir
        || ((db_node != NULL) && (db_node->type() == 'd'))) {
          record += '/';
        }
        records.push_back(record);
      }
    }
  }
  free(db_path);
  delete db_node;
  return (rc < 0) ? -1 : 0;
}

int Database::restore(
    const char*     dest,
    HBackup::LinkType links,
    const char*     path,
    time_t          date) {
  if (date < 0) {
    date = time(NULL) + date;
  }
  bool  failed = false;
  char* db_path  = NULL;
  Node* db_node  = NULL;
  int   rc;
  while ((rc = _d->owner->getNextRecord(path, date, &db_path, &db_node)) > 0) {
    if (db_node == NULL) {
      continue;
    }
    bool this_failed = false;
    Path base(dest, db_path);
    Path dir = base.dirname();
    if (! Directory(dir).isValid()) {
      string command = "mkdir -p \"";
      command += dir;
      command += "\"";
      if (system(command.c_str())) {
        hlog_error("%s creating path '%s'", strerror(errno), dir.c_str());
      }
    }
    char code;
    if (db_node->type() == 'f') {
      switch (links) {
        case HBackup::symbolic:
          code = 'L';
          break;
        case HBackup::hard:
          code = 'H';
          break;
        default:
          code = 'U';
      }
    } else {
      code = 'U';
    }
    hlog_info("%-8c%s", code, base.c_str());
    switch (db_node->type()) {
      case 'f': {
          File* f = static_cast<File*>(db_node);
          if (f->checksum()[0] == '\0') {
            hlog_error("Failed to restore file: data missing");
            this_failed = true;
          } else
          if (links == HBackup::none) {
            if (_d->data.read(base, f->checksum())) {
              hlog_error("%s restoring file '%s'", strerror(errno),
                base.c_str());
              this_failed = true;
            }
          } else {
            string path;
            string extension;
            string dest(base);
            if (_d->data.name(f->checksum(), path, extension)) {
              this_failed = true;
            } else {
              dest += extension;
              if (links == HBackup::symbolic) {
                if (symlink(path.c_str(), dest.c_str())) {
                  hlog_error("%s sym-linking file %s", strerror(errno),
                    dest.c_str());
                  this_failed = true;
                }
              } else {
                if (link(path.c_str(), dest.c_str())) {
                  hlog_error("%s hard-linking file %s", strerror(errno),
                    dest.c_str());
                  this_failed = true;
                }
              }
            }
          }
        } break;
      case 'd':
        if (mkdir(base, db_node->mode())) {
          hlog_error("%s restoring dir '%s'", strerror(errno), base.c_str());
          this_failed = true;
        }
        break;
      case 'l': {
          Link* l = static_cast<Link*>(db_node);
          if (symlink(l->link(), base)) {
            hlog_error("%s restoring file '%s'", strerror(errno), base.c_str());
            this_failed = true;
          }
        } break;
      case 'p':
        if (mkfifo(base, db_node->mode())) {
          hlog_error("%s restoring pipe (FIFO) '%s'", strerror(errno),
            base.c_str());
          this_failed = true;
        }
        break;
      default:
        char type[2] = { db_node->type(), '\0' };
        hlog_error("Type not supported '%s'", type);
        this_failed = true;
    }
    // Report error and go on
    if (this_failed) {
      failed = true;
      continue;
    }
    // Nothing more to do for links
    if ((db_node->type() == 'l') || (links != HBackup::none)) {
      continue;
    }
    // Restore modification time
    {
      struct utimbuf times = { -1, db_node->mtime() };
      if (utime(base, &times)) {
        hlog_error("%s restoring modification time for '%s'", strerror(errno),
          base.c_str());
        this_failed = true;
      }
    }
    // Restore permissions
    if (chmod(base, db_node->mode())) {
      hlog_error("%s restoring permissions for '%s'", strerror(errno),
        base.c_str());
      this_failed = true;
    }
    // Restore owner and group
    if (chown(base, db_node->uid(), db_node->gid())) {
      hlog_error("%s restoring owner/group for '%s'", strerror(errno),
        base.c_str());
      this_failed = true;
    }
    // Report error and go on
    if (this_failed) {
      failed = true;
    }
  }
  free(db_path);
  delete db_node;
  return (failed || (rc < 0)) ? -1 : 0;
}

int Database::scan(
    bool            rm_obsolete) const {
  // Get checksums from list
  list<CompData> list_data;
  list<string> clients;
  if (getClients(clients) < 0) {
    return -1;
  }
  // Do not load list of missing items for now
  _d->load_missing = false;
  bool failed = false;
  for (list<string>::iterator i = clients.begin(); i != clients.end(); i++) {
    hlog_verbose("Reading list for '%s'", i->c_str());
    Owner client(_d->path, i->c_str());
    client.setProgressCallback(_d->list_progress);
    client.getChecksums(list_data);
    if (failed || aborting()) {
      break;
    }
  }
  if (failed || aborting()) {
    return -1;
  }
  list_data.sort();
  // Unique, must do something if checksums match, but not sizes
  for (list<CompData>::iterator i = list_data.begin(); i != list_data.end();
      i++) {
    list<CompData>::iterator j = i;
    j++;
    while ((j != list_data.end())
    && (strcmp(i->checksum(), j->checksum()) == 0)) {
      if (i->size() != j->size()) {
        i->signalBroken();
      }
      j = list_data.erase(j);
    }
  }
  hlog_verbose("Found %zu checksum(s)", list_data.size());

  // Get checksums from DB
  hlog_verbose("Crawling through DB");
  list<CompData> data_data;
  // Check surficially, remove empty dirs
  int rc = _d->data.crawl(false, true, &data_data);
  if (aborting() || (rc < 0)) {
    return -1;
  }

  if (! list_data.empty()) {
    hlog_debug("Checksum(s) from list: %zu", list_data.size());
    for (list<CompData>::iterator i = list_data.begin();
        i != list_data.end(); i++) {
      hlog_debug_arrow(1, "%s, %lld", i->checksum().c_str(), i->size());
    }
  }
  if (! data_data.empty()) {
    hlog_debug("Checksum(s) with data: %zu", data_data.size());
    for (list<CompData>::iterator i = data_data.begin();
        i != data_data.end(); i++) {
      hlog_debug_arrow(1, "%s, %lld", i->checksum().c_str(), i->size());
    }
  }

  // Separate missing and obsolete checksums
  list<CompData>::iterator l = list_data.begin();
  list<CompData>::iterator  d = data_data.begin();
  while ((d != data_data.end()) || (l != list_data.end())) {
    int cmp;
    if (d == data_data.end()) {
      cmp = 1;
    } else
    if (l == list_data.end()) {
      cmp = -1;
    } else
    {
      cmp = strcmp(d->checksum(), l->checksum());
    }
    if (cmp > 0) {
      // Checksum is missing
      _d->missing.setMissing(l->checksum());
      l++;
    } else
    if (cmp < 0) {
      // Keep checksum
      d++;
    } else
    {
      if (d->size() != l->size()) {
        // Data is broken
        _d->missing.setInconsistent(d->checksum(), d->size());
      }
      // Next
      d = data_data.erase(d);
      l++;
    }
  }
  list_data.clear();

  if (! data_data.empty()) {
    hlog_info("Obsolete checksum(s): %zu", data_data.size());
    for (list<CompData>::iterator i = data_data.begin();
        i != data_data.end(); i++) {
      if (i->checksum()[0] != '\0') {
        if (rm_obsolete) {
          if (_d->data.remove(i->checksum()) == 0) {
            hlog_debug_arrow(1, "removed data for %s", i->checksum().c_str());
          } else {
            hlog_error("%s removing data for %s", strerror(errno),
              i->checksum().c_str());
          }
        } else {
          hlog_debug_arrow(1, "%s", i->checksum().c_str());
        }
      }
    }
  }
  _d->missing.show();

  if (aborting()) {
    return -1;
  }
  _d->missing.forceSave();
  if (rc >= 0) {
    // Leave trace of successful scan
    File f(Path(_d->path, ".last-scan"));
    f.remove();
    f.create();
  }
  return rc;
}

int Database::check(
    bool            remove) const {
  hlog_verbose("Crawling through DB data");
  // Check thoroughly, remove corrupted data if told (but not empty dirs)
  int rc = _d->data.crawl(true, remove);
  if (rc >= 0) {
    // Leave trace of successful check
    File f(Path(_d->path, ".last-check"));
    f.remove();
    f.create();
  }
  return rc;
}

int Database::openClient(
    const char*     client,
    time_t          expire) {
  if (_d->access == no) {
    hlog_alert("Open DB before opening client!");
    return -1;
  }

  // Create owner for client
  _d->owner = new Owner(_d->path, client,
    (expire <= 0) ? expire : (time(NULL) - expire));
  _d->owner->setProgressCallback(_d->list_progress);

  // Read/write open
  if (_d->access >= rw) {
    if (_d->load_missing) {
      _d->missing.load();
      _d->load_missing = false;
    }
    if (_d->owner->open(true, _d->access == quiet_rw) < 0) {
      delete _d->owner;
      _d->owner = NULL;
      return -1;
    }
  } else
  // Read-only open
  {
    if (_d->owner->hold() < 0) {
      delete _d->owner;
      _d->owner = NULL;
      return -1;
    }
  }
  // Report
  if ((_d->access == ro) || (_d->access == rw)) {
    hlog_verbose("Database open %s for '%s'",
      _d->access == ro ? "r-o" : "r/w", _d->owner->name());
  }
  return 0;
}

int Database::closeClient(
    bool            abort) {
  bool failed = (_d->owner->close(abort) < 0);
  if (_d->access < quiet_rw) {
    hlog_verbose("Database closed for '%s'", _d->owner->name());
  }
  if (! failed && ! abort) {
    // Leave trace of successful check
    File f(Path(_d->owner->path(), ".last-backup"));
    f.remove();
    f.create();
  }
  delete _d->owner;
  _d->owner = NULL;
  return failed ? -1 : 0;
}

void Database::sendEntry(
    OpData&         op) {
  _d->owner->send(op, _d->missing);
  // Tell caller about compression mode
  op.setCompressionMode(_d->compress_mode);
}

int Database::add(
    OpData&         op,
    bool            report_copy_error_once) {
  bool failed = false;

  // Add new record to active list
  if ((op._node.type() == 'l') && ! op._node.parsed()) {
    hlog_error("Bug in db add: link was not parsed!");
    return -1;
  }

  if (op._node.type() == 'f') {
    if (static_cast<const File&>(op._node).checksum()[0] == '\0') {
      // Copy data
      char* checksum = NULL;
      int compression;
      if (op._comp_mode == never) {
        compression = -1;
      } else {
        compression = op._compression;
      }
      Stream source(op._node.path());
      int rc = _d->data.write(source, _d->owner->name(), &checksum,
        compression, op._comp_mode == auto_now, &compression);
      if (rc >= 0) {
        if (rc > 0) {
          if (compression > 0) {
            op._type = 'z';
          } else {
            op._type = 'f';
          }
          if (rc > 1) {
            op._info = 'r';
          }
        } else {
          // File data found in DB
          op._type = '~';
        }
        static_cast<File&>(op._node).setChecksum(checksum);
        if ((op._id >= 0) && (_d->missing[op._id] == checksum)) {
          // Mark checksum as recovered
          _d->missing.setRecovered(op._id);
        }
      } else {
        if ((op._operation == '!') && report_copy_error_once) {
          hlog_warning("%s backing up file '%s:%s'", strerror(errno),
            _d->owner->name(), static_cast<const char*>(op._path));
        } else {
          hlog_error("%s backing up file '%s:%s'", strerror(errno),
            _d->owner->name(), static_cast<const char*>(op._path));
        }
        op._type = '!';
        failed = true;
      }
      free(checksum);
#if 0
    } else {
      // Checksum copied over
#endif
    }
  } else
  if (op._node.type() == 'd') {
    if (op._node.size() == -1) {
      op._type = '!';
      failed = true;
    }
  }

  // Even if failed, add data if new
  if (! failed || ! op._same_list_entry) {
    // Add entry info to journal
    if (_d->owner->add(op._path, &op._node) < 0) {
      hlog_error("Cannot add to client's list");
      failed = true;
    }
  }

  return failed ? -1 : 0;
}
