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
#include "data.h"
#include "missing.h"
#include "owner.h"
#include "db.h"

using namespace hbackup;

struct Database::Private {
  char*             path;
  int               mode;       // 0: closed, 1: r-o, 2: r/w, 3: r/w, quiet
  Data              data;
  Missing           missing;
  bool              load_missing;
  Owner*            client;
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

Database::Database(const char* path) {
  _d           = new Private;
  _d->path     = strdup(path);
  _d->mode     = 0;
  // Reset progress callback function
  _d->progress = NULL;
}

Database::~Database() {
  if (_d->mode > 0) {
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
      out(info, msg_standard, _d->path, -1, "Database initialized");
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

  // Reset client's data
  _d->client = NULL;
  // Do not load list of missing items for now
  _d->load_missing = false;

  if (read_only) {
    _d->mode = 1;
    out(verbose, msg_standard, "Database open in read-only mode");
  } else {
    _d->missing.open(Path(_d->path, "missing"));
    // Set mode to call openClient for checking only
    _d->mode = 3;
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
    _d->mode = 2;
    // Load list of missing items if/when required
    _d->load_missing = true;
    out(verbose, msg_standard, "Database open in read/write mode");
  }
  return 0;
}

int Database::close() {
  bool failed = false;

  if (_d->mode == 0) {
    out(alert, msg_standard, "Cannot close: DB not open!");
  } else {
    if (_d->client != NULL) {
      closeClient(true);
    }
    if (_d->mode > 1) {
      // Save list of missing items
      _d->missing.close();
      // Release lock
      unlock();
    }
  }
  out(verbose, msg_standard, "Database closed");
  _d->mode = 0;
  return failed ? -1 : 0;
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
  while ((rc = _d->client->getNextRecord(path, date, &db_path, &db_node)) >0) {
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
    const char*     path,
    time_t          date) {
  if (date < 0) {
    date = time(NULL) - date;
  }
  bool  failed = false;
  char* fpath  = NULL;
  Node* fnode  = NULL;
  int   rc;
  while ((rc = _d->client->getNextRecord(path, date, &fpath, &fnode)) > 0) {
    if (fnode == NULL) {
      continue;
    }
    bool this_failed = false;
    Path base(dest, fpath);
    Path dir = base.dirname();
    if (! Directory(dir).isValid()) {
      string command = "mkdir -p \"";
      command += dir;
      command += "\"";
      if (system(command.c_str())) {
        out(error, msg_errno, "Creating path", errno, dir);
      }
    }
    out(info, msg_standard, base, -2, "U");
    switch (fnode->type()) {
      case 'f': {
          File* f = (File*) fnode;
          if (f->checksum()[0] == '\0') {
            out(error, msg_standard, "Failed to restore file: data missing");
            this_failed = true;
          } else
          if (_d->data.read(base, f->checksum())) {
            out(error, msg_errno, "Restoring file", errno);
            this_failed = true;
          }
        } break;
      case 'd':
        if (mkdir(base, fnode->mode())) {
          out(error, msg_errno, "Restoring dir", errno);
          this_failed = true;
        }
        break;
      case 'l': {
          Link* l = (Link*) fnode;
          if (symlink(l->link(), base)) {
            out(error, msg_errno, "Restoring file", errno);
            this_failed = true;
          }
        } break;
      case 'p':
        if (mkfifo(base, fnode->mode())) {
          out(error, msg_errno, "Restoring pipe (FIFO)", errno);
          this_failed = true;
        }
        break;
      default:
        char type[2] = { fnode->type(), '\0' };
        out(error, msg_errno, "Type not supported", -1, type);
        this_failed = true;
    }
    // Report error and go on
    if (this_failed) {
      failed = true;
      continue;
    }
    // Nothing more to do for links
    if (fnode->type() == 'l') {
      continue;
    }
    // Restore modification time
    {
      struct utimbuf times = { -1, fnode->mtime() };
      if (utime(base, &times)) {
        out(error, msg_errno, "Restoring modification time");
        this_failed = true;
      }
    }
    // Restore permissions
    if (chmod(base, fnode->mode())) {
      out(error, msg_errno, "Restoring permissions");
      this_failed = true;
    }
    // Restore owner and group
    if (chown(base, fnode->uid(), fnode->gid())) {
      out(error, msg_errno, "Restoring owner/group");
      this_failed = true;
    }
    // Report error and go on
    if (this_failed) {
      failed = true;
    }
  }
  free(fpath);
  delete fnode;
  return (failed || (rc < 0)) ? -1 : 0;
}

int Database::scan(
    bool            rm_obsolete) {
  // Get checksums from list
  list<string> list_sums;
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
    client.getChecksums(list_sums);
    if (failed || aborting()) {
      break;
    }
  }
  if (failed || aborting()) {
    return -1;
  }
  list_sums.sort();
  list_sums.unique();
  {
    stringstream s;
    s << "Found " << list_sums.size() << " checksums";
    out(verbose, msg_standard, s.str().c_str());
  }

  // Get checksums from DB
  out(verbose, msg_standard, "Crawling through DB");
  list<string> data_sums;
  // Check surficially, remove empty dirs
  int rc = _d->data.crawl(&data_sums, false, true);
  if (aborting() || (rc < 0)) {
    return -1;
  }

  if (! list_sums.empty()) {
    out(debug, msg_standard, "Checksum(s) from list:");
    for (list<string>::iterator i = list_sums.begin(); i != list_sums.end();
        i++) {
      out(debug, msg_standard, i->c_str(), 1);
    }
  }
  if (! data_sums.empty()) {
    out(debug, msg_standard, "Checksum(s) with data:");
    for (list<string>::iterator i = data_sums.begin(); i != data_sums.end();
        i++) {
      out(debug, msg_standard, i->c_str(), 1);
    }
  }

  // Separate missing and obsolete checksums
  list<string>::iterator l = list_sums.begin();
  list<string>::iterator d = data_sums.begin();
  while ((d != data_sums.end()) || (l != list_sums.end())) {
    int cmp;
    if (d == data_sums.end()) {
      cmp = 1;
    } else
    if (l == list_sums.end()) {
      cmp = -1;
    } else
    {
      cmp = d->compare(*l);
    }
    if (cmp > 0) {
      _d->missing.push_back(*l);
      l++;
    } else
    if (cmp < 0) {
      // Keep checksum
      d++;
    } else
    {
      // Remove checksum
      d = data_sums.erase(d);
      l++;
    }
  }
  list_sums.clear();

  if (! data_sums.empty()) {
    out(info, msg_line_no, NULL, data_sums.size(), "Obsolete checksum(s)");
    for (list<string>::iterator i = data_sums.begin(); i != data_sums.end();
        i++) {
      if (i->size() != 0) {
        if (rm_obsolete) {
          out(verbose, msg_standard, (_d->data.remove(i->c_str()) == 0) ?
            "removed" : "FAILED", 1, i->c_str());
        } else {
          out(verbose, msg_standard, i->c_str(), 1);
        }
      }
    }
  }
  _d->missing.show();

  if (aborting()) {
    return -1;
  }
  return rc;
}

int Database::check(
    bool            remove) const {
  out(verbose, msg_standard, "Crawling through DB data");
  // Check thoroughly, remove corrupted data if told (but not empty dirs)
  return _d->data.crawl(NULL, true, remove);
}

int Database::openClient(
    const char*     client,
    time_t          expire) {
  if (_d->mode == 0) {
    out(alert, msg_standard, "Open DB before opening client!");
    return -1;
  }

  // Create owner for client
  _d->client = new Owner(_d->path, client,
    (expire <= 0) ? expire : (time(NULL) - expire));
  _d->client->setProgressCallback(_d->progress);

  // Read/write open
  if (_d->mode > 1) {
    if (_d->load_missing) {
      _d->missing.load();
      _d->load_missing = false;
    }
  }
  // Open
  if (_d->client->open(_d->mode == 1, _d->mode != 1, _d->mode == 3) < 0) {
    delete _d->client;
    _d->client = NULL;
    return -1;
  }
  // Report and set temporary data path
  switch (_d->mode) {
    case 1:
      out(verbose, msg_standard, _d->client->name(), -1, "Database open r-o");
      break;
    case 2:
      out(verbose, msg_standard, _d->client->name(), -1, "Database open r/w");
      // Set temp path inside client's directory
      _d->data.setTemp(Path(_d->client->path(), "data"));
      break;
    default:;
  }
  return 0;
}

int Database::closeClient(
    bool            abort) {
  bool failed = (_d->client->close(abort) < 0);
  if (_d->mode < 3) {
    out(verbose, msg_standard, _d->client->name(), -1, "Database closed");
  }
  delete _d->client;
  _d->client = NULL;
  return failed ? -1 : 0;
}

void Database::sendEntry(
    OpData&         op) {
  Node* db_node = NULL;
  int rc = _d->client->search(op._path, &db_node);

  // Compare entries
  if ((rc > 0) || (db_node == NULL)) {
    // New file
    op._letter = 'A';
  } else
  // Existing file: check for differences
  if (*db_node != op._node) {
    // Metadata differ
    if ((op._node.type() == 'f') && (db_node->type() == 'f')
    && (op._node.size() == db_node->size())
    && (op._node.mtime() == db_node->mtime())) {
      // If the file data is there, just add new metadata
      const char* node_checksum = static_cast<File*>(db_node)->checksum();
      if (node_checksum[0] == '\0') {
        // Checksum missing: retry
        op._get_checksum = true;
      } else {
        static_cast<File&>(op._node).setChecksum(node_checksum);
      }
      op._letter = '~';
    } else {
      // Do it all
      op._letter = 'M';
    }
  } else
  // Same metadata, hence same type...
  if (op._node.type() == 'f') {
    // Check that file data is present
    const char* node_checksum = static_cast<File*>(db_node)->checksum();
    if (node_checksum[0] == '\0') {
      // Checksum missing: retry
      op._get_checksum = true;
      op._letter = '!';
    } else {
      // Same checksum: look for checksum in missing list (binary search)
      op._id = _d->missing.search(node_checksum);
      if (op._id >= 0) {
        op._letter = 'R';
      }
    }
  } else
  if ((op._node.type() == 'l')
  && (strcmp(static_cast<const Link&>(op._node).link(),
      static_cast<Link*>(db_node)->link()) != 0)) {
    op._letter = 'L';
  }
  delete db_node;
}

int Database::add(
    const OpData&   op) {
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
    int rc = _d->data.write(op._node.path(), &checksum, op._compression);
    if (rc >= 0) {
      if (rc > 0) {
        if (op._compression > 0) {
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
      out(warning, msg_errno, "Backing up file", errno, op._path);
      code[2] = '!';
      failed = true;
    }
    free(checksum);
  }

  // Even if failed, add data if new
  if (! failed || ! op._get_checksum) {
    // Add entry info to journal
    if (_d->client->add(op._path, &op._node, ts) < 0) {
      out(error, msg_standard, "Cannot add to client's list");
      failed = true;
    }
  }
  out(info, msg_standard, op._path, -2, code);

  return failed ? -1 : 0;
}
