/*
     Copyright (C) 2006-2007  Herve Fache

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

typedef struct {
  string name;
  string prefix;
} names_convert_t;

int Database::convertList() {
  if (verbosity() > 0) {
    cout << "Converting database list" << endl;
  }
  list<string> prefixes;
  if (getRecords(prefixes)) {
    cerr << "Error: cannot convert: cannot get list of prefixes" << endl;
    return -1;
  }
  // Reset lists
  if (_d->list->close() || _d->list->open("r")) {
    cerr << "Error: cannot re-open DB list" << endl;
    return -1;
  }
  _d->merge = new List(_d->path.c_str(), "list.part");
  if (_d->merge->open("w")) {
    cerr << "Error: cannot open DB merge list" << endl;
    delete _d->merge;
    return -1;
  }
  list<names_convert_t> names;
  for (list<string>::iterator i = prefixes.begin(); i != prefixes.end(); i++) {
    size_t pos = i->find_last_of('/');
    if (pos == string::npos) {
      cerr << "Error: wrong syntax for prefix: " << *i << endl;
      return -1;
    }
    string name = i->substr(pos + 1);
    if (i->compare(0, 6, "smb://") == 0) {
      name += "$";
    } else {
      name += "#";
    }
    names_convert_t nc = { name, *i };
    // Insert in alphabetic order of name
    list<names_convert_t>::iterator i = names.begin();
    while (i != names.end()) {
      if (i->name == nc.name) {
        cerr << "Error: found two identical names: " << i->name << endl;
        return -1;
      } else
      if (i->name > nc.name) {
        break;
      }
      i++;
    }
    names.insert(i, nc);
  }
  if (verbosity() > 0) {
    cout << "Clients found: " << names.size() << endl;
  }
  string last_prefix;
  for (list<names_convert_t>::iterator i = names.begin(); i != names.end();
      i++) {
    if (verbosity() > 0) {
      cout << " " << i->prefix << " -> " << i->name << endl;
    }
    if (last_prefix > i->prefix) {
      if (_d->list->close() || _d->list->open("r")) {
        cerr << "Error: cannot re-open DB list" << endl;
        return -1;
      } else {
        if (verbosity() > 1) {
          cout << "DB list rewinded" << endl;
        }
      }
    } else {
      _d->list->keepLine();
    }
    last_prefix = i->prefix;
    // Find prefix
    if (_d->list->search(i->prefix.c_str(), "") != 2) {
      cerr << "Error: client not found" << endl;
      return -1;
    }
    // Copy till next prefix
    if (_d->merge->client(i->name.c_str())
     || _d->list->search(NULL, NULL, _d->merge) < 0) {
      cerr << "Error: copy failed" << endl;
      return -1;
    }
  }
  // Reset lists
  if (_d->list->close() || _d->merge->close() || update("list", true)
   || _d->list->open("r")) {
    cerr << "Error: cannot re-open DB list" << endl;
    return -1;
  }
  delete _d->merge;

  if (verbosity() > 0) {
    cout << "Conversion successful" << endl;
  }
  return 0;
}

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
        cerr << "Warning: lock reset" << endl;
        std::remove(lock_path.c_str());
      } else {
        cerr << "Error: lock taken by process with pid " << pid << endl;
        failed = true;
      }
    } else {
      cerr << "Error: lock taken by an unidentified process!" << endl;
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
      cerr << "Error: lock: cannot take lock" << endl;
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
  List merge(_d->path.c_str(), "list.part");
  if (! merge.open("w")) {
    if (merge.merge(*_d->list, *_d->journal) && (errno != EBUSY)) {
      cerr << "Error: merge failed" << endl;
      failed = true;
    }
    merge.close();
  } else {
    cerr << strerror(errno) << ": failed to open temporary list" << endl;
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

int Database::crawl(
    Directory&      dir,
    const string&   checksumPart,
    bool            thorough,
    list<string>&   checksums) const {
  return _d->data.crawl(dir, checksumPart, thorough, checksums);
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
    cerr << "Error: given DB path does not exist: " << _d->path << endl;
    return -1;
  }

  if (_d->data.open((_d->path + "/data").c_str())) {
    cerr << "Error: given DB path does not contain a database: " << _d->path
      << endl;
    return -1;
  }

  if (link((_d->path + "/list").c_str(), (_d->path + "/list.ro").c_str())) {
    cerr << "Error: could not create hard link to list" << endl;
    return -1;
  }

  bool failed = false;
  _d->list = new List(_d->path.c_str(), "list.ro");
  _d->journal = NULL;
  if (_d->list->open("r")) {
    cerr << "Error: cannot open list" << endl;
    failed = true;
  } else
  if (_d->list->isOldVersion()) {
    cerr << "Error: list in old format (run hbackup in backup mode to update)"
      << endl;
    failed = true;
  } else
  if (verbosity() > 1) {
    cout << " -> Database open in read-only mode" << endl;
  }
  if (failed) {
    delete _d->list;
    return -1;
  }
  return 0;
}

int Database::open_rw() {
  if (isOpen()) {
    cerr << "Error: DB already open!" << endl;
    return -1;
  }

  bool failed = false;

  if (! Directory(_d->path.c_str()).isValid()) {
    if (mkdir(_d->path.c_str(), 0755)) {
      cerr << "Error: cannot create DB base directory" << endl;
      return -1;
    }
  }
  // Try to take lock
  if (lock()) {
    errno = ENOLCK;
    return -1;
  }

  List list(_d->path.c_str(), "list");

  // Check DB dir
  switch (_d->data.open((_d->path + "/data").c_str(), true)) {
    case 0:
      // Open successful
      if (! list.isValid()) {
        Stream backup(_d->path.c_str(), "list~");

        cerr << "Error: list not accessible...";
        if (backup.isValid()) {
          cerr << "using backup" << endl;
          rename((_d->path + "/list~").c_str(), (_d->path + "/list").c_str());
        } else {
          cerr << "no backup accessible, aborting.";
          failed = true;
        }
      }
      break;
    case 1:
      // Creation successful
      if (list.open("w") || list.close()) {
        cerr << "Error: cannot create list file" << endl;
        failed = true;
      } else {
        cout << "Database initialized in " << _d->path << endl;
      }
      break;
    default:
      // Creation failed
      cerr << "Error: cannot create data directory" << endl;
      failed = true;
  }

  // Open list
  if (! failed) {
    _d->list = new List(_d->path.c_str(), "list");
    if (_d->list->open("r")) {
      cerr << "Error: cannot open list" << endl;
      failed = true;
    } else
    if (_d->list->isOldVersion() && (convertList() != 0)) {
      failed = true;
    }
  } else {
    _d->list = NULL;
  }

  // Deal with journal
  if (! failed) {
    _d->journal = new List(_d->path.c_str(), "journal");

    // Check previous crash
    if (! _d->journal->open("r")) {
      cout << "Backup was interrupted in a previous run, finishing up..." << endl;
      if (merge()) {
        cerr << "Error: failed to recover previous data" << endl;
        failed = true;
      }
      _d->journal->close();
      _d->list->close();

      if (! failed) {
        if (update("list", true)) {
          cerr << "Error: cannot rename lists" << endl;
          failed = true;
        } else {
          update("journal");
        }
        // Re-open list
        if (_d->list->open("r")) {
          cerr << "Error: cannot re-open DB list" << endl;
          failed = true;
        }
      }
    }

    // Create journal (do not cache)
    if (! failed && (_d->journal->open("w", -1))) {
      cerr << "Error: cannot open DB journal" << endl;
      failed = true;
    }
  } else {
    _d->journal = NULL;
  }

  // Open merged list (can fail)
  if (! failed) {
    _d->merge = new List(_d->path.c_str(), "list.part");
    if (_d->merge->open("w")) {
      cerr << "Error: cannot open DB merge list" << endl;
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
  if (verbosity() > 1) {
    cout << " -> Database open in read/write mode" << endl;
  }
  return 0;
}

int Database::close(int trash_expire) {
  bool failed = false;
  bool read_only = ! isWriteable();

  if (! isOpen()) {
    cerr << "Error: cannot close DB because not open!" << endl;
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
        if (verbosity() > 1) {
          cout << " -> Journal is empty, not merging" << endl;
        }
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
          cerr << "Error: cannot rename DB lists" << endl;
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
              if (verbosity() > 1) {
                cout << " -> Removing data for " << *i << ": ";
              }
              switch (_d->data.remove(*i)) {
                case 0:
                  if (verbosity() > 1) {
                    cout << "done";
                  }
                  break;
                case 1:
                  if (verbosity() > 1) {
                    cout << "ALREADY GONE!";
                  }
                  break;
                case 2:
                  if (verbosity() > 1) {
                    cout << "DATA GONE!";
                  }
                  break;
                default:
                  if (verbosity() > 1) {
                    cout << "FAILED!";
                  }
              }
              if (verbosity() > 1) {
                cout << endl;
              }
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

  if (verbosity() > 1) {
    cout << " -> Database closed" << endl;
  }
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
        cerr << "Error reading from list: " << strerror(errno) << endl;
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
        cerr << "Error reading from list: " << strerror(errno) << endl;
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
    cerr << "Not implemented" << endl;
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
          cerr << dir << ": " << strerror(errno) << endl;
        }
      }
      free(dir);
      if (verbosity() > 0) {
        cout << "U " << base.c_str() << endl;
      }
      bool set_permissions = false;
      switch (fnode->type()) {
        case 'f': {
            File* f = (File*) fnode;
            if (f->checksum()[0] == '\0') {
              cerr << "Failed to restore file: data missing" << endl;
            } else
            if (_d->data.read(base.c_str(), f->checksum())) {
              cerr << "Failed to restore file: " << strerror(errno) << endl;
              failed = true;
            } else
            {
              set_permissions = true;
            }
          } break;
        case 'd':
          if (mkdir(base.c_str(), fnode->mode())) {
            cerr << "Failed to restore dir: " << strerror(errno) << endl;
            failed = true;
          } else
          {
            set_permissions = true;
          }
          break;
        case 'l': {
            Link* l = (Link*) fnode;
            if (symlink(l->link(), base.c_str())) {
              cerr << "Failed to restore file: " << strerror(errno) << endl;
              failed = true;
            }
          } break;
        case 'p':
          if (mkfifo(base.c_str(), fnode->mode())) {
            cerr << "Failed to restore named pipe (FIFO): "
              << strerror(errno) << endl;
            failed = true;
          }
          break;
        default:
          cerr << "Type '" << fnode->type() << "' not supported" << endl;
      }
      if (set_permissions && chmod(base.c_str(), fnode->mode())) {
        cerr << "Failed to restore file permissions: " << strerror(errno)
          << endl;
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
    const string&   checksum) const {
  if (! isWriteable()) {
    return -1;
  }

  // Check entire database for missing/corrupted data
  if (checksum == "") {
    list<string> sums;
    Node*        node = NULL;

    // Get list of checksums
    while (_d->list->getEntry(NULL, NULL, NULL, &node) > 0) {
      if (node->type() == 'f') {
        File* f = (File*) node;
        if (f->checksum()[0] != '\0') {
          sums.push_back(f->checksum());
        }
      }
      delete node;
      node = NULL;
      if (terminating()) {
        errno = EINTR;
        return -1;
      }
    }
    sums.sort();
    sums.unique();
    if (verbosity() > 0) {
      cout << "Scanning database contents";
      if (thorough) {
        cout << " thoroughly";
      }
      cout << ": " << sums.size() << " file";
      if (sums.size() != 1) {
        cout << "s";
      }
      cout << endl;
    }
    for (list<string>::iterator i = sums.begin(); i != sums.end(); i++) {
      scan(thorough, i->c_str());
      if (terminating()) {
        errno = EINTR;
        return -1;
      }
    }
  } else {
    // Check given file data
    _d->data.check(checksum, thorough);
  }
  return 0;
}

void Database::setClient(
    const char*     client,
    time_t          expire) {
  // Finish previous client work (removed items at end of list)
  if (_d->client.length() != 0) {
    sendEntry(NULL, NULL, NULL);
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
    Node**          db_node) {
  if (! isWriteable()) {
    return -1;
  }

  // DB
  char* db_path = NULL;
  int   cmp     = 1;
  while (_d->list->search(_d->client.c_str(), NULL, _d->merge, _d->expire,
      &_d->active_data, &_d->expired_data) == 2) {
    if (_d->list->getEntry(NULL, NULL, &db_path, NULL, -2) <= 0) {
      return -1;
    }
    if (remote_path != NULL) {
      cmp = Path::compare(db_path, remote_path);
    } else {
      cmp = -1;
    }
    // If found or exceeded, break loop
    if (cmp >= 0) {
      if (cmp == 0) {
        // Get metadata
        _d->list->getLine();
        _d->list->getEntry(NULL, NULL, &db_path, db_node, -2);
        // Do not lose this metadata!
        _d->list->keepLine();
      }
      break;
    }
    // Not reached, mark 'removed' (getLineType keeps line automatically)
    _d->merge->path(db_path);
    if (_d->list->getLineType() != '-') {
      if (! _d->clientJournalled) {
        _d->journal->client(_d->client.c_str());
        _d->clientJournalled = true;
      }
      _d->journal->path(db_path);
      _d->journal->data(time(NULL));
      // Add path and 'removed' entry
      _d->merge->data(time(NULL));
      if (verbosity() > 0) {
        cout << "D " << db_path << endl;
      }
    }
  }
  free(db_path);
  // Send status back
  if (remote_path != NULL) {
    _d->merge->path(remote_path);
    if ((cmp > 0) || (*db_node == NULL)) {
      // Exceeded, keep line for later
      _d->list->keepLine();
      return 1;
    }
  }
  return 0;
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
    cerr << "Bug in db add: link was not parsed!" << endl;
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
      _d->journal->client(_d->client.c_str());
      _d->clientJournalled = true;
    }
    _d->journal->path(remote_path);
    _d->journal->data(ts, node2);
    _d->merge->data(ts, node2, true);
  }
  delete node2;

  return failed ? -1 : 0;
}
