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

// Compression to use when required: gzip -5 (best speed/ratio)

#include <sstream>
#include <string>
#include <list>

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
#include "report.h"
#include "missing.h"
#include "opdata.h"
#include "compdata.h"
#include "owner.h"
#include "data.h"
#include "db.h"

namespace hbackup {
  typedef enum {
    no,
    ro,
    rw,
    quiet_rw
  } Access;
};

using namespace hbackup;

struct Database::Private {
  char*             path;
  Access            access;
  Data              data;
  Missing           missing;
  bool              load_missing;
  bool              compress_auto;
  Owner*            owner;
  progress_f        progress;
};

int Database::lock() {
  Path  lock_path(_d->path, "lock");
  FILE* file;
  bool  failed = false;

  // Set the database path that we just locked as default
  // Try to open lock file for reading: check who's holding the lock
  if ((file = fopen(lock_path, "r")) != NULL) {
    pid_t pid = 0;

    // Lock already taken
    fscanf(file, "%d", &pid);
    fclose(file);
    if (pid != 0) {
      // Find out whether process is still running, if not, reset lock
      kill(pid, 0);
      if (errno == ESRCH) {
        out(warning, msg_standard, "Lock reset");
        std::remove(lock_path);
      } else {
        stringstream s;
        s << "Lock taken by process with pid " << pid;
        out(error, msg_standard, s.str().c_str());
        failed = true;
      }
    } else {
      out(error, msg_standard, "Lock taken by an unidentified process!");
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
      out(error, msg_standard, "Cannot take lock");
      failed = true;
    }
  }
  if (failed) {
    return -1;
  }
  return 0;
}

void Database::unlock() {
  File(Path(_d->path, "lock")).remove();
}

Database::Database(const char* path) : _d(new Private) {
  _d->path     = strdup(path);
  _d->access   = no;
  // Reset progress callback function
  _d->progress = NULL;
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
    out(error, msg_standard, "Cannot initialize in read-only mode");
    return -1;
  }

  if (! Directory(_d->path).isValid()) {
    if (read_only || ! initialize) {
      out(error, msg_standard, _d->path, -1, "Given DB path does not exist");
      return -1;
    } else
    if (mkdir(_d->path, 0755)) {
      out(error, msg_standard, "Cannot create DB base directory");
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
      File(Path(_d->path, "missing")).create();
      out(verbose, msg_standard, _d->path, -1, "Database initialized");
    case 0:
      // Open successful
      break;
    default:
      // Creation failed
      if (initialize) {
        out(error, msg_errno, "Create data directory in given DB path", errno,
          _d->path);
      } else {
        out(error, msg_standard, _d->path, -1,
          "Given DB path does not contain a database");
        if (! read_only) {
          out(info, msg_standard,
            "See help for information on how to initialize the DB");
        }
      }
      if (! read_only) {
        unlock();
      }
      return -1;
  }

  // Disable auto-compression mode
  _d->compress_auto = false;
  // Reset client's data
  _d->owner = NULL;
  // Do not load list of missing items for now
  _d->load_missing = false;

  if (read_only) {
    _d->access = ro;
    out(verbose, msg_standard, "Database open in read-only mode");
  } else {
    _d->missing.open(Path(_d->path, "missing"));
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
    out(verbose, msg_standard, "Database open in read/write mode");
  }
  return 0;
}

int Database::close() {
  bool failed = false;

  if (_d->access == no) {
    out(alert, msg_standard, "Cannot close: DB not open!");
  } else {
    if (_d->owner != NULL) {
      closeClient(true);
    }
    if (_d->access >= rw) {
      // Save list of missing items
      _d->missing.close();
      // Release lock
      unlock();
    }
  }
  out(verbose, msg_standard, "Database closed");
  _d->access = no;
  return failed ? -1 : 0;
}

void Database::setAutoCompression(bool enabled) {
  _d->compress_auto = enabled;
}

void Database::setProgressCallback(progress_f progress) {
  _d->progress = progress;
  _d->data.setProgressCallback(progress);
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
  unsigned int path_len = 0;
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
        out(error, msg_errno, "Creating path", errno, dir);
      }
    }
    if (db_node->type() == 'f') {
      switch (links) {
        case HBackup::none:
          out(info, msg_standard, base, -2, "U");
          break;
        case HBackup::symbolic:
          out(info, msg_standard, base, -2, "L");
          break;
        case HBackup::hard:
          out(info, msg_standard, base, -2, "H");
      }
    } else {
      out(info, msg_standard, base, -2, "U");
    }
    switch (db_node->type()) {
      case 'f': {
          File* f = (File*) db_node;
          if (f->checksum()[0] == '\0') {
            out(error, msg_standard, "Failed to restore file: data missing");
            this_failed = true;
          } else
          if (links == HBackup::none) {
            if (_d->data.read(base, f->checksum())) {
              out(error, msg_errno, "Restoring file", errno);
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
                  out(error, msg_errno, "Sym-linking file", errno);
                  this_failed = true;
                }
              } else {
                if (link(path.c_str(), dest.c_str())) {
                  out(error, msg_errno, "Hard-linking file", errno);
                  this_failed = true;
                }
              }
            }
          }
        } break;
      case 'd':
        if (mkdir(base, db_node->mode())) {
          out(error, msg_errno, "Restoring dir", errno);
          this_failed = true;
        }
        break;
      case 'l': {
          Link* l = (Link*) db_node;
          if (symlink(l->link(), base)) {
            out(error, msg_errno, "Restoring file", errno);
            this_failed = true;
          }
        } break;
      case 'p':
        if (mkfifo(base, db_node->mode())) {
          out(error, msg_errno, "Restoring pipe (FIFO)", errno);
          this_failed = true;
        }
        break;
      default:
        char type[2] = { db_node->type(), '\0' };
        out(error, msg_errno, "Type not supported", -1, type);
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
        out(error, msg_errno, "Restoring modification time");
        this_failed = true;
      }
    }
    // Restore permissions
    if (chmod(base, db_node->mode())) {
      out(error, msg_errno, "Restoring permissions");
      this_failed = true;
    }
    // Restore owner and group
    if (chown(base, db_node->uid(), db_node->gid())) {
      out(error, msg_errno, "Restoring owner/group");
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
    out(verbose, msg_standard, i->c_str(), -1, "Reading list");
    Owner client(_d->path, i->c_str());
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
#if 0 // FIXME This needs more work
      if (i->sizeCompare(*j) != 0) {
        i->signalBroken();
      }
#endif
      j = list_data.erase(j);
    }
  }

  {
    stringstream s;
    s << "Found " << list_data.size() << " checksums";
    out(verbose, msg_standard, s.str().c_str());
  }

  // Get checksums from DB
  out(verbose, msg_standard, "Crawling through DB");
  list<CompData> data_data;
  // Check surficially, remove empty dirs
  int rc = _d->data.crawl(false, true, &data_data);
  if (aborting() || (rc < 0)) {
    return -1;
  }

  if (! list_data.empty()) {
    out(debug, msg_number, NULL, list_data.size(), "Checksum(s) from list");
    for (list<CompData>::iterator i = list_data.begin();
        i != list_data.end(); i++) {
      stringstream s;
      s << i->checksum() << ", " << i->size();
      out(debug, msg_standard, s.str().c_str(), 1);
    }
  }
  if (! data_data.empty()) {
    out(debug, msg_number, NULL, data_data.size(), "Checksum(s) with data");
    for (list<CompData>::iterator i = data_data.begin();
        i != data_data.end(); i++) {
      stringstream s;
      s << i->checksum() << ", " << i->size();
      out(debug, msg_standard, s.str().c_str(), 1);
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
      _d->missing.push_back(l->checksum());
      l++;
    } else
    if (cmp < 0) {
      // Keep checksum
      d++;
    } else
    {
#if 0 // FIXME This needs more work
      if (d->sizeCompare(*l) != 0) {
        // Data is broken
        _d->missing.push_back(l->checksum());
      }
#endif
      // Next
      d = data_data.erase(d);
      l++;
    }
  }
  list_data.clear();

  if (! data_data.empty()) {
    out(info, msg_number, NULL, data_data.size(), "Obsolete checksum(s)");
    for (list<CompData>::iterator i = data_data.begin();
        i != data_data.end(); i++) {
      if (i->checksum()[0] != '\0') {
        if (rm_obsolete) {
          if (_d->data.remove(i->checksum()) == 0) {
            out(debug, msg_standard, "removed", 1, i->checksum());
          } else {
            out(error, msg_errno, "Removing data", errno, i->checksum());
          }
        } else {
          out(debug, msg_standard, i->checksum(), 1);
        }
      }
    }
  }
  _d->missing.show();

  if (aborting()) {
    return -1;
  }
  if (rc >= 0) {
    // Leave trace of successful scan
    File(Path(_d->path, ".last-scan")).create();
  }
  return rc;
}

int Database::check(
    bool            remove) const {
  out(verbose, msg_standard, "Crawling through DB data");
  // Check thoroughly, remove corrupted data if told (but not empty dirs)
  int rc = _d->data.crawl(true, remove);
  if (rc >= 0) {
    // Leave trace of successful check
    File(Path(_d->path, ".last-check")).create();
  }
  return rc;
}

int Database::openClient(
    const char*     client,
    time_t          expire) {
  if (_d->access == no) {
    out(alert, msg_standard, "Open DB before opening client!");
    return -1;
  }

  // Create owner for client
  _d->owner = new Owner(_d->path, client,
    (expire <= 0) ? expire : (time(NULL) - expire));
  _d->owner->setProgressCallback(_d->progress);

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
  // Report and set temporary data path
  switch (_d->access) {
    case ro:
      out(verbose, msg_standard, _d->owner->name(), -1, "Database open r-o");
      break;
    case rw:
      out(verbose, msg_standard, _d->owner->name(), -1, "Database open r/w");
      break;
    default:;
  }
  return 0;
}

int Database::closeClient(
    bool            abort) {
  bool failed = (_d->owner->close(abort) < 0);
  if (_d->access < quiet_rw) {
    out(verbose, msg_standard, _d->owner->name(), -1, "Database closed");
  }
  delete _d->owner;
  _d->owner = NULL;
  return failed ? -1 : 0;
}

void Database::sendEntry(
    OpData&         op) {
  _d->owner->sendEntry(op, _d->missing);
  // Tell caller about auto compression
  if (_d->compress_auto) {
    op._compression = -1;
  }
}

int Database::add(
    const OpData&   op,
    bool            report_copy_error_once) {
  bool failed = false;
  time_t ts = time(NULL);

  // Add new record to active list
  if ((op._node.type() == 'l') && ! op._node.parsed()) {
    out(error, msg_standard, "Bug in db add: link was not parsed!");
    return -1;
  }

  char code[] = "       ";
  code[0] = op._letter;

  if ((op._node.type() == 'f')
  && (static_cast<File&>(op._node).checksum()[0] == '\0')) {
    // Only missing the checksum
    if (op._get_checksum) {
      ts = 0;
    }
    // Copy data
    char* checksum = NULL;
    int compression;
    int rc = _d->data.write(op._node.path(), _d->owner->name(), &checksum,
      op._compression, &compression);
    if (rc >= 0) {
      if (rc > 0) {
        if (compression > 0) {
          code[2] = 'z';
        } else {
          code[2] = 'f';
        }
      }
      static_cast<File&>(op._node).setChecksum(checksum);
      if ((op._id >= 0) && (_d->missing[op._id] == checksum)) {
        // Mark checksum as recovered
        _d->missing.setRecovered(op._id);
      }
    } else {
      if ((op._letter != '!') || (! report_copy_error_once)) {
        char* full_name;
        asprintf(&full_name, "%s:%s", _d->owner->name(), op._path);
        out(error, msg_errno, "Backing up file", errno, full_name);
        free(full_name);
      }
      code[2] = '!';
      failed = true;
    }
    free(checksum);
  }

  // Even if failed, add data if new
  if (! failed || ! op._get_checksum) {
    // Add entry info to journal
    if (_d->owner->add(op._path, &op._node, ts) < 0) {
      out(error, msg_standard, "Cannot add to client's list");
      failed = true;
    }
  }
  out(info, msg_standard, op._path, -2, code);

  return failed ? -1 : 0;
}
