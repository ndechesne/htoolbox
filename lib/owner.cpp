/*
     Copyright (C) 2008  Herve Fache

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

#include <iostream>
#include <iomanip>
#include <list>

#include <errno.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "report.h"
#include <list.h>
#include <owner.h>

using namespace hbackup;

static void progress(long long previous, long long current, long long total) {
  if (current < total) {
    cout << "Done: " << setw(5) << setiosflags(ios::fixed) << setprecision(1)
      << 100.0 * current /total << "%\r" << flush;
  } else if (previous != 0) {
    cout << "            \r";
  }
}

struct Owner::Private {
  char*             path;
  char*             name;
  List*             original;
  List*             journal;
  List*             partial;
  int               expiration;
};

// The procedure is as follows:
// ---> step 0: backup running
//    list      -> part       (copy, always discarded)
//              -> journal    (create, used for recovery)
// ---> step 1: backup finished
//    part      -> next       (rename, journal will now be discarded)
// ---> step 2
//    journal   ->            (delete, discard journal)
// ---> step 3
//    list      -> list~      (rename, overwrite backup)
// ---> step 4
//    next      -> list       (rename, process finished)
// ---> list ballet finished
//
// This allows for a safe recovery:
//    if list.next:
//      discard journal
//      goto ---> step 3
//    if journal:
//      merge list + journal -> part
//      goto ---> step 1
int Owner::finishOff(
    bool            recovery) {
  string name = _d->original->path();
  File next((name + ".next").c_str());

  // All files are expected to be closed
  bool got_next = recovery && next.isValid();

  // Recovering from list and journal (step 0)
  if (recovery && ! got_next) {
    if (_d->journal->isValid()) {
      // Let's recover what we got
      if (! _d->partial->open("w")) {
        bool failed = false;
        _d->original->open("r");
        _d->journal->open("r");
        _d->original->setProgressCallback(progress);
        if (_d->partial->merge(*_d->original, *_d->journal) < 0) {
          failed = true;
        }
        _d->original->setProgressCallback(NULL);
        _d->original->close();
        _d->journal->close();
        _d->partial->close();
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

  // list._d->partial -> list.next (step 1)
  if (! got_next && rename(_d->partial->path(), next.path())) {
    return -1;
  }

  // Discard journal (step 2)
  if (! got_next || _d->journal->isValid()) {
    string backup = _d->journal->path();
    backup += "~";
    if (rename(_d->journal->path(), backup.c_str())) {
      return -1;
    }
  }

  // list -> list~ (step 3)
  if (! got_next || _d->original->isValid()) {
    if (rename(_d->original->path(), (name + "~").c_str())) {
      return -1;
    }
  }

  // list.next -> list (step 4)
  if (rename(next.path(), _d->original->path())) {
    return -1;
  }
  return 0;
}

Owner::Owner(
    const char*     path,
    const char*     name,
    time_t          expiration) {
  _d             = new Private;
  _d->path       = strdup(Path(path, name).c_str());
  _d->name       = strdup(name);
  _d->original   = NULL;
  _d->journal    = NULL;
  _d->partial    = NULL;
  _d->expiration = expiration;
}

Owner::~Owner() {
  if (_d->original != NULL) {
    close(true);
  }
  free(_d->path);
  free(_d->name);
  delete _d;
}

const char* Owner::name() const {
  return _d->name;
}

const char* Owner::path() const {
  return _d->path;
}

time_t Owner::expiration() const {
  return _d->expiration;
}

int Owner::open(
    bool            read_only,
    bool            initialize,
    bool            check) {
  if (read_only && initialize) {
    out(error, msg_standard, "Cannot initialize in read-only mode");
    return -1;
  }
  if (read_only && check) {
    out(error, msg_standard, "Cannot check in read-only mode");
    return -1;
  }
  Directory owner_dir(_d->path);
  if (! owner_dir.isValid()) {
    if (initialize) {
      if (owner_dir.create() < 0) {
        out(error, msg_standard, "Directory cannot be created, aborting", -1,
          _d->name);
        return -1;
      }
    } else
    if (check) {
      // No need to check if not initialized and not required to initialize
      return 0;
    } else
    {
      out(error, msg_standard, "Directory does not exist, aborting", -1,
        _d->name);
      return -1;
    }
  }
  string owner_path = owner_dir.path();
  owner_path += "/";
  File owner_list((owner_path + "list").c_str());

  bool failed = false;
  if (read_only) {
    if (! owner_list.isValid()) {
      out(error, msg_standard, "List not accessible, aborting", -1,
        _d->name);
      return -1;
    }

    stringstream file_name;
    file_name << owner_list.path() << "." << getpid();
    if (link(owner_list.path(), file_name.str().c_str())) {
      out(error, msg_standard, "Could not create hard link to list, aborting");
      return -1;
    }

    _d->original = new List(file_name.str().c_str());
    if (_d->original->open("r")) {
      out(error, msg_standard, "Cannot open list, aborting", -1, _d->name);
      failed = true;
    } else
    if (_d->original->isOldVersion()) {
      out(error, msg_standard,
        "List in old format (try running hbackup in backup mode to update)"
        ", aborting");
      failed = true;
    } else
    if (failed) {
      delete _d->original;
      _d->original = NULL;
    }
  } else {
    // Open list
    _d->original = new List(owner_list.path());
    if (! _d->original->isValid()) {
      File backup((owner_path + "list~").c_str());

      if (backup.isValid()) {
        rename(backup.path(), _d->original->path());
        out(warning, msg_standard, "List not accessible, using backup", -1,
          _d->name);
      } else if (initialize) {
        if (_d->original->open("w")) {
          out(error, msg_errno, "Creating list", errno, _d->name);
          failed = true;
        } else {
          _d->original->close();
          out(info, msg_standard, _d->name, -1, "List created");
        }
      }
    }
    if (! failed) {
      // Check journal
      _d->journal = new List((owner_path + "journal").c_str());
      _d->partial = new List((owner_path + "partial").c_str());
      if (_d->journal->isValid()) {
        // Check previous crash
        out(warning, msg_standard, _d->name, -1,
          "Previous backup interrupted");
        if (finishOff(true)) {
          out(error, msg_standard, "Failed to recover previous data");
          failed = true;
        }
      }
    }
    if (! failed && ! check) {
      // Open list
      if (_d->original->open("r")) {
        out(error, msg_errno, "Opening list", errno, _d->name);
        failed = true;
      } else
      // Open journal
      if (_d->journal->open("w", -1)) {
        out(error, msg_errno, "Opening journal", errno, _d->name);
        failed = true;
      } else
      // Open list
      if (_d->partial->open("w")) {
        out(error, msg_errno, "Opening merge list", errno, _d->name);
        failed = true;
      }
    }
    if (failed || check) {
      if (_d->original->isOpen()) {
        _d->original->close();
      }
      if (_d->journal->isOpen()) {
        _d->journal->close();
      }
      if (_d->partial->isOpen()) {
        _d->partial->close();
      }
      delete _d->original;
      delete _d->journal;
      delete _d->partial;
      _d->original = NULL;
      _d->journal  = NULL;
      _d->partial  = NULL;
    }
  }
  return failed ? -1 : 0;
}

int Owner::close(
    bool            teardown) {
  bool failed = false;
  if (_d->original == NULL) {
    // Not open, maybe just checked
  } else
  if (_d->journal == NULL) {
    // Open read-only
    // Close list
    _d->original->close();
    // Remove link
    unlink(_d->original->path());
    // Delete list
    delete _d->original;
    // for isOpen and isWriteable
    _d->original = NULL;
  } else {
    // Open read/write
    // Finish previous client work (removed items at end of list)
    if (! teardown) {
      search(NULL, NULL);
    }
    // Close journal
    _d->journal->close();
    // Decide what to do with journal
    if (_d->journal->isEmpty()) {
      // Do nothing
      _d->journal->remove();
    } else {
      if (! terminating()) {
        // Finish off list reading/copy
        _d->original->setProgressCallback(progress);
        // Finish off merging
        out(verbose, msg_standard, _d->name, -1, "Database modified");
        _d->original->search("", _d->partial, -1);
      }
    }
    // Close list
    _d->original->close();
    // Close merge
    _d->partial->close();
    // Decide what to do with merge
    if (terminating() || _d->journal->isEmpty()) {
      _d->partial->remove();
    }
    // Deal with new list and journal
    if (! _d->journal->isEmpty()) {
      if (! terminating()) {
        if (finishOff(false)) {
          out(error, msg_standard, "Failed to close lists");
          failed = true;
        }
      }
    }
    // Free lists
    delete _d->original;
    delete _d->journal;
    delete _d->partial;
    _d->original    = NULL;
    _d->journal = NULL;
    _d->partial    = NULL;
  }
  return failed ? -1 : 0;
}

int Owner::search(
    const char*     path,
    Node**          node) const {
  char* db_path = NULL;
  // If we don't enter the loop (no records), we want to add
  int   cmp     = 1;
  bool  failed  = false;
  while (_d->original->search(NULL, _d->partial, _d->expiration) == 2) {
    if (_d->original->getEntry(NULL, &db_path, NULL, -2) <= 0) {
      // Error
      out(error, msg_standard, "Failed to get entry");
      failed = true;
      break;
    }
    if (path != NULL) {
      cmp = Path::compare(db_path, path);
      // If found or exceeded, break loop
      if (cmp >= 0) {
        if (cmp == 0) {
          // Get metadata
          _d->original->getEntry(NULL, &db_path, node, -1);
          // Do not lose this metadata!
          _d->original->keepLine();
        }
        break;
      }
    } else {
      // Want to get all paths
      cmp = -1;
    }
    // Make sure we are not terminating
    if (terminating()) {
      failed = true;
      break;
    }
    // Not reached, mark 'removed' (getLineType keeps line automatically)
    _d->partial->addPath(db_path);
    if (_d->original->getLineType() != '-') {
      _d->journal->addPath(db_path);
      _d->journal->addData(time(NULL));
      // Add path and 'removed' entry
      _d->partial->addData(time(NULL));
      out(info, msg_standard, db_path, -2, "D");
    }
  }
  free(db_path);
  if (failed) {
    return -1;
  }
  if (path != NULL) {
    _d->partial->addPath(path);
    if ((cmp > 0) || (*node == NULL)) {
      _d->original->keepLine();
    }
  }
  return (cmp > 0) ? 1 : 0;
}

int Owner::add(
    const char*     path,
    const Node*     node,
    time_t          timestamp) {
  return ((_d->journal->addPath(path) < 0)
  ||      (_d->journal->addData(timestamp, node) < 0)
  ||      (_d->partial->addData(timestamp, node, true) < 0)) ? -1 : 0;
}

int Owner::getNextRecord(
    const char*     path,
    time_t          date,
    char**          fpath,
    Node**          fnode) const {
  int  len = strlen(path);
  bool path_is_dir = false;
  if ((len > 0) && (path[len - 1] == '/')) {
    path_is_dir = true;
    len--;
  }
  bool failed = false;
  while (_d->original->search() == 2) {
    if (terminating()) {
      failed = true;
      return -1;
    }
    if (_d->original->getEntry(NULL, fpath, fnode, date) <= 0) {
      failed = true;
      return -1;
    }
    // Start of path must match
    if (len == 0) return 1;
    int cmp = Path::compare(*fpath, path, len);
    // Not reached
    if (cmp < 0) continue;
    // Exceeded
    if (cmp > 0) return 0;
    int flen = strlen(*fpath);
    // Not reached, but on the right path
    if (flen < len) continue;
    if ((flen > len) && ((*fpath)[len] != '/')) return 0;
    // Match
    return 1;
  }
  return failed ? -1 : 0;
}

int Owner::getChecksums(
    list<string>&   checksums) {
// Not const because of open, need a read-only open that'd be const...
  if (open(true) < 0) {
    return -1;
  }
  _d->original->setProgressCallback(progress);
  Node* node = NULL;
  while (_d->original->getEntry(NULL, NULL, &node) > 0) {
    if ((node != NULL) && (node->type() == 'f')) {
      File* f = (File*) node;
      if (f->checksum()[0] != '\0') {
        checksums.push_back(f->checksum());
      }
    }
    if (terminating()) {
      break;
    }
  }
  delete node;
  close(true);
  return terminating() ? -1 : 0;
}
