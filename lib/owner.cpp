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
#include "db.h"
#include "compdata.h"
#include "owner.h"

using namespace hbackup;
using namespace htools;

struct Owner::Private {
  string          name;
  string          path;
  bool            modified;
  bool            files_modified;
  ListReader*     original;
  ListWriter*     journal;
  ListWriter*     partial;
  time_t          expiration;
  progress_f      progress;
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
  Node next(Path(_d->path.c_str(), "list.next"));

  // All files are expected to be closed
  bool got_next = recovery && next.isReg();

  // Need to open journal in read mode
  ListReader journal(_d->journal->path());

  // Recovering from list and journal (step 0)
  if (recovery && ! got_next) {
    // Let's recover what we got
    errno = 0;
    if (! journal.open()) {
      bool failed = false;
      bool empty  = true;
      if (! journal.end()) {
        empty = false;
        hlog_verbose("Register modified for '%s'", _d->name.c_str());
        if (! _d->partial->open()) {
          if (! _d->original->open()) {
            _d->original->setProgressCallback(_d->progress);
            if (ListWriter::merge(*_d->partial, *_d->original, journal) < 0) {
              hlog_error("Merge failed");
              failed = true;
            } else {
              _d->files_modified = true;
            }
            _d->original->setProgressCallback(NULL);
            _d->original->close();
          } else {
            hlog_error("%s opening list '%s'", strerror(errno),
              _d->original->path());
            failed = true;
          }
          if (_d->partial->close()) {
            hlog_error("%s closing merge '%s'", strerror(errno),
              _d->partial->path());
            failed = true;
          }
        } else {
          hlog_error("%s opening merge '%s'", strerror(errno),
            _d->partial->path());
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
      hlog_error("%s opening journal '%s'", strerror(errno), journal.path());
      return -1;
    }
  }

  // list._d->partial -> list.next (step 1)
  if (! got_next && rename(_d->partial->path(), next.path())) {
    hlog_error("%s renaming next list to '%s'", strerror(errno), next.path());
    return -1;
  }

  // Discard journal (step 2)
  if (! got_next || Node(_d->journal->path()).isReg()) {
    if (rename(_d->journal->path(), Path(_d->path.c_str(), "journal~").c_str())) {
      hlog_error("%s renaming journal to '%s/journal~'", strerror(errno),
        _d->path.c_str());
      return -1;
    }
  }

  // list -> list~ (step 3)
  if (! got_next || Node(_d->original->path()).isReg()) {
    if (rename(_d->original->path(), Path(_d->path.c_str(), "list~").c_str())) {
      hlog_error("%s renaming backup list to '%s/list~'", strerror(errno),
        _d->path.c_str());
      return -1;
    }
  }

  // list.next -> list (step 4)
  if (rename(next.path(), _d->original->path())) {
    hlog_error("%s renaming list to '%s'", strerror(errno),
      _d->original->path());
    return -1;
  }
  return 0;
}

Owner::Owner(
    const char*     path,
    const char*     name,
    time_t          expiration) : _d(new Private) {
  _d->name       = name;
  _d->path       = Path(path, name).c_str();
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
  return _d->name.c_str();
}

const char* Owner::path() const {
  return _d->path.c_str();
}

int Owner::hold() const {
  _d->files_modified = false;
  Node owner_dir(_d->path.c_str());
  if (! owner_dir.isDir()) {
    hlog_error("Directory does not exist '%s', aborting", _d->name.c_str());
    return -1;
  }

  Node owner_list(Path(_d->path.c_str(), "list").c_str());
  if (! owner_list.isReg()) {
    hlog_error("Register not accessible in '%s', aborting", _d->name.c_str());
    return -1;
  }

  stringstream file_name;
  file_name << owner_list.path() << "." << getpid();
  if (link(owner_list.path(), file_name.str().c_str())) {
    hlog_error("%s creating hard link to list, aborting", strerror(errno));
    return -1;
  }

  _d->original = new ListReader(file_name.str().c_str());
  if (_d->original->open()) {
    hlog_error("%s opening list '%s', aborting", strerror(errno),
      _d->name.c_str());
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
  Node owner_dir(_d->path.c_str());
  if (! owner_dir.isDir()) {
    if (initialize) {
      if (owner_dir.mkdir() < 0) {
        hlog_error("Node cannot be created '%s', aborting",
          _d->name.c_str());
        return -1;
      }
    } else
    if (check) {
      // No need to check if not initialized and not required to initialize
      return 0;
    } else
    {
      hlog_error("Node does not exist '%s', aborting", _d->name.c_str());
      return -1;
    }
  }
  Node owner_list(Path(_d->path.c_str(), "list").c_str());

  bool failed = false;
  // Open list
  _d->modified = false;
  _d->files_modified = false;
  _d->original = new ListReader(owner_list.path());
  if (_d->original->open(initialize)) {
    Node backup(Path(_d->path.c_str(), "list~").c_str());

    if (backup.isReg()) {
      rename(backup.path(), _d->original->path());
      hlog_warning("Register not accessible '%s', using backup",
        _d->name.c_str());
    } else if (initialize) {
      ListWriter original(owner_list.path());
      if (original.open()) {
        hlog_error("%s creating list '%s'", strerror(errno), _d->name.c_str());
        failed = true;
      } else {
        original.close();
        hlog_info("Register created for '%s'", _d->name.c_str());
      }
    }
  } else {
    _d->original->close();
  }
  if (! failed) {
    // Check journal
    _d->journal = new ListWriter(Path(_d->path.c_str(), "journal").c_str(), true);
    _d->partial = new ListWriter(Path(_d->path.c_str(), "partial").c_str());
    ListReader journal(_d->journal->path());
    if (! journal.open(true)) {
      // Check previous crash
      journal.close();
      hlog_warning("Previous backup interrupted for '%s'", _d->name.c_str());
      if (finishOff(true)) {
        hlog_error("Failed to recover previous data");
        failed = true;
      }
    }
  }
  if (! failed && ! check) {
    // Open list
    if (_d->original->open()) {
      hlog_error("%s opening list in '%s'", strerror(errno),
        _d->name.c_str());
      failed = true;
    } else
    // Open journal
    if (_d->journal->open()) {
      _d->original->close();
      hlog_error("%s creating journal in '%s'", strerror(errno),
        _d->name.c_str());
      failed = true;
    } else
    // Open list
    if (_d->partial->open()) {
      _d->original->close();
      _d->journal->close();
      hlog_error("%s creating merge list in '%s'", strerror(errno),
        _d->name.c_str());
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
      if (_d->partial->search(_d->original, "", _d->expiration,
          abort ? 0 : time(NULL), _d->journal, &_d->modified) < 0) {
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
      hlog_verbose("Register modified for '%s'", _d->name.c_str());
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
      hlog_error("Failed to close lists");
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

bool Owner::modified() const {
  return _d->files_modified;
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
    Database::OpData&   op,
    Missing&            missing) {
  // Search path and get current metadata
  int rc = _d->partial->search(_d->original, op.path, _d->expiration,
    time(NULL), _d->journal, &_d->modified);
  if (rc < 0) {
    return -1;
  }
  _d->journal->flush();

  const char* db_node = NULL;
  const char* db_node_type = NULL;
  const char* db_node_extra = NULL;
  if (rc == 2) {
    // Get metadata (needs to be kept, as it will be added after any new)
    db_node = _d->original->fetchData();
    if (db_node == NULL) {
      hlog_error("no data found for '%s' in list", op.path);
      return -1;
    }
    // Find second TAB (first TAB is first char of string)
    db_node_type = strchr(&db_node[1], '\t');
    if (db_node_type == NULL) {
      hlog_error("strange data found for '%s' in list", op.path);
      return -1;
    }
    ++db_node_type;
    // Is the file still active?
    if (*db_node_type == '-') {
      db_node = NULL;
    } else
    if ((op.node.type() == 'f') || (op.node.type() == 'l')) {
      // Go get extra, the 8th field, but we already got rid of one!
      db_node_extra = db_node_type;
      for (size_t skip_fields = 0; skip_fields < 6; ++skip_fields) {
        db_node_extra = strchr(db_node_extra, '\t');
        if ((db_node_extra == NULL) || (++db_node_extra == '\0')) {
          db_node_extra = NULL;
          break;
        }
      }
    }
  }

  // Prepare metadata and extra
  op.end_offset = List::encode(op.node, op.encoded_metadata, &op.sep_offset);
  op.extra = NULL;

  // New or resurrected file: (re-)add
  if ((rc < 2) || (db_node == NULL)) {
    op.operation = 'A';
  } else
  // Existing file: check for differences
  if (strncmp(db_node_type, op.encoded_metadata, op.end_offset) != 0) {
    // Metadata differ but not type, mtime and size: just add new metadata
    if (strncmp(db_node_type, op.encoded_metadata, op.sep_offset) == 0) {
      op.operation = '~';
      if (op.node.type() == 'f') {
        // Checksum missing: retry
        if (db_node_extra == NULL) {
          op.same_list_entry = true;
        }
        // Copy checksum accross
        op.extra = db_node_extra;
      }
    } else
    // Data differs
    {
      op.operation = 'M';
    }
  } else
  // Same metadata (hence same type): check for broken data
  if (op.node.type() == 'f') {
    // Check that file data is present
    if (db_node_extra == NULL) {
      // Checksum missing: retry
      op.same_list_entry = true;
      op.operation = '!';
    } else {
      // Same checksum: look for checksum in missing list (binary search)
      op.id = missing.search(db_node_extra);
      if (op.id >= 0) {
        if (missing.isInconsistent(op.id)) {
          if (missing.dataSize(op.id) != op.node.size()) {
            // Replace data with correct one
            op.same_list_entry = true;
            op.operation = 'C';
          }
        } else {
          // Recover missing data
          op.operation = 'R';
        }
      } else
      // Copy checksum accross
      {
        op.extra = db_node_extra;
      }
    }
  } else
  if ((op.node.type() == 'd') && (op.node.size() == -1)) {
    op.same_list_entry = true;
    op.operation = '!';
  } else
  if ((op.node.type() == 'l') && (db_node_extra != NULL) &&
      (strcmp(op.node.link(), db_node_extra) != 0)) {
    op.operation = 'L';
  }
  return 0;
}

int Owner::add(
    const Database::OpData& op) {
  int rc = 0;
  time_t ts = time(NULL);
  // Add to new list
  if (_d->partial->putData(ts, op.encoded_metadata, op.extra) < 0) {
    rc = -1;
  }
  // Add to journal
  if ((_d->journal->putPath(op.path) != static_cast<ssize_t>(op.path_len)) ||
      (_d->journal->putData(ts, op.encoded_metadata, op.extra) < 0) ||
      (_d->journal->flush() != 0)) {
    rc = -1;
  }
  if (rc != 0) {
    return rc;
  }
  _d->modified = true;
  _d->files_modified = true;
  return 0;
}

int Owner::getNextRecord(
    const char*     path,
    time_t          date,
    char*           fpath,
    Node**          fnode) const {
  size_t len = strlen(path);
  bool path_is_dir = false;
  if ((len > 0) && (path[len - 1] == '/')) {
    path_is_dir = true;
    len--;
  }
  bool failed = false;
  while (_d->original->searchPath() == 2) {
    if (aborting()) {
      failed = true;
      return -1;
    }
    if (_d->original->getEntry(NULL, fpath, fnode, date) <= 0) {
      failed = true;
      return -1;
    }
    // Start of path must match
    if (len == 0) {
      return 1;
    }
    int cmp = Path::compare(fpath, path, len);
    // Not reached
    if (cmp < 0) {
      delete *fnode;
      continue;
    }
    // Exceeded
    if (cmp > 0) {
      delete *fnode;
      return 0;
    }
    size_t flen = strlen(fpath);
    // Not reached, but on the right path
    if (flen < len) {
      delete *fnode;
      continue;
    }
    if ((flen > len) && (fpath[len] != '/')) {
      delete *fnode;
      return 0;
    }
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
  long long size;
  char checksum[List::MAX_CHECKSUM_LENGTH + 1];
  while (_d->original->fetchSizeChecksum(&size, checksum) > 0) {
    if (checksum[0] != '\0') {
      checksums.push_back(CompData(checksum, size));
    }
    if (aborting()) {
      break;
    }
  }
  release();
  return aborting() ? -1 : 0;
}
