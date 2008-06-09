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

#include <vector>
#include <string>
#include <sstream>
#include <errno.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "report.h"
#include "missing.h"

namespace hbackup {
  enum MissingStatus {
    recovered,
    missing,
    inconsistent
  };

  struct MissingData {
    string            checksum;
    MissingStatus     status;
    MissingData(const string& c, MissingStatus s) : checksum(c), status(s) {}
    string line() const { return ((status == missing)? "M" : "I") + checksum; }
  };
}

using namespace hbackup;

struct Missing::Private {
  char*               path;
  vector<MissingData> data;
  progress_f          progress;
  bool                modified;
  bool                force_save;
};

Missing::Missing() : _d(new Private) {
  _d->path     = NULL;
  _d->progress = NULL;
}

Missing::~Missing() {
  delete _d;
}

void Missing::setProgressCallback(progress_f progress) {
  _d->progress = progress;
}

int Missing::close() {
  if (_d->path == NULL) {
    errno = EBADF;
    return -1;
  }
  string path = _d->path;
  free(_d->path);
  _d->path = NULL;
  bool failed = false;
  int  count  = _d->data.size();
  if (_d->modified || _d->force_save) {
    // Save list of missing items
    if (_d->modified) {
      out(info, msg_standard, "Missing checksums list updated");
    }
    Stream missing_list((path + ".part").c_str());
    if (missing_list.open("w")) {
      out(error, msg_errno, "Saving problematic checksums list", errno);
      failed = true;
    }
    int rc = 0;
    count = 0;
    for (unsigned int i = 0; i < _d->data.size(); i++) {
      if (_d->progress != NULL) {
        (*_d->progress)(i, i + 1, _d->data.size());
      }
      if (_d->data[i].status != recovered) {
        rc = missing_list.putLine(_d->data[i].line().c_str());
        count++;
      }
      if (rc < 0) break;
    }
    _d->data.clear();
    if (rc >= 0) {
      rc = missing_list.close();
    }
    if (rc >= 0) {
      // Put new version in place
      if ((rc == 0) || (errno == ENOENT)) {
        rc = rename(missing_list.path(), path.c_str());
      }
    }
    failed = (rc != 0);
  }
  if (count > 0) {
    stringstream s;
    s << "List of problematic checksums contains " << count << " item(s)";
    out(info, msg_standard, s.str().c_str());
  }
  return failed ? -1 : 0;
}

void Missing::open(const char* path) {
  _d->path       = strdup(path);
  _d->modified   = false;
  _d->force_save = false;
}

int Missing::load() {
  bool failed = false;
  // Read list of problematic checksums (it is ordered and contains no dup)
  out(verbose, msg_standard, "Reading list of problematic checksums", -1);
  Stream missing_list(_d->path);
  if (! missing_list.open("r")) {
    missing_list.setProgressCallback(_d->progress);
    char*        checksum = NULL;
    unsigned int checksum_capacity = 0;
    while (missing_list.getLine(&checksum, &checksum_capacity) > 0) {
      switch (checksum[0]) {
        case 'M':
          _d->data.push_back(MissingData(&checksum[1], missing));
          out(debug, msg_standard, &checksum[1], 1, "Missing");
          break;
        case 'I':
          _d->data.push_back(MissingData(&checksum[1], inconsistent));
          out(debug, msg_standard, &checksum[1], 1, "Inconsistent");
          break;
        default:
          // Backwards compatibility
          _d->data.push_back(MissingData(checksum, missing));
          out(debug, msg_standard, checksum, 1);
      }
    }
    free(checksum);
    missing_list.close();
  } else {
    out(warning, msg_errno, "Opening" , errno, "Missing checksums list");
    failed = true;
  }
  return failed ? -1 : 0;
}

void Missing::forceSave() {
  _d->force_save = true;
}

unsigned int Missing::size() const {
  return _d->data.size();
}

void Missing::setMissing(const char* checksum) {
  _d->data.push_back(MissingData(checksum, missing));
  _d->modified = true;
}

void Missing::setInconsistent(const char* checksum) {
  _d->data.push_back(MissingData(checksum, inconsistent));
  _d->modified = true;
}

const string& Missing::operator[](unsigned int id) const {
  return _d->data[id].checksum;
}

int Missing::search(const char* checksum) const {
  // Look for checksum in list (binary search)
  int start  = 0;
  int end    = _d->data.size() - 1;
  int middle;
  int found = -1;
  while (start <= end) {
    middle = (end + start) / 2;
    int cmp = _d->data[middle].checksum.compare(checksum);
    if (cmp > 0) {
      end   = middle - 1;
    } else
    if (cmp < 0) {
      start = middle + 1;
    } else
    {
      found = middle;
      break;
    }
  }
  return found;
}

void Missing::setRecovered(unsigned int id) {
  _d->data[id].status = recovered;
  _d->modified        = true;
}

void Missing::show() const {
  if (! _d->data.empty()) {
    stringstream s;
    s << "Problematic checksum(s): " << _d->data.size();
    out(warning, msg_standard, s.str().c_str());
    for (unsigned int i = 0; i < _d->data.size(); i++) {
      const char* checksum = _d->data[i].checksum.c_str();
      switch (_d->data[i].status) {
        case missing:
          out(debug, msg_standard, checksum, 1, "Missing");
          break;
        case inconsistent:
          out(debug, msg_standard, checksum, 1, "Inconsistent");
          break;
        default:
          out(debug, msg_standard, checksum, 1, "Recovered");
      }
    }
  }
}
