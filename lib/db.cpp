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

#include <iostream>
#include <sstream>
#include <string>
#include <list>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>

using namespace std;

#include "files.h"
#include "list.h"
#include "data.h"
#include "db.h"
#include "hbackup.h"

using namespace hbackup;

struct Database::Private {
  string            path;
  Data              data;
  int               expire;
  list<string>      active_data;
  list<string>      expired_data;
  List*             list;
  List*             journal;
  List*             merge;
  Path              client;
  bool              clientJournalled;
};

int Database::lock() {
  string  lock_path;
  FILE    *file;
  bool    failed = false;

  // Set the database path that we just locked as default
  lock_path = _d->path + "/lock";

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
        out(warning) << "Lock reset" << endl;
        std::remove(lock_path.c_str());
      } else {
        out(error) << "Lock taken by process with pid " << pid << endl;
        failed = true;
      }
    } else {
      out(error) << "Lock taken by an unidentified process!" << endl;
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
      out(error) << "Cannot take lock" << endl;
      failed = true;
    }
  }
  if (failed) {
    return -1;
  }
  return 0;
}

void Database::unlock() {
  string lock_path = _d->path + "/lock";
  std::remove(lock_path.c_str());
}

int Database::merge() {
  bool failed = false;

  if (! isWriteable()) {
    return -1;
  }

  // Merge with existing list into new one
  List merge(Path(_d->path.c_str(), "list.part"));
  if (! merge.open("w")) {
    if (merge.merge(*_d->list, *_d->journal) && (errno != EBUSY)) {
      out(error) << "Merge failed" << endl;
      failed = true;
    }
    merge.close();
  } else {
    out(error) << strerror(errno) << ": failed to open temporary list" << endl;
    failed = true;
  }
  return failed ? -1 : 0;
}

int Database::update(
    string          name,
    bool            new_file) {
  // Simplify writings to avoid mistakes
  string path = _d->path + "/" + name;

  // Backup file
  int rc = rename(path.c_str(), (path + "~").c_str());
  // Put new version in place
  if (new_file && (rc == 0)) {
    rc = rename((path + ".part").c_str(), path.c_str());
  }
  return rc;
}

bool Database::isOpen() const {
  return (_d->list != NULL) && _d->list->isOpen();
}

bool Database::isWriteable() const {
  return (_d->journal != NULL) && _d->journal->isOpen();
}

Database::Database(const string& path) {
  _d          = new Private;
  _d->path    = path;
  _d->list    = NULL;
  _d->journal = NULL;
  _d->expire  = -1;
}

Database::~Database() {
  if (isOpen()) {
    close();
  }
  delete _d;
}

int Database::open_ro() {
  if (! Directory(_d->path.c_str()).isValid()) {
    out(error) << "Given DB path does not exist: " << _d->path << endl;
    return -1;
  }

  if (_d->data.open((_d->path + "/data").c_str())) {
    out(error) << "Given DB path does not contain a database: "
      << _d->path << endl;
    return -1;
  }

  if (link((_d->path + "/list").c_str(), (_d->path + "/list.ro").c_str())) {
    out(error) << "Could not create hard link to list" << endl;
    return -1;
  }

  bool failed = false;
  _d->list = new List(Path(_d->path.c_str(), "list.ro"));
  _d->journal = NULL;
  if (_d->list->open("r")) {
    out(error) << "Cannot open list" << endl;
    failed = true;
  } else
  if (_d->list->isOldVersion()) {
    out(error)
      << "list in old format (run hbackup in backup mode to update)"
      << endl;
    failed = true;
  } else
  out(verbose) << "Database open in read-only mode" << endl;
  if (failed) {
    delete _d->list;
    return -1;
  }
  return 0;
}

int Database::open_rw() {
  if (isOpen()) {
    out(error) << "DB already open!" << endl;
    return -1;
  }

  bool failed = false;

  if (! Directory(_d->path.c_str()).isValid()) {
    if (mkdir(_d->path.c_str(), 0755)) {
      out(error) << "Cannot create DB base directory" << endl;
      return -1;
    }
  }
  // Try to take lock
  if (lock()) {
    errno = ENOLCK;
    return -1;
  }

  List list(Path(_d->path.c_str(), "list"));

  // Check DB dir
  switch (_d->data.open((_d->path + "/data").c_str(), true)) {
    case 0:
      // Open successful
      if (! list.isValid()) {
        Stream backup(Path(_d->path.c_str(), "list~"));

        out(error) << "List not accessible...";
        if (backup.isValid()) {
          out(error, 0) << "using backup" << endl;
          rename((_d->path + "/list~").c_str(), (_d->path + "/list").c_str());
        } else {
          out(error, 0) << "no backup accessible, aborting." << endl;
          failed = true;
        }
      }
      break;
    case 1:
      // Creation successful
      if (list.open("w") || list.close()) {
        out(error) << "Cannot create list file" << endl;
        failed = true;
      } else {
        out(info) << "Database initialized in " << _d->path << endl;
      }
      break;
    default:
      // Creation failed
      out(error) << "Cannot create data directory" << endl;
      failed = true;
  }

  // Open list
  if (! failed) {
    _d->list = new List(Path(_d->path.c_str(), "list"));
    if (_d->list->open("r")) {
      out(error) << "Cannot open list" << endl;
      failed = true;
    } else
    if (_d->list->isOldVersion()) {
      failed = true;
    }
  } else {
    _d->list = NULL;
  }

  // Deal with journal
  if (! failed) {
    _d->journal = new List(Path(_d->path.c_str(), "journal"));

    // Check previous crash
    if (! _d->journal->open("r")) {
      out(warning)
        << "Backup was interrupted in a previous run, finishing up..." << endl;
      if (merge()) {
        out(error) << "Failed to recover previous data" << endl;
        failed = true;
      }
      _d->journal->close();
      _d->list->close();

      if (! failed) {
        if (update("list", true)) {
          out(error) << "Cannot rename lists" << endl;
          failed = true;
        } else {
          update("journal");
        }
        // Re-open list
        if (_d->list->open("r")) {
          out(error) << "Cannot re-open DB list" << endl;
          failed = true;
        }
      }
    }

    // Create journal (do not cache)
    if (! failed && (_d->journal->open("w", -1))) {
      out(error) << "Cannot open DB journal" << endl;
      failed = true;
    }
  } else {
    _d->journal = NULL;
  }

  // Open merged list (can fail)
  if (! failed) {
    _d->merge = new List(Path(_d->path.c_str(), "list.part"));
    if (_d->merge->open("w")) {
      out(error) << "Cannot open DB merge list" << endl;
      delete _d->merge;
      failed = true;
    }
  }

  if (failed) {
    if (_d->list != NULL) {
        // Close list
      _d->list->close();
      // Delete list
      delete _d->list;
    }
    if (_d->journal != NULL) {
        // Close journal
      _d->journal->close();
      // Delete journal
      delete _d->journal;
    }

    // for isOpen and isWriteable
    _d->list    = NULL;
    _d->journal = NULL;
    _d->merge   = NULL;

    // Unlock DB
    unlock();
    return -1;
  }

  // Setup some data
  _d->client = "";
  out(verbose) << "Database open in read/write mode" << endl;
  return 0;
}

int Database::close(int trash_expire) {
  bool failed = false;
  bool read_only = ! isWriteable();

  if (! isOpen()) {
    out(error) << "Cannot close DB because not open!" << endl;
    return -1;
  }

  if (read_only) {
    // Close list
    _d->list->close();
    unlink((_d->path + "/list.ro").c_str());
  } else {
    if (terminating()) {
      // Close list
      _d->list->close();
      // Close journal
      _d->journal->close();
      // Close and delete merge
      _d->merge->close();
      remove((_d->path + "/list.part").c_str());
    } else {
      // Finish off list reading/copy
      setClient("");
      // Close journal
      _d->journal->close();
      // See if any data was added
      if (_d->journal->isEmpty()) {
        // Do nothing
        out(verbose, 1) << "Journal is empty, not merging" << endl;
        // Close list
        _d->list->close();
        // Close merge
        _d->merge->close();
        remove((_d->path + "/list.part").c_str());
        remove((_d->path + "/journal").c_str());
      } else {
        // Finish off merging
        _d->list->search("", "", _d->merge, -1, &_d->active_data,
          &_d->expired_data);
        // Close list
        _d->list->close();
        // Close merge
        _d->merge->close();
        // File names ballet
        if (update("list", true)) {
          out(error) << "Cannot rename DB lists" << endl;
          failed = true;
        } else {
          // All merging successful
          update("journal");

          // Get rid of expired data
          _d->active_data.sort();
          _d->active_data.unique();
          _d->expired_data.sort();
          _d->expired_data.unique();

          // Get checksums for removal
          list<string>::iterator i = _d->expired_data.begin();
          list<string>::iterator j = _d->active_data.begin();
          while (i != _d->expired_data.end()) {
            while ((j != _d->active_data.end()) && (*j < *i)) { j++; }
            if ((j != _d->active_data.end()) && (*j == *i)) {
              i = _d->expired_data.erase(i);
            } else {
              i++;
            }
          }
          _d->active_data.clear();
          // Remove obsolete data
          i = _d->expired_data.begin();
          while (i != _d->expired_data.end()) {
            if (i->size() != 0) {
              out(verbose, 1) << "Removing data for " << *i << ": ";
              switch (_d->data.remove(*i)) {
                case 0:
                  out(verbose) << "done";
                  break;
                case 1:
                  out(verbose) << "ALREADY GONE!";
                  break;
                case 2:
                  out(verbose) << "DATA GONE!";
                  break;
                default:
                  out(verbose) << "FAILED!";
              }
              out(verbose) << endl;
            }
            i = _d->expired_data.erase(i);
          }
          _d->expired_data.clear();
        }
      }
    }
    // Delate lists
    delete _d->journal;
    delete _d->merge;

    // Release lock
    unlock();
  }
  // Delete list
  delete _d->list;

  // for isOpen and isWriteable
  _d->list    = NULL;
  _d->journal = NULL;
  _d->merge   = NULL;

  out(verbose) << "Database closed" << endl;
  return failed ? -1 : 0;
}

int Database::getRecords(
    list<string>&   records,
    const char*     client,
    const char*     path,
    time_t          date) {
  // Look for clientes
  if ((client == NULL) || (client[0] == '\0')) {
    while (_d->list->search() == 2) {
      if (terminating()) {
        return -1;
      }
      char *db_client = NULL;
      if (_d->list->getEntry(NULL, &db_client, NULL, NULL) < 0) {
        out(error) << strerror(errno) << ": reading from list" << endl;
        return -1;
      }
      records.push_back(db_client);
      free(db_client);
    }
    return 0;
  } else
  // Look for paths
  if (date == 0) {
    int blocks = 0;
    Path ls_path;
    if (path != NULL) {
      ls_path = path;
      ls_path.noTrailingSlashes();
      blocks = ls_path.countBlocks('/');
    }

    // Skip to given client
    if (_d->list->search(client, "") != 2) {
      return -1;
    }

    char* db_path = NULL;
    while (_d->list->search(client, NULL) == 2) {
      if (terminating()) {
        return -1;
      }
      if (_d->list->getEntry(NULL, NULL, &db_path, NULL, -2) <= 0) {
        out(error) << strerror(errno) << ": reading from list" << endl;
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
    return 0;
  } else
  // The rest still needs to see the light of day
  {
    out(error) << "Not implemented" << endl;
  }
  return -1;
}

int Database::restore(
    const char* dest,
    const char* client,
    const char* path,
    time_t      date) {
  bool    failed  = false;
  char*   fclient = NULL;
  char*   fpath   = NULL;
  Node*   fnode   = NULL;
  time_t  fts;
  int     len = strlen(path);
  bool    path_is_dir     = false;
  bool    path_is_not_dir = false;

  if ((len == 0) || (path[len - 1] == '/')) {
    path_is_dir = true;
  }

  // Skip to given client
  if (_d->list->search(client, "") != 2) {
    return -1;
  }

  // Restore relevant data
  while (_d->list->search(client, NULL) == 2) {
    if (terminating()) {
      failed = true;
      break;
    }
    if (_d->list->getEntry(&fts, &fclient, &fpath, &fnode, date) <= 0) {
      failed = true;
      break;
    }
    if (path_is_dir) {
      if (Path::compare(fpath, path, len) > 0) {
        break;
      }
    } else {
      int cmp;
      int flen = strlen(fpath);
      if (flen < len) {
        // Not even close
        continue;
      } else
      if (flen == len) {
        cmp = Path::compare(fpath, path);
        if (cmp < 0) {
          continue;
        } else
        if (cmp > 0) {
          break;
        } else
        // Perfect match
        if (fnode->type() == 'd') {
          path_is_dir = true;
        } else {
          path_is_not_dir = true;
        }
      } else
      if (fpath[len] == '/') {
        // Contained?
        cmp = Path::compare(fpath, path, len);
        if (cmp < 0) {
          continue;
        } else
        if (cmp > 0) {
          break;
        }
        // Yes indeed! Accept
        path_is_dir = true;
      } else {
        continue;
      }
    }
    if (fnode != NULL) {
      Path base(dest, fpath);
      char* dir = Path::dirname(base.c_str());
      if (! Directory(dir).isValid()) {
        string command = "mkdir -p ";
        command += dir;
        if (system(command.c_str())) {
          out(error) << strerror(errno) << ": " << dir << endl;
        }
      }
      free(dir);
      out(info) << "U " << base.c_str() << endl;
      bool set_permissions = false;
      switch (fnode->type()) {
        case 'f': {
            File* f = (File*) fnode;
            if (f->checksum()[0] == '\0') {
              out(error) << "Failed to restore file: data missing" << endl;
            } else
            if (_d->data.read(base.c_str(), f->checksum())) {
              out(error) << "Failed to restore file: " << strerror(errno)
                << endl;
              failed = true;
            } else
            {
              set_permissions = true;
            }
          } break;
        case 'd':
          if (mkdir(base.c_str(), fnode->mode())) {
            out(error) << strerror(errno) << ": restoring dir" << endl;
            failed = true;
          } else
          {
            set_permissions = true;
          }
          break;
        case 'l': {
            Link* l = (Link*) fnode;
            if (symlink(l->link(), base.c_str())) {
              out(error) << strerror(errno) << ": restoring file" << endl;
              failed = true;
            }
          } break;
        case 'p':
          if (mkfifo(base.c_str(), fnode->mode())) {
            out(error) << strerror(errno) << ": restoring pipe (FIFO)" << endl;
            failed = true;
          }
          break;
        default:
          out(error) << "Type '" << fnode->type() << "' not supported" << endl;
      }
      if (set_permissions && chmod(base.c_str(), fnode->mode())) {
        out(error) << strerror(errno) << ": restoring permissions" << endl;
      }
    }
    if (path_is_not_dir) {
      break;
    }
  }
  return failed ? -1 : 0;
}

int Database::scan(
    bool            thorough,
    bool            rm_corrupt) const {
  // Get checksums from list
  out(verbose) << "Reading list" << endl;
  list<string> list_sums;
  Node*        node = NULL;
  while (_d->list->getEntry(NULL, NULL, NULL, &node) > 0) {
    if (node->type() == 'f') {
      File* f = (File*) node;
      if (f->checksum()[0] != '\0') {
        list_sums.push_back(f->checksum());
      }
    }
    if (terminating()) {
      break;
    }
  }
  delete node;
  if (terminating()) {
    return -1;
  }
  list_sums.sort();
  list_sums.unique();
  out(verbose) << "Found " << list_sums.size() << " checksums" << endl;

  // Get checksums from DB
  out(verbose) << "Crawling through DB" << endl;
  list<string> data_sums;
  int rc = _d->data.crawl(data_sums, thorough, rm_corrupt);
  if (terminating() || (rc < 0)) {
    return -1;
  }
  data_sums.sort();
  data_sums.unique();
  out(verbose) << "Found " << data_sums.size() << " valid checksums" << endl;

  // Separate missing and obsolete checksums
  list<string> obsolete;
  list<string> missing;
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
      missing.push_back(*l);
      l++;
    } else
    if (cmp < 0) {
      obsolete.push_back(*d);
      d++;
    } else
    {
      d++;
      l++;
    }
  }

  if (! list_sums.empty()) {
    out(debug) << "Checksum(s) from list:" << endl;
    for (list<string>::iterator i = list_sums.begin(); i != list_sums.end(); i++) {
      out(debug, 1) << *i << endl;
    }
    list_sums.clear();
  }
  if (! data_sums.empty()) {
    out(debug) << "Checksum(s) with data:" << endl;
    for (list<string>::iterator i = data_sums.begin(); i != data_sums.end(); i++) {
      out(debug, 1) << *i << endl;
    }
    data_sums.clear();
  }
  if (! obsolete.empty()) {
    out(info) << "Removing obsolete checksum(s)" << endl;
    for (list<string>::iterator i = obsolete.begin(); i != obsolete.end();
        i++) {
      if (i->size() != 0) {
        out(verbose, 1) << *i << ": "
          << ((_d->data.remove(*i) == 0) ? "done" : "FAILED") << endl;
      }
    }
  }
  if (! missing.empty()) {
    out(warning) << "Missing checksum(s):" << endl;
    for (list<string>::iterator i = missing.begin(); i != missing.end(); i++) {
      out(warning, 1) << *i << endl;
    }
  }

  if (terminating()) {
    return -1;
  }
  return rc;
}

void Database::setClient(
    const char*     client,
    time_t          expire) {
  // Finish previous client work (removed items at end of list)
  if (_d->client.length() != 0) {
    sendEntry(NULL, NULL);
  }
  _d->client = client;
  if (expire > 0) {
    _d->expire = time(NULL) - expire;
  } else {
    _d->expire = expire;
  }
  _d->clientJournalled = false;
  // This will add the client if not found, copy it if found
  _d->list->search(client, "", _d->merge, -1, &_d->active_data,
    &_d->expired_data);
}

void Database::failedClient() {
  // Skip to next client/end of file
  _d->list->search(NULL, NULL, _d->merge, -1, &_d->active_data,
    &_d->expired_data);
  // Keep client found
  _d->list->keepLine();
}

int Database::sendEntry(
    const char*     remote_path,
    const Node*     node,
    char**          checksum) {
  if (! isWriteable()) {
    return -1;
  }

  // DB
  char* db_path = NULL;
  Node* db_node = NULL;
  int   cmp     = 1;
  while (_d->list->search(_d->client.c_str(), NULL, _d->merge, _d->expire,
      &_d->active_data, &_d->expired_data) == 2) {
    if (_d->list->getEntry(NULL, NULL, &db_path, NULL, -2) <= 0) {
      // Error
      out(error) << "Failed to get entry" << endl;
      return -1;
    }
    if (remote_path != NULL) {
      cmp = Path::compare(db_path, remote_path);
    } else {
      // Want to get all paths
      cmp = -1;
    }
    // If found or exceeded, break loop
    if (cmp >= 0) {
      if (cmp == 0) {
        // Get metadata
        _d->list->getEntry(NULL, NULL, &db_path, &db_node, -1);
        // Do not lose this metadata!
        _d->list->keepLine();
      }
      break;
    }
    // Not reached, mark 'removed' (getLineType keeps line automatically)
    _d->merge->addPath(db_path);
    if (_d->list->getLineType() != '-') {
      if (! _d->clientJournalled) {
        _d->journal->addClient(_d->client.c_str());
        _d->clientJournalled = true;
      }
      _d->journal->addPath(db_path);
      _d->journal->addData(time(NULL));
      // Add path and 'removed' entry
      _d->merge->addData(time(NULL));
      out(info) << "D " << db_path << endl;
    }
  }
  free(db_path);

  // Compare entries
  bool add_node = false;
  if (remote_path != NULL) {
    _d->merge->addPath(remote_path);
    if ((cmp > 0) || (db_node == NULL)) {
      // Exceeded, keep line for later
      _d->list->keepLine();
      // New file
      out(info) << "A";
      add_node = true;
    }

    if (! add_node) {
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
          out(info) << "~";
        } else {
          // Do it all
          out(info) << "M";
        }
        add_node = true;
      } else
      // Same metadata, hence same type...
      {
        // Compare linked data
        if ((node->type() == 'l')
        && (strcmp(((Link*)node)->link(), ((Link*)db_node)->link()) != 0)) {
          out(info) << "L";
          add_node = true;
        } else
        // Check that file data is present
        if ((node->type() == 'f')
          && (((File*)db_node)->checksum()[0] == '\0')) {
          // Checksum missing: retry
          out(info) << "!";
          *checksum = strdup("");
          add_node = true;
        }
      }
    }

    if (add_node) {
      out(info) << " " << remote_path;
      if (node->type() == 'f') {
        out(info) << " (";
        if (node->size() < 10000) {
          out(info) << node->size();
          out(info) << " ";
        } else
        if (node->size() < 10000000) {
          out(info) << node->size() / 1000;
          out(info) << " k";
        } else
        if (node->size() < 10000000000ll) {
          out(info) << node->size() / 1000000;
          out(info) << " M";
        } else
        {
          out(info) << node->size() / 1000000000;
          out(info) << " G";
        }
        out(info) << "B)";
      }
      out(info) << endl;
    }
  }
  delete db_node;

  // Send status back
  return add_node ? 1 : 0;
}

int Database::add(
    const char*     remote_path,
    const Node*     node,
    const char*     old_checksum,
    int             compress) {
  if (! isWriteable()) {
    return -1;
  }
  bool failed = false;

  // Add new record to active list
  if ((node->type() == 'l') && ! node->parsed()) {
    out(error) << "Bug in db add: link was not parsed!" << endl;
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
        if (! _d->data.write(string(node->path()), &checksum, compress)) {
          ((File*)node2)->setChecksum(checksum);
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
    if (! _d->clientJournalled) {
      _d->journal->addClient(_d->client.c_str());
      _d->clientJournalled = true;
    }
    _d->journal->addPath(remote_path);
    _d->journal->addData(ts, node2);
    _d->merge->addData(ts, node2, true);
  }
  delete node2;

  return failed ? -1 : 0;
}
