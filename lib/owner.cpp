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

#include <sstream>
#include <list>

#include <errno.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "report.h"
#include "list.h"
#include "missing.h"
#include "opdata.h"
#include "compdata.h"
#include "owner.h"

using namespace hbackup;

struct Owner::Private {
  char*             path;
  char*             name;
  ListReader*       original;
  List*             journal;
  List*             partial;
  int               expiration;
  progress_f        progress;
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

  // Need to open journal in read mode
  ListReader journal(_d->journal->path());

  // Recovering from list and journal (step 0)
  if (recovery && ! got_next) {
    if (journal.isValid()) {
      // Let's recover what we got
      bool failed = false;
      if (! journal.open()) {
        if (! journal.isEmpty()) {
          out(verbose, msg_standard, _d->name, -1, "Database modified");
          if (! _d->partial->open()) {
            if (! _d->original->open()) {
              _d->original->setProgressCallback(_d->progress);
              if (_d->partial->merge(*_d->original, journal) < 0) {
                out(error, msg_standard, "Merge failed");
                failed = true;
              }
              _d->original->setProgressCallback(NULL);
              _d->original->close();
            } else {
              out(error, msg_standard, "Open list failed");
              failed = true;
            }
            if (_d->partial->close()) {
              out(error, msg_standard, "Close merge failed");
              failed = true;
            }
          } else {
            out(error, msg_standard, "Open merge failed");
            failed = true;
          }
        }
        journal.close();
        if (journal.isEmpty()) {
          // Nothing to do (journal is empty)
          journal.remove();
          return 0;
        }
      } else {
        out(error, msg_standard, "Open journal failed");
        failed = true;
      }
      if (failed) {
        return -1;
      }
    } else {
      // Nothing to do (no list.next => list exists)
      return 0;
    }
  }

  // list._d->partial -> list.next (step 1)
  if (! got_next && rename(_d->partial->path(), next.path())) {
    out(error, msg_errno, "rename next list failed", errno);
    return -1;
  }

  // Discard journal (step 2)
  if (! got_next || _d->journal->isValid()) {
    string backup = _d->journal->path();
    backup += "~";
    if (rename(_d->journal->path(), backup.c_str())) {
      out(error, msg_errno, "rename journal failed", errno);
      return -1;
    }
  }

  // list -> list~ (step 3)
  if (! got_next || _d->original->isValid()) {
    if (rename(_d->original->path(), (name + "~").c_str())) {
      out(error, msg_errno, "rename backup list failed", errno);
      return -1;
    }
  }

  // list.next -> list (step 4)
  if (rename(next.path(), _d->original->path())) {
    out(error, msg_errno, "rename list failed", errno);
    return -1;
  }
  return 0;
}

Owner::Owner(
    const char*     path,
    const char*     name,
    time_t          expiration) : _d(new Private) {
  _d->path       = strdup(Path(path, name));
  _d->name       = strdup(name);
  _d->original   = NULL;
  _d->journal    = NULL;
  _d->partial    = NULL;
  _d->expiration = expiration;
  // Reset progress callback function
  _d->progress   = NULL;
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

int Owner::hold() const {
  Directory owner_dir(_d->path);
  if (! owner_dir.isValid()) {
    out(error, msg_standard, "Directory does not exist, aborting", -1,
      _d->name);
    return -1;
  }

  bool failed = false;
  File owner_list(Path(owner_dir.path(), "list"));
  if (! owner_list.isValid()) {
    out(error, msg_standard, "List not accessible, aborting", -1, _d->name);
    return -1;
  }

  stringstream file_name;
  file_name << owner_list.path() << "." << getpid();
  if (link(owner_list.path(), file_name.str().c_str())) {
    out(error, msg_standard, "Could not create hard link to list, aborting");
    return -1;
  }

  _d->original = new ListReader(file_name.str().c_str());
  if (_d->original->open()) {
    out(error, msg_standard, "Cannot open list, aborting", -1, _d->name);
    failed = true;
  } else
  if (_d->original->isOldVersion()) {
    out(error, msg_standard,
      "List in old format (try running hbackup in backup mode to update)"
      ", aborting");
    failed = true;
  }
  if (failed) {
    release();
    return -1;
  }
  return 0;
}

int Owner::release() const {
  // Close list
  if (_d->original->isOpen()) {
    _d->original->close();
  }
  // Remove link
  _d->original->remove();
  // Delete list
  delete _d->original;
  // for isOpen and isWriteable
  _d->original = NULL;
  return 0;
}

int Owner::open(
    bool            initialize,
    bool            check) {
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
  // Open list
  _d->original = new ListReader(owner_list.path());
  if (! _d->original->isValid()) {
    File backup((owner_path + "list~").c_str());

    if (backup.isValid()) {
      rename(backup.path(), _d->original->path());
      out(warning, msg_standard, "List not accessible, using backup", -1,
        _d->name);
    } else if (initialize) {
      List original(owner_list.path());
      if (original.open()) {
        out(error, msg_errno, "creating list", errno, _d->name);
        failed = true;
      } else {
        original.close();
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
    if (_d->original->open()) {
      out(error, msg_errno, "opening list", errno, _d->name);
      failed = true;
    } else
    // Open journal
    if (_d->journal->open()) {
      out(error, msg_errno, "opening journal", errno, _d->name);
      failed = true;
    } else
    // Open list
    if (_d->partial->open()) {
      out(error, msg_errno, "opening merge list", errno, _d->name);
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
  return failed ? -1 : 0;
}

int Owner::close(
    bool            abort) {
  bool failed = false;
  if (_d->original == NULL) {
    // Not open, maybe just checked
  } else
  if (_d->journal == NULL) {
    // Open read-only
    release();
  } else {
    // Open read/write
    _d->original->setProgressCallback(_d->progress);
    if (! aborting()) {
      // Leave things clean
      if (! abort) {
        // Finish work (remove items at end of list)
        search(NULL, NULL);
      } else {
        // Finish off merging
        _d->original->search("", _d->partial, _d->expiration);
      }
    }
    // Now we can close the journal (failure to close is not problematic)
    _d->journal->close();
    // Check whether any work was done
    if (_d->journal->isEmpty()) {
      // No modification done, don't need journal anymore
      _d->journal->remove();
    } else
    if (! aborting()) {
      out(verbose, msg_standard, _d->name, -1, "Database modified");
    }
    // Close list (was open read-only)
    _d->original->close();
    // Close merge
    if (_d->partial->close()) {
      failed = true;
    }
    // Check whether we need to merge now
    if (failed || aborting() || _d->journal->isEmpty()) {
      // Merging will occur at next read/write open, if journal exists
      _d->partial->remove();
    } else
    // Merge now
    if (finishOff(false)) {
      out(error, msg_standard, "Failed to close lists");
      failed = true;
    }
    // Free lists
    delete _d->original;
    delete _d->journal;
    delete _d->partial;
    _d->original = NULL;
    _d->journal  = NULL;
    _d->partial  = NULL;
  }
  return failed ? -1 : 0;
}

void Owner::setProgressCallback(progress_f progress) {
  _d->progress = progress;
}

int Owner::search(
    const char*     path,
    Node**          node) const {
  char* db_path = NULL;
  // If we don't enter the loop (no records), we want to add
  int   cmp     = 1;
  bool  failed  = false;
  // For each path
  while (_d->original->search(NULL, _d->partial, _d->expiration) == 2) {
    // Get path
    if (_d->original->getEntry(NULL, &db_path, NULL, -2) <= 0) {
      // Error
      out(error, msg_standard, "Failed to get path");
      failed = true;
      break;
    }
    if (path != NULL) {
      cmp = Path::compare(db_path, path);
      // If found or exceeded, break loop
      if (cmp >= 0) {
        if (cmp == 0) {
          // Get metadata
          _d->original->getEntry(NULL, &db_path, node);
          // This metadata needs to be kept, as it will be added after any new
          _d->original->keepLine();
        }
        break;
      }
    } else {
      // Want to get all paths
      cmp = -1;
    }
    // Make sure we are not terminating
    if (aborting()) {
      failed = true;
      break;
    }
    // Not reached, mark 'removed' (getLineType keeps line automatically)
    if (_d->original->getLineType() != '-') {
      // Add path and 'removed' entry
      time_t tm = time(NULL);
      _d->journal->add(db_path, tm);
      _d->partial->add(db_path, tm);
      out(info, msg_standard, db_path, -2, "D      ");
    } else {
      _d->partial->add(db_path);
    }
  }
  free(db_path);
  if (failed) {
    return -1;
  }
  if (path != NULL) {
    _d->partial->add(path);
    if ((cmp > 0) || (*node == NULL)) {
      _d->original->keepLine();
    }
  }
  return (cmp > 0) ? 1 : 0;
}

// The main output of sendEntry is the letter:
// * A: added data
// * ~: modified metadata
// * M: modified data
// * !: incomplete data (checksum missing)
// * C: conflicting data (size is not what was expected)
// * R: recoverable data (data not found in DB for checksum)
void Owner::sendEntry(
    OpData&         op,
    Missing&        missing) {
  Node* db_node = NULL;
  int rc = search(op._path, &db_node);

  // New file: add
  if ((rc > 0) || (db_node == NULL)) {
    op._operation = 'A';
  } else
  // Existing file: check for differences
  if (*db_node != op._node) {
    // Metadata differ but not type, mtime and size: just add new metadata
    if ((op._node.type() == db_node->type())
    &&  (op._node.size() == db_node->size())
    &&  (op._node.mtime() == db_node->mtime())) {
      op._operation = '~';
      if (op._node.type() == 'f') {
        const char* node_checksum = static_cast<File*>(db_node)->checksum();
        // Checksum missing: retry
        if (node_checksum[0] == '\0') {
          op._same_list_entry = true;
        } else
        // Copy checksum accross
        {
          static_cast<File&>(op._node).setChecksum(node_checksum);
        }
      }
    } else
    // Data differs
    {
      op._operation = 'M';
    }
  } else
  // Same metadata (hence same type): check for broken data
  if (op._node.type() == 'f') {
    // Check that file data is present
    const char* node_checksum = static_cast<File*>(db_node)->checksum();
    if (node_checksum[0] == '\0') {
      // Checksum missing: retry
      op._same_list_entry = true;
      op._operation = '!';
    } else {
      // Same checksum: look for checksum in missing list (binary search)
      op._id = missing.search(node_checksum);
      if (op._id >= 0) {
        if (missing.isInconsistent(op._id)) {
          if (missing.dataSize(op._id) != op._node.size()) {
            // Replace data with correct one
            op._same_list_entry = true;
            op._operation = 'C';
          }
        } else {
          // Recover missing data
          op._operation = 'R';
        }
      }
    }
  } else
  if ((op._node.type() == 'd') && (op._node.size() == -1)) {
    op._same_list_entry = true;
    op._operation = '!';
  } else
  if ((op._node.type() == 'l')
  && (strcmp(static_cast<const Link&>(op._node).link(),
      static_cast<Link*>(db_node)->link()) != 0)) {
    op._operation = 'L';
  }
  delete db_node;
}

int Owner::add(
    const char*     path,
    const Node*     node,
    time_t          timestamp) {
  return ((_d->journal->add(path, timestamp, node) < 0)
  ||      (_d->partial->add(NULL, timestamp, node) < 0)) ? -1 : 0;
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
    if (aborting()) {
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
    list<CompData>& checksums) const {
  if (hold() < 0) {
    return -1;
  }
  _d->original->setProgressCallback(_d->progress);
  Node* node = NULL;
  while (_d->original->getEntry(NULL, NULL, &node) > 0) {
    if ((node != NULL) && (node->type() == 'f')) {
      File* f = static_cast<File*>(node);
      if (f->checksum()[0] != '\0') {
        CompData d(f->checksum(), f->size());
        checksums.push_back(d);
      }
    }
    if (aborting()) {
      break;
    }
  }
  delete node;
  release();
  return aborting() ? -1 : 0;
}
