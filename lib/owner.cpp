/*
     Copyright (C) 2008-2010  Herve Fache

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

#include <stdio.h>
#include <errno.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "hreport.h"
#include "list.h"
#include "missing.h"
#include "opdata.h"
#include "compdata.h"
#include "owner.h"

using namespace hbackup;
using namespace hreport;

struct Owner::Private {
  Path              path;
  char*             name;
  bool              modified;
  List*             original;
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
  File next(Path(_d->path, "list.next"));

  // All files are expected to be closed
  bool got_next = recovery && next.isValid();

  // Need to open journal in read mode
  List journal(_d->journal->path());

  // Recovering from list and journal (step 0)
  if (recovery && ! got_next) {
    // Let's recover what we got
    errno = 0;
    if (! journal.open()) {
      bool failed = false;
      bool empty  = true;
      if (! journal.end()) {
        empty = false;
        out(verbose, msg_standard, _d->path.basename(), -1,
          "Register modified");
        if (! _d->partial->create()) {
          if (! _d->original->open()) {
            _d->original->setProgressCallback(_d->progress);
            if (List::merge(_d->original, _d->partial, &journal) < 0) {
              out(error, msg_standard, "Merge failed", -1, NULL);
              failed = true;
            }
            _d->original->setProgressCallback(NULL);
            _d->original->close();
          } else {
            out(error, msg_errno, "opening list", errno, NULL);
            failed = true;
          }
          if (_d->partial->close()) {
            out(error, msg_errno, "closing merge", errno, NULL);
            failed = true;
          }
        } else {
          out(error, msg_errno, "opening merge", errno, NULL);
          failed = true;
        }
      }
      journal.close();
      if (empty) {
        // Nothing to do (journal is empty)
        remove(journal.path());
        return 0;
      }
      if (failed) {
        return -1;
      }
    } else
    if (errno != ENOENT) {
      out(error, msg_errno, "opening journal", errno, NULL);
      return -1;
    }
  }

  // list._d->partial -> list.next (step 1)
  if (! got_next && rename(_d->partial->path(), next.path())) {
    out(error, msg_errno, "renaming next list", errno, NULL);
    return -1;
  }

  // Discard journal (step 2)
  if (! got_next || File(_d->journal->path()).isValid()) {
    if (rename(_d->journal->path(), Path(_d->path, "journal~"))) {
      out(error, msg_errno, "renaming journal", errno, NULL);
      return -1;
    }
  }

  // list -> list~ (step 3)
  if (! got_next || File(_d->original->path()).isValid()) {
    if (rename(_d->original->path(), Path(_d->path, "list~"))) {
      out(error, msg_errno, "renaming backup list", errno, NULL);
      return -1;
    }
  }

  // list.next -> list (step 4)
  if (rename(next.path(), _d->original->path())) {
    out(error, msg_errno, "renaming list", errno, NULL);
    return -1;
  }
  return 0;
}

Owner::Owner(
    const char*     path,
    const char*     name,
    time_t          expiration) : _d(new Private) {
  _d->path       = Path(path, name);
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
  delete _d;
}

const char* Owner::name() const {
  return _d->path.basename();
}

const char* Owner::path() const {
  return _d->path;
}

int Owner::hold() const {
  Directory owner_dir(_d->path);
  if (! owner_dir.isValid()) {
    out(error, msg_standard, "Directory does not exist, aborting", -1,
      _d->path.basename());
    return -1;
  }

  File owner_list(Path(_d->path, "list"));
  if (! owner_list.isValid()) {
    out(error, msg_standard, "Register not accessible, aborting", -1,
      _d->path.basename());
    return -1;
  }

  stringstream file_name;
  file_name << owner_list.path() << "." << getpid();
  if (link(owner_list.path(), file_name.str().c_str())) {
    out(error, msg_errno, "creating hard link to list, aborting", -1, NULL);
    return -1;
  }

  _d->original = new List(file_name.str().c_str());
  if (_d->original->open()) {
    out(error, msg_errno, "opening list, aborting", -1, _d->path.basename());
    release();
    return -1;
  }
  return 0;
}

int Owner::release() const {
  // Close list
  _d->original->close();
  // Remove link
  remove(_d->original->path());
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
          _d->path.basename());
        return -1;
      }
    } else
    if (check) {
      // No need to check if not initialized and not required to initialize
      return 0;
    } else
    {
      out(error, msg_standard, "Directory does not exist, aborting", -1,
        _d->path.basename());
      return -1;
    }
  }
  File owner_list(Path(_d->path, "list"));

  bool failed = false;
  // Open list
  _d->modified = false;
  _d->original = new List(owner_list.path());
  if (_d->original->open()) {
    File backup(Path(_d->path, "list~"));

    if (backup.isValid()) {
      rename(backup.path(), _d->original->path());
      out(warning, msg_standard, "Register not accessible, using backup", -1,
        _d->path.basename());
    } else if (initialize) {
      List original(owner_list.path());
      if (original.create()) {
        out(error, msg_errno, "creating list", errno, _d->path.basename());
        failed = true;
      } else {
        original.close();
        out(info, msg_standard, _d->path.basename(), -1, "Register created");
      }
    }
  } else {
    _d->original->close();
  }
  if (! failed) {
    // Check journal
    _d->journal = new List(Path(_d->path, "journal"));
    _d->partial = new List(Path(_d->path, "partial"));
    List journal(_d->journal->path());
    if (! journal.open()) {
      // Check previous crash
      journal.close();
      out(warning, msg_standard, _d->path.basename(), -1,
        "Previous backup interrupted");
      if (finishOff(true)) {
        out(error, msg_standard, "Failed to recover previous data", -1, NULL);
        failed = true;
      }
    }
  }
  if (! failed && ! check) {
    // Open list
    if (_d->original->open()) {
      out(error, msg_errno, "opening list", errno, _d->path.basename());
      failed = true;
    } else
    // Open journal
    if (_d->journal->create()) {
      _d->original->close();
      out(error, msg_errno, "creating journal", errno, _d->path.basename());
      failed = true;
    } else
    // Open list
    if (_d->partial->create()) {
      _d->original->close();
      _d->journal->close();
      out(error, msg_errno, "creating merge list", errno, _d->path.basename());
      failed = true;
    }
  }
  if (failed || check) {
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
      // Finish work (if not aborting, remove items at end of list)
      if (List::search(_d->original, "",
          _d->expiration, abort ? 0 : time(NULL), _d->partial, _d->journal,
          &_d->modified) < 0) {
        failed = true;
      }
      _d->journal->flush();
    }
    // Now we can close the journal (failure to close is not problematic)
    _d->journal->close();
    // Check whether any work was done
    if (! _d->modified) {
      // No modification done, don't need journal anymore
      remove(_d->journal->path());
    } else
    if (! aborting()) {
      out(verbose, msg_standard, _d->path.basename(), -1, "Register modified");
    }
    // Close list (was open read-only)
    _d->original->close();
    // Close merge
    if (_d->partial->close()) {
      failed = true;
    }
    // Check whether we need to merge now
    if (failed || aborting() || ! _d->modified) {
      // Merging will occur at next read/write open, if journal exists
      remove(_d->partial->path());
    } else
    // Merge now
    if (finishOff(false)) {
      out(error, msg_standard, "Failed to close lists", -1, NULL);
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

// The main output of sendEntry is the letter:
// * A: added data
// * ~: modified metadata
// * M: modified data
// * !: incomplete data (checksum missing)
// * C: conflicting data (size is not what was expected)
// * R: recoverable data (data not found in DB for checksum)
int Owner::send(
    OpData&         op,
    Missing&        missing) {
  Node* db_node = NULL;

  // Search path and get current metadata
  int rc = List::search(_d->original, op._path, _d->expiration, time(NULL),
    _d->partial, _d->journal, &_d->modified);
  if (rc < 0) {
    return -1;
  }
  _d->journal->flush();
  if (rc == 2) {
    // Get metadata (needs to be kept, as it will be added after any new)
    List::getEntry(_d->original, NULL, NULL, &db_node, -2);
  }

  // New or resurrected file: (re-)add
  if ((rc < 2) || (db_node == NULL)) {
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
  return 0;
}

int Owner::add(
    const Path&     path,
    const Node*     node) {
  if (List::add(path, node, _d->partial, _d->journal) || _d->journal->flush()) {
    return -1;
  }
  _d->modified = true;
  return 0;
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
  while (List::search(_d->original) == 2) {
    if (aborting()) {
      failed = true;
      return -1;
    }
    if (List::getEntry(_d->original, NULL, fpath, fnode, date) <= 0) {
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
  while (List::getEntry(_d->original, NULL, NULL, &node) > 0) {
    if ((node != NULL) && (node->type() == 'f')) {
      File* f = static_cast<File*>(node);
      if (f->checksum()[0] != '\0') {
        checksums.push_back(CompData(f->checksum(), f->size()));
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
