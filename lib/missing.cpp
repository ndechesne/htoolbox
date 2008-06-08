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

using namespace hbackup;

struct Missing::Private {
  char*             path;
  vector<string>    data;
  bool              modified;
  progress_f        progress;
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
  if (_d->modified) {
    // Save list of missing items
    out(info, msg_standard, "Missing checksums list updated");
    Stream missing_list((path + ".part").c_str());
    if (missing_list.open("w")) {
      out(error, msg_errno, "Saving missing checksums list", errno);
      failed = true;
    }
//     missing_list.setProgressCallback(_d->progress);  need total size...
    int rc = 0;
    count = 0;
    for (unsigned int i = 0; i < _d->data.size(); i++) {
      if (_d->data[i][_d->data[i].size() - 1] != '+') {
        rc = missing_list.putLine(_d->data[i].c_str());
        count++;
      }
      if (rc < 0) break;
    }
    _d->data.clear();
    if (rc >= 0) {
      rc = missing_list.close();
    }
    if (rc >= 0) {
      // Backup file
      rc = rename(path.c_str(), (path + "~").c_str());
      // Put new version in place
      if ((rc == 0) || (errno == ENOENT)) {
        rc = rename(missing_list.path(), path.c_str());
      }
    }
    failed = (rc != 0);
  }
  if (count > 0) {
    stringstream s;
    s << "List of missing checksums contains " << count << " item(s)";
    out(info, msg_standard, s.str().c_str());
  }
  return failed ? -1 : 0;
}

void Missing::open(const char* path) {
  _d->path     = strdup(path);
  _d->modified = false;
}

int Missing::load() {
  bool failed = false;
  // Read list of missing checksums (it is ordered and contains no dup)
  out(verbose, msg_standard, "Reading list of missing checksums", -1);
  Stream missing_list(_d->path);
  if (! missing_list.open("r")) {
    missing_list.setProgressCallback(_d->progress);
    char*        checksum = NULL;
    unsigned int checksum_capacity = 0;
    while (missing_list.getLine(&checksum, &checksum_capacity) > 0) {
      _d->data.push_back(checksum);
      out(debug, msg_standard, checksum, 1);
    }
    free(checksum);
    missing_list.close();
  } else {
    out(warning, msg_errno, "Opening" , errno, "Missing checksums list");
    failed = true;
  }
  return failed ? -1 : 0;
}

bool Missing::empty() const {
  return size() == 0;
}

unsigned int Missing::size() const {
  return _d->data.size();
}

void Missing::push_back(const char* checksum) {
  _d->data.push_back(checksum);
  _d->modified = true;
}

string Missing::operator[](unsigned int id) {
  return _d->data[id];
}

int Missing::search(const char* checksum) {
  // Look for checksum in list (binary search)
  int start  = 0;
  int end    = _d->data.size() - 1;
  int middle;
  int found = -1;
  while (start <= end) {
    middle = (end + start) / 2;
    int cmp = _d->data[middle].compare(checksum);
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
  _d->data[id] += "+";
  _d->modified = true;
}

void Missing::show() const {
  if (! _d->data.empty()) {
    stringstream s;
    s << "Missing checksum(s): " << _d->data.size();
    out(warning, msg_standard, s.str().c_str());
    for (unsigned int i = 0; i < _d->data.size(); i++) {
      out(debug, msg_standard, _d->data[i].c_str(), 1);
    }
  }
}
