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

#include "strings.h"
#include "files.h"
#include "list.h"
#include "db.h"
#include "hbackup.h"

using namespace hbackup;

static bool cancel() {
  return terminating("write") != 0;
}

struct Database::Private {
  string                  path;
  int                     expire;
  list<string>            active_data;
  list<string>            expired_data;
  List*                   list;
  List*                   journal;
  List*                   merge;
  StrPath                 client;
  bool                    clientJournalled;
};

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

int Database::organise(const string& path, int number) {
  DIR           *directory;
  struct dirent *dir_entry;
  File          nofiles(path.c_str(), ".nofiles");
  int           failed   = 0;

  if (! isWriteable()) {
    return -1;
  }

  // Already organised?
  if (nofiles.isValid()) {
    return 0;
  }
  // Find out how many entries
  if ((directory = opendir(path.c_str())) == NULL) {
    return 1;
  }
  // Skip . and ..
  number += 2;
  while (((dir_entry = readdir(directory)) != NULL) && (number > 0)) {
    number--;
  }
  // Decide what to do
  if (number == 0) {
    rewinddir(directory);
    while ((dir_entry = readdir(directory)) != NULL) {
      // Ignore . and ..
      if (! strcmp(dir_entry->d_name, ".")
       || ! strcmp(dir_entry->d_name, "..")) {
        continue;
      }
      Node node(path.c_str(), dir_entry->d_name);
      string source_path = path + "/" + dir_entry->d_name;
      if (node.type() == '?') {
        cerr << "Error: organise: cannot get metadata: " << source_path
          << endl;
        failed = 2;
      } else
      if ((node.type() == 'd')
       // If we crashed, we might have some two-letter dirs already
       && (strlen(node.name()) != 2)
       // If we've reached the point where the dir is ??-?, stop!
       && (node.name()[2] != '-')) {
        // Create two-letter directory
        string dir_path = path + "/" + node.name()[0] + node.name()[1];
        if (Directory(dir_path.c_str()).create()) {
          failed = 2;
        } else {
          // Create destination path
          string dest_path = dir_path + "/" + &node.name()[2];
          // Move directory accross, changing its name
          if (rename(source_path.c_str(), dest_path.c_str())) {
            failed = 1;
          }
        }
      }
    }
    if (! failed) {
      nofiles.create();
    }
  }
  closedir(directory);
  return failed;
}

int Database::write(
    const string&   path,
    char**          dchecksum,
    int             compress) {
  if (! isWriteable()) {
    return -1;
  }

  int failed = 0;

  Stream source(path.c_str());
  if (source.open("r")) {
    cerr << strerror(errno) << ": " << path << endl;
    return -1;
  }

  // Temporary file to write to
  string temp_path = _d->path + "/filedata";
  Stream temp(temp_path.c_str());
  if (temp.open("w", compress)) {
    cerr << strerror(errno) << ": " << temp_path << endl;
    failed = -1;
  } else

  // Copy file locally
  if (temp.copy(source, cancel)) {
    cerr << strerror(errno) << ": " << path << endl;
    failed = -1;
  }

  source.close();
  temp.close();

  if (failed) {
    return failed;
  }

  // Get file final location
  string dest_path;
  if (getDir(source.checksum(), dest_path, true) == 2) {
    cerr << "Error: write: failed to get dir for: " << source.checksum()
      << endl;
    return -1;
  }

  // Make sure our checksum is unique: compare first bytes of files
  int  index = 0;
  bool missing = false;
  bool present = false;
  do {
    char* final_path = NULL;
    asprintf(&final_path, "%s-%u", dest_path.c_str(), index);

    // Check whether directory already exists
    if (Directory(final_path).isValid()) {
      Stream* try_file = new Stream(final_path, "data");

      if (try_file->isValid()) {
        try_file->open("r");
      } else {
        delete try_file;
        try_file = new Stream(final_path, "data.gz");
        if (! try_file->isValid()) {
          delete try_file;
          // Need to copy file accross: leave index to current value and exit
          break;
        }
        try_file->open("r", 1);
      }
      temp.open("r", compress);
      switch (temp.compare(*try_file, 1024)) {
        case 0:
          present = true;
          break;
        case 1:
          index++;
          break;
        default:
          failed = -1;
      }
      try_file->close();
      delete try_file;
      temp.close();
    } else {
      Directory(final_path).create();
      dest_path = final_path;
      missing = true;
    }
    free(final_path);
  } while ((failed == 0) && (! missing) && (! present));

  // Now move the file in its place
  if (! present && rename(temp_path.c_str(),
        (dest_path + "/data" + ((compress != 0) ? ".gz" : "")).c_str())) {
    cerr << "Failed to move file " << temp_path << " to "
      << dest_path << ": " << strerror(errno) << endl;
    failed = -1;
  } else
  if (! present && (verbosity() > 1)) {
    cout << "Adding " << ((compress != 0) ? "compressed " : "")
      << "file data to DB in: " << dest_path << endl;
  }

  // If anything failed, delete temporary file
  if (failed || present) {
    std::remove(temp_path.c_str());
  }

  // Report checksum
  *dchecksum = NULL;
  if (! failed) {
    asprintf(dchecksum, "%s-%d", source.checksum(), index);
  }

  // Make sure we won't exceed the file number limit
  if (! failed) {
    // dest_path is /path/to/checksum
    unsigned int pos = dest_path.rfind('/');

    if (pos != string::npos) {
      dest_path.erase(pos);
      // Now dest_path is /path/to
      organise(dest_path, 256);
    }
  }

  return failed;
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

int Database::crawl(Directory &dir, string path, bool check) const {
  if (dir.isValid() && ! dir.createList()) {
    bool no_files = false;
    list<Node*>::iterator i = dir.nodesList().begin();
    while (i != dir.nodesList().end()) {
      if (((*i)->type() == 'f') && (strcmp((*i)->name(), ".nofiles") == 0)) {
        no_files = true;
      }
      if ((*i)->type() == 'd') {
        if (no_files) {
          Directory *d = new Directory(**i);
          crawl(*d, path + (*i)->name(), check);
          delete d;
        } else {
          string checksum = path + (*i)->name();
          cout << " -> " << path << (*i)->name() << endl;
          if (File((*i)->path(), "data").isValid()) {
            if (check) {
              Stream f((*i)->path(), "data");
              int rc = f.open("r", 0);
              if (! rc) {
                rc = f.computeChecksum();
                if (! rc) {
                  if (strncmp(f.checksum(), checksum.c_str(),
                      strlen(f.checksum()))) {
                    cout << "File data corrupted for " << checksum << ", ";
                    if (f.remove()) {
                      cout << "NOT";
                    }
                    cout << "removed" << endl;
                  }
                }
                f.close();
              }
              if (rc) {
                cerr << strerror(errno) << ": " << checksum << endl;
              }
            }
          } else
          if (File((*i)->path(), "data.gz").isValid()) {
            if (check) {
              Stream f((*i)->path(), "data.gz");
              int rc = f.open("r", 1);
              if (! rc) {
                rc = f.computeChecksum();
                if (! rc) {
                  if (strncmp(f.checksum(), checksum.c_str(),
                      strlen(f.checksum()))) {
                    cout << "File data corrupted for " << checksum << ", ";
                    if (f.remove()) {
                      cout << "NOT";
                    }
                    cout << "removed" << endl;
                  }
                }
                f.close();
              }
              if (rc) {
                cerr << strerror(errno) << ": " << checksum << endl;
              }
            }
          } else
          {
            cerr << "File data missing for " << checksum << endl;
          }
        }
      }
      delete *i;
      i = dir.nodesList().erase(i);
    }
  } else {
    return -1;
  }
  return 0;
}

int Database::getDir(
    const string& checksum,
    string&       path,
    bool          create) {
  path = _d->path + "/data";
  int level = 0;

  // Two cases: either there are files, or a .nofiles file and directories
  do {
    // If we can find a .nofiles file, then go down one more directory
    if (File(path.c_str(), ".nofiles").isValid()) {
      path += "/" + checksum.substr(level, 2);
      level += 2;
      if (create && Directory(path.c_str()).create()) {
        return 1;
      }
    } else {
      break;
    }
  } while (true);
  // Return path
  path += "/" + checksum.substr(level);
  return ! Directory(path.c_str()).isValid();
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
    if (! failed) {
      if (update("list", true)) {
        cerr << "Error: cannot rename lists" << endl;
        failed = true;
      }
    }
  } else {
    cerr << strerror(errno) << ": failed to open temporary list" << endl;
    failed = true;
  }
  return failed ? -1 : 0;
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

int Database::open(bool read_only) {
  if (isOpen()) {
    cerr << "Error: DB already open!" << endl;
    return -1;
  }

  bool failed = false;

  if (! Directory(_d->path.c_str()).isValid()) {
    if (read_only) {
      cerr << "Error: given DB path does not exist: " << _d->path << endl;
      return 2;
    } else
    if (mkdir(_d->path.c_str(), 0755)) {
      cerr << "Error: cannot create DB base directory" << endl;
      return 2;
    }
  }
  // Try to take lock
  if (lock()) {
    errno = ENOLCK;
    return 2;
  }

  List list(_d->path.c_str(), "list");

  // Check DB dir
  if (! Directory((_d->path + "/data").c_str()).isValid()) {
    if (read_only) {
      cerr << "Error: given path does not contain a database: "
        << _d->path << endl;
      failed = true;
    } else
    if (Directory(_d->path.c_str(), "data").create()) {
      cerr << "Error: cannot create data directory" << endl;
      failed = true;
    } else
    if (list.open("w") || list.close()) {
      cerr << "Error: cannot create list file" << endl;
      failed = true;
    } else
    if (verbosity() > 0) {
      cout << "Database initialized" << endl;
    }
  } else {
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
  }

  // Open list
  if (! failed) {
    _d->list = new List(_d->path.c_str(), "list");
    if (_d->list->open("r")) {
      cerr << "Error: cannot open list" << endl;
      failed = true;
    } else
    if (_d->list->isOldVersion()) {
      cerr << "Error: list cannot be converted yet" << endl;
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
      cout << "Previous crash detected, attempting recovery" << endl;
      if (merge()) {
        cerr << "Error: cannot recover from previous crash" << endl;
        failed = true;
      }
      _d->journal->close();
      _d->list->close();

      if (! failed) {
        update("journal");
        // Re-open list
        if (_d->list->open("r")) {
          cerr << "Error: cannot re-open DB list" << endl;
          failed = true;
        }
      }
    }

    // Create journal (do not cache)
    if (! read_only && ! failed && (_d->journal->open("w", -1))) {
      cerr << "Error: cannot open DB journal" << endl;
      failed = true;
    }
  } else {
    _d->journal = NULL;
  }

  // Open merged list (can fail)
  if (! read_only && ! failed) {
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
    return 2;
  }

  // Setup some data
  _d->client = "";
  if (verbosity() > 1) {
    cout << " -> Database open" << endl;
  }
  return 0;
}

int Database::close() {
  bool failed = false;
  bool read_only = ! isWriteable();

  if (! isOpen()) {
    cerr << "Error: cannot close DB because not open!" << endl;
    return -1;
  }

  if (read_only) {
    // Close list
    _d->list->close();
  } else if (terminating()) {
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
            string path;
            if (verbosity() > 1) {
              cout << " -> Removing data for " << *i << ": ";
            }
            if (! getDir(*i, path)) {
              if (File(path.c_str(), "data").isValid()) {
                remove((path + "/data").c_str());
              } else
              if (File(path.c_str(), "data.gz").isValid()) {
                remove((path + "/data.gz").c_str());
              }
              int rc = remove(path.c_str());
              if (verbosity() > 1) {
                if (rc) {
                  cout << "FAILED!";
                } else {
                  cout << "done";
                }
              }
            } else {
              if (verbosity() > 1) {
                cout << "ALREADY GONE!";
              }
            }
            if (verbosity() > 1) {
              cout << endl;
            }
          }
          i = _d->expired_data.erase(i);
        }
      }
    }
    delete _d->journal;
    delete _d->merge;
  }

  // Delete list
  delete _d->list;

  // for isOpen and isWriteable
  _d->list    = NULL;
  _d->journal = NULL;
  _d->merge   = NULL;

  // Release lock
  unlock();
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
    StrPath ls_path;
    if (path != NULL) {
      ls_path = path;
      ls_path.noEndingSlash();
      blocks = ls_path.countBlocks('/');
    }

    // Skip to given client
    if (_d->list->search(client, "") != 2) {
      return -1;
    }

    char* db_path = NULL;
    while (_d->list->search(client, NULL) == 2) {
      if (_d->list->getEntry(NULL, NULL, &db_path, NULL, -2) <= 0) {
        cerr << "Error reading from list: " << strerror(errno) << endl;
        return -1;
      }
      StrPath db_path2 = db_path;
      if ((db_path2.size() >= ls_path.size())                 // Path no shorter
       && (db_path2.countBlocks('/') > blocks)                // Not less blocks
       && (ls_path.compare(db_path2, ls_path.size()) == 0)) { // Paths alike
        char* slash = strchr(&db_path[ls_path.size() + 1], '/');
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
  int     rc;
  int     len = strlen(path);
  bool    path_is_dir     = false;
  bool    path_is_not_dir = false;

  if (path[0] == '\0') {
    path_is_dir = true;
  }

  // Skip to given client FIXME use search to look for path and speed things up
  if (_d->list->search(client, "") != 2) {
    return -1;
  }

  // Restore relevant data
  while ((rc = _d->list->getEntry(&fts, &fclient, &fpath, &fnode, date)) > 0) {
    if (strcmp(fclient, client) != 0) {
      break;
    }
    if (path_is_dir) {
      if (pathCompare(fpath, path, len) > 0) {
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
        cmp = pathCompare(fpath, path);
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
        cmp = pathCompare(fpath, path, len);
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
      StrPath base = dest;
      base += fpath;
      if (! Directory(base.dirname().c_str()).isValid()) {
        string command = "mkdir -p ";
        command += base.dirname().c_str();
        if (system(command.c_str())) {
          cerr << base.dirname() << ": " << strerror(errno) << endl;
        }
      }
      if (verbosity() > 0) {
        cout << "U " << base << endl;
      }
      switch (fnode->type()) {
        case 'f': {
            File* f = (File*) fnode;
            if (read(base, f->checksum())) {
              cerr << "Failed to restore file: " << strerror(errno) << endl;
              failed = true;
            }
            if (chmod(base.c_str(), fnode->mode())) {
              cerr << "Failed to restore file permissions: "
                << strerror(errno) << endl;
            }
          } break;
        case 'd':
          if (mkdir(base.c_str(), fnode->mode())) {
            cerr << "Failed to restore dir: " << strerror(errno) << endl;
            failed = true;
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
    }
    if (path_is_not_dir) {
      break;
    }
  }
  return failed ? -1 : 0;
}

int Database::read(const string& path, const string& checksum) {
  if (! isOpen()) {
    return -1;
  }
  int     failed = 0;

  string source_path;
  if (getDir(checksum, source_path)) {
    cerr << "Error: read: failed to get dir for: " << checksum << endl;
    return 2;
  }

  // Open source
  bool decompress = false;
  source_path += "/data";
  if (! File(source_path.c_str()).isValid()) {
    source_path += ".gz";
    decompress = true;
  }
  Stream source(source_path.c_str());
  if (source.open("r", decompress ? 1 : 0)) {
    cerr << "Error: failed to open read source file: " << source_path << endl;
    return 2;
  }

  // Open temporary file to write to
  string temp_path = path + ".part";
  Stream temp(temp_path.c_str());
  if (temp.open("w")) {
    cerr << "Error: failed to open read dest file: " << temp_path << endl;
    failed = 2;
  } else

  // Copy file to temporary name (size not checked: checksum suffices)
  if (temp.copy(source)) {
    cerr << "Error: failed to read file: " << source_path << endl;
    failed = 2;
  }

  source.close();
  temp.close();

  if (! failed) {
    // Verify that checksums match before overwriting final destination
    if (strncmp(checksum.c_str(), temp.checksum(), strlen(temp.checksum()))) {
      cerr << "Error: read checksums don't match: " << checksum << " "
        << temp.checksum() << endl;
      failed = 2;
    } else

    // All done
    if (rename(temp_path.c_str(), path.c_str())) {
      cerr << "Error: failed to rename read file to " << strerror(errno)
        << ": " << path << endl;
      failed = 2;
    }
  }

  if (failed) {
    std::remove(temp_path.c_str());
  }

  return failed;
}

int Database::scan(const string& checksum, bool thorough) {
  if (! isWriteable()) {
    return -1;
  }

  if (checksum == "") {
    list<string> sums;
    char*       path   = NULL;
    Node*       node   = NULL;

    // Get list of checksums
    while (_d->list->getEntry(NULL, NULL, &path, &node) > 0) {
      if (node->type() == 'f') {
        File *f = (File*) node;
        if (f->checksum()[0] != '\0') {
          sums.push_back(f->checksum());
        }
      }
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
      scan(i->c_str());
      if (terminating()) {
        errno = EINTR;
        return -1;
      }
    }
  } else {
    size_t pos = checksum.rfind('-');
    if (pos == string::npos) {
      errno = EINVAL;
      return -1;
    }
    StrPath path;
    if (getDir(checksum, path)) {
      errno = EUCLEAN;
      return -1;
    }
    Stream filedata((path + "/data").c_str());
    if (! filedata.isValid()) {
//       cerr << "Error: data missing for " << checksum << endl;
      errno = ENOENT;
      return -1;
    }
    filedata.computeChecksum();
    if (checksum.substr(0, pos) != filedata.checksum()) {
      cerr << "Error: data corrupted for " << checksum << endl;
      errno = ENOEXEC;
      return -1;
    }
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
      cmp = pathCompare(db_path, remote_path);
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
        if (! write(string(node->path()), &checksum, compress)) {
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
