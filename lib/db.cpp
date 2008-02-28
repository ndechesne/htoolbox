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
#include <iomanip>
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

static void progress(long long previous, long long current, long long total) {
  if (current < total) {
    out(verbose) << "Done: " << setw(5) << setiosflags(ios::fixed)
      << setprecision(1) << 100.0 * current /total << "%\r" << flush;
  } else if (previous != 0) {
    out(verbose) << "            \r";
  }
}

struct Database::Private {
  char*             path;
  int               mode;       // 0: closed, 1: r-o, 2: r/w, 3: r/w, quiet
  Data              data;
  vector<string>    missing;
  int               missing_id; // -2: not loaded, -3: do not load
  char*             client;
  List*             client_list;
  List*             client_journal;
  List*             client_part;
  int               client_expire;
  bool              client_journalled;
};

int Database::convertList(list<string>* clients) {
  out(info) << "Converting database list" << endl;
  Stream source(Path(_d->path, "list"));
  if (source.open("r")) {
    out(error) << strerror(errno) << "opening source file" << endl;
    return -1;
  }
  int     rc;
  bool    begin = true;
  string  line;
  string last_client;
  Stream* dest = NULL;
  while ((rc = source.getLine(line)) > 0) {
    if (line[0] == '#') {
      if (begin) {
        begin = false;
      } else {
        if (dest != NULL) {
          dest->putLine("# end");
          dest->close();
          delete dest;
        }
        break;
      }
    } else
    if (line[0] != '\t') {
      string client = line.substr(0, line.size() - 1);
      if (last_client == client) {
        client += ":1";
      }
      last_client = client;
      if (dest != NULL) {
        dest->putLine("# end");
        dest->close();
        delete dest;
      }
      dest = new Stream(Path(_d->path, (client + ".list").c_str()));
      if (dest->open("w") < 0) {
        out(alert) << strerror(errno) << "opening destination file" << endl;
        break;
      }
      dest->putLine("# version 3");
      dest->putLine(client.c_str());
      if (clients != NULL) {
        clients->push_back(client);
      }
    } else if (dest != NULL) {
      dest->putLine(line.c_str());
    } else {
      out(alert) << "No destination file!" << endl;
    }
  }
  source.close();

  out(info) << "Conversion successful" << endl;
  return 0;
}

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
  File(Path(_d->path, "lock")).remove();
}

// The procedure is as follows:
// ---> step 0: backup running
//    list      -> list.part  (copy, always discarded)
//              -> journal    (create, used for recovery)
// ---> step 1: backup finished
//    list.part -> list.next  (rename, journal will now be discarded)
// ---> step 2
//    journal   ->            (delete, discard journal)
// ---> step 3
//    list      -> list~      (rename, overwrite backup)
// ---> step 4
//    list.next -> list       (rename, process finished)
// ---> list ballet finished
//
// This allows for a safe recovery:
//    if list.next:
//      discard journal
//      goto ---> step 3
//    if journal:
//      merge list + journal -> list.part
//      goto ---> step 1
static int finishOff(
    List&           main,
    List&           journal,
    List&           part,
    bool            recovery) {
  string name = main.path();
  File next(Path((name + ".next").c_str()));

  // All files are expected to be closed
  bool got_next = recovery && next.isValid();

  // Recovering from list and journal (step 0)
  if (recovery && ! got_next) {
    if (journal.isValid()) {
      // Let's recover what we got
      if (! part.open("w")) {
        bool failed = false;
        main.open("r");
        journal.open("r");
        if (part.merge(main, journal) < 0) {
          failed = true;
        }
        main.close();
        journal.close();
        part.close();
        if (failed) {
          return -1;
        }
      } else {
        return -1;
      }
    } else {
      // Nothing to do (no list.next => list exists)
      return 0;
    }
  }

  // list.part -> list.next (step 1)
  if (! got_next && rename(part.path(), next.path())) {
    return -1;
  }

  // Discard journal (step 2)
  if (! got_next || File(journal).isValid()) {
    string backup = journal.path();
    backup += "~";
    if (rename(journal.path(), backup.c_str())) {
      return -1;
    }
  }

  // list -> list~ (step 3)
  if (! got_next || File(main).isValid()) {
    if (rename(main.path(), (name + "~").c_str())) {
      return -1;
    }
  }

  // list.next -> list (step 4)
  if (rename(next.path(), main.path())) {
    return -1;
  }
  return 0;
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
  _d                  = new Private;
  _d->path            = strdup(path);
  _d->mode            = 0;
  _d->client_list     = NULL;
  _d->client_journal  = NULL;
  _d->client_part     = NULL;
  _d->client_expire   = -1;
}

Database::~Database() {
  if (_d->mode > 0) {
    close();
  }
  free(_d->path);
  delete _d;
}

int Database::open(
    bool            read_only,
    bool            initialize) {

  if (read_only && initialize) {
    out(error) << "Cannot initialize in read-only mode" << endl;
    return -1;
  }

  if (! Directory(_d->path).isValid()) {
    if (read_only || ! initialize) {
      out(error) << "Given DB path does not exist: " << _d->path << endl;
      return -1;
    } else
    if (mkdir(_d->path, 0755)) {
      out(error) << "Cannot create DB base directory" << endl;
      return -1;
    }
  }
  // Try to take lock
  if (! read_only && lock()) {
    return -1;
  }

  // Check DB dir
  switch (_d->data.open(Path(_d->path, "data").c_str(), initialize)) {
    case 1:
      // Creation successful
      File(Path(_d->path, "missing")).create();
      out(info) << "Database initialized in " << _d->path << endl;
    case 0:
      // Open successful
      break;
    default:
      // Creation failed
      if (initialize) {
        out(error) << strerror(errno)
          << " creating data directory in given DB path: " << _d->path << endl;
      } else {
        out(error) << "Given DB path does not contain a database: "
          << _d->path << endl;
        if (! read_only) {
          out(info) << "See help for information on how to initialize the DB"
            << endl;
        }
      }
      if (! read_only) {
        unlock();
      }
      return -1;
  }

  // Reset client's name
  _d->client = NULL;

  // Do not load list of missing items for now
  _d->missing_id = -3;

  if (read_only) {
    _d->mode = 1;
    out(verbose) << "Database open in read-only mode" << endl;
  } else {
    // Set mode to quietly call openClient below
    _d->mode = 3;
    // Finish off any previous backup
    list<string> clients;
    if (getClients(clients) < 0) {
      unlock();
      return -1;
    }
    for (list<string>::iterator i = clients.begin(); i != clients.end(); i++) {
      if (openClient(i->c_str()) >= 0) {
        closeClient(true);
      }
      if (terminating()) {
        break;
      }
    }
    _d->mode       = 2;
    // Load list of missing items if/when required
    _d->missing_id = -2;
    out(verbose) << "Database open in read/write mode" << endl;
  }
  return 0;
}

int Database::close() {
  bool failed = false;

  if (_d->mode == 0) {
    out(alert) << "Cannot close: DB not open!" << endl;
  } else {
    if (_d->client != NULL) {
cout << "DBG0" << endl;
      closeClient(true);
    }
    if (_d->mode > 1) {
      // Save list of missing items
      if ((_d->missing_id > -2) && ! terminating()) {
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
            out(info) << "List of missing checksums contains " << count
              << " item(s)" << endl;
          }
          if (rc >= 0) {
            rc = missing_list.close();
          }
          if (rc >= 0) {
            update("missing", true);
          }
        } else {
          out(error) << strerror(errno) << " opening missing checksums list"
            << endl;
        }
      }
      // Release lock
      unlock();
    }
  }
  out(verbose) << "Database closed" << endl;
  _d->mode = 0;
  return failed ? -1 : 0;
}

int Database::getClients(
    list<string>& clients) const {
  // Clients' lists are in the DB base directory, named *.list
  Directory dir(_d->path);
  if (dir.createList() < 0) {
    return -1;
  }
  list<Node*>::iterator i = dir.nodesList().begin();
  while (i != dir.nodesList().end()) {
    if (((*i)->type() == 'f') && ((*i)->pathLength() > 5)) {
      if (strcmp(&(*i)->path()[(*i)->pathLength() - 5], ".list") == 0) {
        clients.push_back((*i)->name());
        clients.back().erase(clients.back().size() - 5);
      }
    }
    i++;
  }
  return 0;
}

int Database::getRecords(
    list<string>&   records,
    const char*     client,
    const char*     path,
    time_t          date) {
  // Look for clients
  if ((client == NULL) || (client[0] == '\0')) {
    return getClients(records);
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

    char* db_path = NULL;
    while (_d->client_list->search(client, NULL) == 2) {
      if (terminating()) {
        return -1;
      }
      if (_d->client_list->getEntry(NULL, NULL, &db_path, NULL, -2) <= 0) {
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

  // Restore relevant data
  while (_d->client_list->search(client, NULL) == 2) {
    if (terminating()) {
      failed = true;
      break;
    }
    if (_d->client_list->getEntry(&fts, &fclient, &fpath, &fnode, date) <= 0) {
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
    bool            rm_obsolete) {
  // Get checksums from list
  list<string> list_sums;
  Node*        node = NULL;
  list<string> clients;
  if (getClients(clients) < 0) {
    return -1;
  }
  _d->mode       = 3;
  _d->missing_id = -3;
  bool failed = false;
  for (list<string>::iterator i = clients.begin(); i != clients.end(); i++) {
    out(verbose) << "Reading " << *i << "'s list" << endl;
    if (openClient(i->c_str(), true) < 0) {
      failed = true;
    } else {
      _d->client_list->setProgressCallback(progress);
      while (_d->client_list->getEntry(NULL, NULL, NULL, &node) > 0) {
        if ((node != NULL) && (node->type() == 'f')) {
          File* f = (File*) node;
          if (f->checksum()[0] != '\0') {
            list_sums.push_back(f->checksum());
          }
        }
        if (terminating()) {
          break;
        }
      }
      closeClient(true);
    }
    if (failed || terminating()) {
      break;
    }
  }
  delete node;
  if (failed || terminating()) {
    return -1;
  }
  list_sums.sort();
  list_sums.unique();
  out(verbose) << "Found " << list_sums.size() << " checksums" << endl;

  // Get checksums from DB
  out(verbose) << "Crawling through DB" << endl;
  list<string> data_sums;
  // Check surficially, remove empty dirs
  int rc = _d->data.crawl(&data_sums, false, true);
  if (terminating() || (rc < 0)) {
    return -1;
  }

  if (! list_sums.empty()) {
    out(debug) << "Checksum(s) from list:" << endl;
    for (list<string>::iterator i = list_sums.begin(); i != list_sums.end();
        i++) {
      out(debug, 1) << *i << endl;
    }
  }
  if (! data_sums.empty()) {
    out(debug) << "Checksum(s) with data:" << endl;
    for (list<string>::iterator i = data_sums.begin(); i != data_sums.end();
        i++) {
      out(debug, 1) << *i << endl;
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
    out(info) << "Obsolete checksum(s): " << data_sums.size() << endl;
    for (list<string>::iterator i = data_sums.begin(); i != data_sums.end();
        i++) {
      if (i->size() != 0) {
        if (rm_obsolete) {
          out(verbose, 1) << *i << ": "
            << ((_d->data.remove(*i) == 0) ? "removed" : "FAILED") << endl;
        } else {
          out(verbose, 1) << *i << endl;
        }
      }
    }
  }
  if (! _d->missing.empty()) {
    out(warning) << "Missing checksum(s): " << _d->missing.size()
      << endl;
    for (unsigned int i = 0; i < _d->missing.size(); i++) {
      out(verbose, 1) << _d->missing[i] << endl;
    }
  }

  if (terminating()) {
    return -1;
  }
  // Make sure we save the list
  _d->missing_id = -1;
  return rc;
}

int Database::check(
    bool            remove) const {
  out(verbose) << "Crawling through DB data" << endl;
  // Check thoroughly, remove corrupted data if told (but not empty dirs)
  return _d->data.crawl(NULL, true, remove);
}

int Database::openClient(
    const char*     client,
    time_t          expire) {
  // Open client list
  bool   failed = false;
  string name   = Path(_d->path, client).c_str();

  if (_d->mode == 0) {
    out(alert) << "Open DB before opening client!" << endl;
    return -1;
  }

  // Keep client name
  _d->client = strdup(client);

  if (_d->mode > 1) {
    if (_d->missing_id == -2) {
      // Read list of missing checksums (it is ordered and contains no dup)
      out(verbose) << "Reading list of missing checksums" << endl;
      Stream missing_list(Path(_d->path, "missing"));
      if (! missing_list.open("r")) {
        string checksum;
        while (missing_list.getLine(checksum) > 0) {
          _d->missing.push_back(checksum);
          out(debug, 1) << checksum << endl;
        }
        missing_list.close();
      } else {
        out(warning) << strerror(errno) << " opening missing checksums list"
          << endl;
      }
      _d->missing_id = -1;
    }

    // Open list
    _d->client_list = new List((name + ".list").c_str());
    if (! _d->client_list->isValid()) {
      File backup((name + ".list~").c_str());

      if (backup.isValid()) {
        rename(backup.path(), _d->client_list->path());
        out(warning) << client << "'s list not accessible, using backup"
          << endl;
      } else {
        if (_d->client_list->open("w")) {
          out(error) << strerror(errno) << " creating " << client << "'s list"
            << endl;
          failed = true;
        } else {
          _d->client_list->close();
          out(info) << "Created " << client << "'s list" << endl;
        }
      }
    }
    if (! failed) {
      // Check journal
      _d->client_journal = new List((name + ".journal").c_str());
      _d->client_part    = new List((name + ".part").c_str());
      if (_d->client_journal->isValid()) {
        // Check previous crash
        out(warning) << "Backup of client " << client
          << " was interrupted in a previous run, finishing up..." << endl;
        _d->client_list->setProgressCallback(progress);
        if (finishOff(*_d->client_list, *_d->client_journal, *_d->client_part,
            true)) {
          out(error) << "Failed to recover previous data" << endl;
          failed = true;
        }
        _d->client_list->setProgressCallback(NULL);

      }

      // Create journal (do not cache)
      if (! failed && (_d->client_journal->open("w", -1))) {
        out(error) << "Cannot open " << client << "'s journal" << endl;
        failed = true;
      }
    }
    if (! failed) {
      // Open list
      if (_d->client_list->open("r")) {
        out(error) << "Cannot open " << client << "'s list" << endl;
        failed = true;
      } else
      // Open journal
      if (_d->client_journal->open("w", -1)) {
        out(error) << "Cannot open " << client << "'s journal" << endl;
        failed = true;
      } else
      // Open list
      if (_d->client_part->open("w")) {
        out(error) << "Cannot open " << client << "'s merge list" << endl;
        failed = true;
      }
    }
    if (failed) {
      _d->client_list->close();
      _d->client_journal->close();
      _d->client_part->close();
      delete _d->client_list;
      delete _d->client_journal;
      delete _d->client_part;
      return -1;
    }
    if (expire > 0) {
      _d->client_expire = time(NULL) - expire;
    } else {
      _d->client_expire = expire;
    }
    _d->client_journalled = false;
    if (_d->mode < 3) {
      out(verbose) << "Database client " << client
        << " open in read/write mode" << endl;
    }
  } else {
    if (! File((name + ".list").c_str()).isValid()) {
      out(error) << "Client " << client
        << "'s list not accessible, aborting" << endl;
      return -1;
    }

    string list_db = name + ".list";
    stringstream file_name;
    file_name << name << ".list." << getpid();
    if (link(list_db.c_str(), file_name.str().c_str())) {
      out(error) << "Could not create hard link to list" << endl;
      return -1;
    }

    bool failed = false;
    _d->client_list = new List(file_name.str().c_str());
    if (_d->client_list->open("r")) {
      out(error) << "Cannot open " << client << "'s list" << endl;
      failed = true;
    } else
    if (_d->client_list->isOldVersion()) {
      out(error)
        << "list in old format (run hbackup in backup mode to update)"
        << endl;
      failed = true;
    } else
    if (failed) {
      delete _d->client_list;
      _d->client_list = NULL;
      return -1;
    }
    out(verbose) << "Database client " << client << " open in read-only mode"
      << endl;
  }
  return (_d->client_list->search(client, "", _d->client_part, -1) < 0) ? -1:0;
}

int Database::closeClient(
    bool            abort) {
  bool failed = false;

  if (_d->mode == 1) {
    // Open read-only
    // Close list
    _d->client_list->close();
    // Remove link
    unlink(_d->client_list->path());
    // Delete list
    delete _d->client_list;
    // for isOpen and isWriteable
    _d->client_list = NULL;
  } else {
    if (_d->mode > 1) {
      if (abort) {
        // Skip to next client/end of file
        _d->client_list->search(NULL, NULL, _d->client_part, -1);
        // Keep client found
        _d->client_list->keepLine();
      } else {
        // Finish previous client work (removed items at end of list)
        sendEntry(NULL, NULL);
      }

      // Open read/write
      // Close journal
      _d->client_journal->close();

      if (_d->client_journal->isEmpty()) {
        _d->client_journal->remove();
        // Do nothing
        if (_d->mode < 3) {
          out(verbose) << "Database client " << _d->client << " not modified"
            << endl;
        }
      } else {
        if (! terminating()) {
          // Finish off list reading/copy
          _d->client_list->setProgressCallback(progress);
          // Finish off merging
          out(verbose) << "Closing database client " << _d->client << endl;
          _d->client_list->search("", "", _d->client_part, -1);
        }
      }

      // Close list
      _d->client_list->close();

      // Close merge
      _d->client_part->close();
      if (terminating() || _d->client_journal->isEmpty()) {
        _d->client_part->remove();
      }

      // Deal with new list and journal
      if (! _d->client_journal->isEmpty()) {
        if (! terminating()) {
          if (finishOff(*_d->client_list, *_d->client_journal,
              *_d->client_part, false)) {
            out(error) << "Failed to close lists" << endl;
            failed = true;
          }
        }
      }

      // Free lists
      delete _d->client_list;
      delete _d->client_journal;
      delete _d->client_part;
      _d->client_list    = NULL;
      _d->client_journal = NULL;
      _d->client_part    = NULL;
    }
  }

  if (_d->mode < 3) {
    out(verbose) << "Database client " << _d->client << " closed" << endl;
  }

  // Discard client name
  free(_d->client);
  _d->client = NULL;

  return failed ? -1 : 0;
}

int Database::sendEntry(
    const char*     remote_path,
    const Node*     node,
    char**          checksum) {
  // Reset ID of missing checksum
  _d->missing_id = -1;

  // DB
  char* db_path = NULL;
  Node* db_node = NULL;
  int   cmp     = 1;
  while (_d->client_list->search(_d->client, NULL, _d->client_part,
      _d->client_expire) == 2) {
    if (_d->client_list->getEntry(NULL, NULL, &db_path, NULL, -2) <= 0) {
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
        _d->client_list->getEntry(NULL, NULL, &db_path, &db_node, -1);
        // Do not lose this metadata!
        _d->client_list->keepLine();
      }
      break;
    }
    // Not reached, mark 'removed' (getLineType keeps line automatically)
    _d->client_part->addPath(db_path);
    if (_d->client_list->getLineType() != '-') {
      if (! _d->client_journalled) {
        _d->client_journal->addClient(_d->client);
        _d->client_journalled = true;
      }
      _d->client_journal->addPath(db_path);
      _d->client_journal->addData(time(NULL));
      // Add path and 'removed' entry
      _d->client_part->addData(time(NULL));
      out(info) << "D " << db_path << endl;
    }
  }
  free(db_path);

  // Compare entries
  bool add_node = false;
  if (remote_path != NULL) {
    _d->client_part->addPath(remote_path);
    if ((cmp > 0) || (db_node == NULL)) {
      // Exceeded, keep line for later
      _d->client_list->keepLine();
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
        if (node->type() == 'f') {
          const char* node_checksum = ((File*)db_node)->checksum();
          if (node_checksum[0] == '\0') {
            // Checksum missing: retry
            out(info) << "!";
            *checksum = strdup("");
            add_node = true;
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
                add_node = true;
              }
            }
          }
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
          if ((_d->missing_id >= 0)
          && (_d->missing[_d->missing_id] == checksum)) {
            out(verbose) << "Recovered checksum: " << checksum << endl;
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
    if (! _d->client_journalled) {
      _d->client_journal->addClient(_d->client);
      _d->client_journalled = true;
    }
    _d->client_journal->addPath(remote_path);
    _d->client_journal->addData(ts, node2);
    _d->client_part->addData(ts, node2, true);
  }
  delete node2;

  return failed ? -1 : 0;
}
