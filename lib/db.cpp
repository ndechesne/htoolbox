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
#include "list.h"
#include "owner.h"
#include "db.h"

using namespace hbackup;

struct Database::Private {
  char*             path;
  int               mode;       // 0: closed, 1: r-o, 2: r/w, 3: r/w, quiet
  Data              data;
  vector<string>    missing;
  int               missing_id; // -2: not loaded, -3: do not load
  Owner*            client;
  progress_f        progress;
};

int Database::lock() {
  Path  lock_path(_d->path, "lock");
  FILE* file;
  bool  failed = false;

  // Set the database path that we just locked as default
  // Try to open lock file for reading: check who's holding the lock
  if ((file = fopen(lock_path.c_str(), "r")) != NULL) {
    pid_t pid = 0;

    // Lock already taken
    fscanf(file, "%d", &pid);
    fclose(file);
    if (pid != 0) {
      // Find out whether process is still running, if not, reset lock
      kill(pid, 0);
      if (errno == ESRCH) {
        out(warning, msg_standard, "Lock reset");
        std::remove(lock_path.c_str());
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
    if ((file = fopen(lock_path.c_str(), "w")) != NULL) {
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

int Database::update(
    const char*     name,
    bool            new_file) const {
  // Simplify writings to avoid mistakes
  Path path(_d->path, name);

  // Backup file
  int rc = rename(path.c_str(), (path.str() + "~").c_str());
  // Put new version in place
  if (new_file && ((rc == 0) || (errno == ENOENT))) {
    rc = rename((path.str() + ".part").c_str(), path.c_str());
  }
  return rc;
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
  switch (_d->data.open(Path(_d->path, ".data").c_str(), initialize)) {
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
  _d->missing_id = -3;

  if (read_only) {
    _d->mode = 1;
    out(verbose, msg_standard, "Database open in read-only mode");
  } else {
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
    _d->missing_id = -2;
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
      if ((_d->missing_id > -2) && ! aborting()) {
        Stream missing_list(Path(_d->path, "missing.part"));
        if (! missing_list.open("w")) {
          int rc    = 0;
          int count = 0;
          for (unsigned int i = 0; i < _d->missing.size(); i++) {
            if (_d->missing[i][_d->missing[i].size() - 1] != '+') {
              rc = missing_list.putLine(_d->missing[i].c_str());
              count++;
            }
            if (rc < 0) break;
          }
          _d->missing.clear();
          if (count > 0) {
            stringstream s;
            s << "List of missing checksums contains " << count << " item(s)";
            out(info, msg_standard, s.str().c_str());
          }
          if (rc >= 0) {
            rc = missing_list.close();
          }
          if (rc >= 0) {
            update("missing", true);
          }
        } else {
          out(error, msg_errno, "Opening missing checksums list", errno);
        }
      }
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
  int blocks = 0;
  Path ls_path;
  if (path != NULL) {
    ls_path = path;
    ls_path.noTrailingSlashes();
    blocks = ls_path.countBlocks('/');
  }
  if (date < 0) {
    date = time(NULL) + date;
  }
  char* db_path = NULL;
  int   rc;
  while ((rc = _d->client->getNextRecord(path, date, &db_path)) > 0) {
    if (aborting()) {
      return -1;
    }
    Path db_path2 = db_path;
    if ((db_path2.length() >= ls_path.length())   // Path no shorter
      && (db_path2.countBlocks('/') > blocks)      // Not less blocks
      && (ls_path.compare(db_path2.c_str(), ls_path.length()) == 0)) {
      char* slash = strchr(&db_path[ls_path.length() + 1], '/');
      if (slash != NULL) {
        *slash = '\0';
      }

      if ((records.size() == 0)
        || (strcmp(records.back().c_str(), db_path) != 0)) {
        records.push_back(db_path);
      }
    }
  }
  free(db_path);
  return (rc < 0) ? -1 : 0;
}

int Database::restore(
    const char*     dest,
    const char*     path,
    time_t          date) {
  if (date < 0) {
    date = time(NULL) - date;
  }
  bool  failed  = false;
  char* fpath   = NULL;
  Node* fnode   = NULL;
  int   rc;
  while ((rc = _d->client->getNextRecord(path, date, &fpath, &fnode)) > 0) {
    if (fnode == NULL) {
      continue;
    }
    bool this_failed = false;
    Path base(dest, fpath);
    char* dir = Path::dirname(base.c_str());
    if (! Directory(dir).isValid()) {
      string command = "mkdir -p ";
      command += dir;
      if (system(command.c_str())) {
        out(error, msg_errno, "Creating path", errno, dir);
      }
    }
    free(dir);
    out(info, msg_standard, base.c_str(), -2, "U");
    switch (fnode->type()) {
      case 'f': {
          File* f = (File*) fnode;
          if (f->checksum()[0] == '\0') {
            out(error, msg_standard, "Failed to restore file: data missing");
            this_failed = true;
          } else
          if (_d->data.read(base.c_str(), f->checksum())) {
            out(error, msg_errno, "Restoring file", errno);
            this_failed = true;
          }
        } break;
      case 'd':
        if (mkdir(base.c_str(), fnode->mode())) {
          out(error, msg_errno, "Restoring dir", errno);
          this_failed = true;
        }
        break;
      case 'l': {
          Link* l = (Link*) fnode;
          if (symlink(l->link(), base.c_str())) {
            out(error, msg_errno, "Restoring file", errno);
            this_failed = true;
          }
        } break;
      case 'p':
        if (mkfifo(base.c_str(), fnode->mode())) {
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
      if (utime(base.c_str(), &times)) {
        out(error, msg_errno, "Restoring modification time");
        this_failed = true;
      }
    }
    // Restore permissions
    if (chmod(base.c_str(), fnode->mode())) {
      out(error, msg_errno, "Restoring permissions");
      this_failed = true;
    }
    // Restore owner and group
    if (chown(base.c_str(), fnode->uid(), fnode->gid())) {
      out(error, msg_errno, "Restoring owner/group");
      this_failed = true;
    }
    // Report error and go on
    if (this_failed) {
      failed = true;
    }
  }
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
  _d->missing_id = -3;
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
  if (! _d->missing.empty()) {
    stringstream s;
    s << "Missing checksum(s): " << _d->missing.size();
    out(warning, msg_standard, s.str().c_str());
    for (unsigned int i = 0; i < _d->missing.size(); i++) {
      out(verbose, msg_standard, _d->missing[i].c_str(), 1);
    }
  }

  if (aborting()) {
    return -1;
  }
  // Make sure we save the list
  _d->missing_id = -1;
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
    if (_d->missing_id == -2) {
      // Read list of missing checksums (it is ordered and contains no dup)
      out(verbose, msg_standard, "Reading list of missing checksums");
      Stream missing_list(Path(_d->path, "missing"));
      if (! missing_list.open("r")) {
        string checksum;
        while (missing_list.getLine(checksum) > 0) {
          _d->missing.push_back(checksum);
          out(debug, msg_standard, checksum.c_str(), 1);
        }
        missing_list.close();
      } else {
        out(warning, msg_errno, "Opening" , errno, "Missing checksums list");
      }
      _d->missing_id = -1;
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
      _d->data.setTemp(Path(_d->client->path(), "data").c_str());
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

int Database::sendEntry(
    const char*     remote_path,
    const Node*     node,
    char**          checksum) {
  // Reset ID of missing checksum
  _d->missing_id = -1;

  Node* db_node = NULL;
  int rc = _d->client->search(remote_path, &db_node);

  // Compare entries
  char letter = 0;
  bool new_path = false;
  if (remote_path != NULL) {
    if ((rc > 0) || (db_node == NULL)) {
      // New file
      letter = 'A';
      new_path = true;
    }

    if (letter == 0) {
      // Existing file: check for differences
      if (*db_node != *node) {
        // Metadata differ
        if ((node->type() == 'f')
        && (db_node->type() == 'f')
        && (node->size() == db_node->size())
        && (node->mtime() == db_node->mtime())) {
          // If the file data is there, just add new metadata
          // If the checksum is missing, this shall retry too
          *checksum = strdup(((File*)db_node)->checksum());
          letter = '~';
        } else {
          // Do it all
          letter = 'M';
        }
      } else
      // Same metadata, hence same type...
      {
        // Compare linked data
        if ((node->type() == 'l')
        && (strcmp(((Link*)node)->link(), ((Link*)db_node)->link()) != 0)) {
          letter = 'L';
        } else
        // Check that file data is present
        if (node->type() == 'f') {
          const char* node_checksum = ((File*)db_node)->checksum();
          if (node_checksum[0] == '\0') {
            // Checksum missing: retry
            *checksum = strdup("");
            letter = '!';
          } else {
            // Same checksum: check in missing list!
            if (! _d->missing.empty()) {
              // Look for checksum in missing list (binary search)
              int start = 0;
              int end   = _d->missing.size() - 1;
              int middle;
              while (start <= end) {
                middle = (end + start) / 2;
                int cmp = _d->missing[middle].compare(node_checksum);
                if (cmp > 0) {
                  end   = middle - 1;
                } else
                if (cmp < 0) {
                  start = middle + 1;
                } else
                {
                  _d->missing_id = middle;
                  break;
                }
              }
              if (_d->missing_id >= 0) {
                letter = 'R';
              }
            }
          }
        }
      }
    }

    if (letter != 0) {
      stringstream s;
      s << remote_path;
      long long size = -1;
      if (node->type() == 'f') {
        size = node->size();
        s << "\t(";
        if (node->size() < 10000) {
          s << node->size();
          s << " ";
        } else
        if (node->size() < 10000000) {
          s << node->size() / 1000;
          s << " k";
        } else
        if (node->size() < 10000000000ll) {
          s << node->size() / 1000000;
          s << " M";
        } else
        {
          s << node->size() / 1000000000;
          s << " G";
        }
        s << "B)";
      }
      char cletter[2] = { letter, '\0' };
      out(info, msg_standard, s.str().c_str(), -2, cletter);
    }
  }
  delete db_node;

  // Send status back
  return new_path ? 2 : ((letter != 0) ? 1 : 0);
}

int Database::add(
    const char*     remote_path,
    const Node*     node,
    const char*     old_checksum,
    int             compress) {
  bool failed = false;

  // Add new record to active list
  if ((node->type() == 'l') && ! node->parsed()) {
    out(error, msg_standard, "Bug in db add: link was not parsed!");
    return -1;
  }

  // Create data
  Node* node2 = NULL;
  switch (node->type()) {
    case 'l':
      node2 = new Link(*(Link*)node);
      break;
    case 'f':
      node2 = new File(*node);
      if ((old_checksum != NULL) && (old_checksum[0] != '\0')) {
        // Use same checksum
        ((File*)node2)->setChecksum(old_checksum);
      } else {
        // Copy data
        char* checksum  = NULL;
        if (! _d->data.write(node->path(), &checksum, compress)) {
          ((File*)node2)->setChecksum(checksum);
          if ((_d->missing_id >= 0)
          && (_d->missing[_d->missing_id] == checksum)) {
            out(verbose, msg_standard, checksum, -1, "Recovered checksum");
            // Mark checksum as already recovered
            _d->missing[_d->missing_id] += "+";
          }
          free(checksum);
        } else {
          failed = true;
        }
      }
      break;
    default:
      node2 = new Node(*node);
  }

  if (! failed || (old_checksum == NULL)) {
    time_t ts;
    if ((old_checksum != NULL) && (old_checksum[0] == '\0')) {
      ts = 0;
    } else {
      ts = time(NULL);
    }
    // Add entry info to journal
    if (_d->client->add(remote_path, node2, ts) < 0) {
      out(error, msg_standard, "Cannot add to client's list");
      failed = true;
    }
  }
  delete node2;

  return failed ? -1 : 0;
}
