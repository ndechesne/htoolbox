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

#include <vector>
#include <string>
#include <sstream>

#include <stdio.h>
#include <errno.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "configuration.h"
#include "hreport.h"
#include "missing.h"

namespace hbackup {
  enum MissingStatus {
    recovered,
    missing,
    inconsistent
  };

  struct MissingData {
    string            checksum;
    long long         size;
    MissingStatus     status;
    MissingData(const string& c, long long l, MissingStatus s) :
        checksum(c), size(l), status(s) {}
    string line() const {
      stringstream s;
      s << checksum << '\t' << ((status == missing) ? "m" : "i")
        << '\t' << size;
      return s.str();
    }
  };
}

using namespace hbackup;
using namespace hreport;

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
  size_t count  = _d->data.size();
  if (_d->modified || _d->force_save) {
    // Save list of missing items
    if (_d->modified) {
      out(info, msg_standard, "Missing checksums list updated", -1, NULL);
    }
    Stream missing_list((path + ".part").c_str());
    if (missing_list.open(O_WRONLY)) {
      hlog_error("%s saving problematic checksums list '%s'", strerror(errno),
        missing_list.path());
      failed = true;
    }
    ssize_t rc = 0;
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
    out(info, msg_standard, s.str().c_str(), -1, NULL);
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
  out(verbose, msg_standard, "Reading list of problematic checksums", -1, NULL);
  Stream missing_list(_d->path);
  if (! missing_list.open(O_RDONLY)) {
    missing_list.setProgressCallback(_d->progress);
    char* line = NULL;
    size_t line_capacity = 0;
    while (missing_list.getLine(&line, &line_capacity) > 0) {
      vector<string> params;
      Config::extractParams(line, params);
      long long size;
      if (params.size() < 3) {
        if (params.size() == 1) {
          // Backwards compatibility
          _d->data.push_back(MissingData(line, -1, missing));
          out(debug, msg_standard, line, 1, NULL);
          continue;
        } else {
          out(error, msg_standard, "wrong number of parameters", -1,
            "Missing checksums list");
          break;
        }
      }
      const char* checksum = params[0].c_str();
      if (sscanf(params[2].c_str(), "%lld", &size) != 1) {
        out(error, msg_standard, "wrong type for size parameter", -1,
          "Missing checksums list");
        continue;
      }
      switch (params[1][0]) {
        case 'm':
          _d->data.push_back(MissingData(checksum, -1, missing));
          out(debug, msg_standard, checksum, 1, "Missing");
          break;
        case 'i':
          _d->data.push_back(MissingData(checksum, size, inconsistent));
          out(debug, msg_standard, checksum, 1, "Inconsistent");
          break;
        default:
          out(warning, msg_standard, "wrong identifier", -1,
            "Missing checksums list");
      }
    }
    free(line);
    missing_list.close();
  } else {
    hlog_warning("%s opening missing checksums list '%s'", strerror(errno),
      missing_list.path());
    failed = true;
  }
  return failed ? -1 : 0;
}

void Missing::forceSave() {
  _d->force_save = true;
}

size_t Missing::size() const {
  return _d->data.size();
}

bool Missing::isInconsistent(unsigned int id) const {
  return _d->data[id].status == inconsistent;
}

bool Missing::isMissing(unsigned int id) const {
  return _d->data[id].status == missing;
}

long long Missing::dataSize(unsigned int id) const {
  return _d->data[id].size;
}

void Missing::setMissing(const char* checksum) {
  _d->data.push_back(MissingData(checksum, -1, missing));
  _d->modified = true;
}

void Missing::setInconsistent(const char* checksum, long long size) {
  _d->data.push_back(MissingData(checksum, size, inconsistent));
  _d->modified = true;
}

const string& Missing::operator[](unsigned int id) const {
  return _d->data[id].checksum;
}

int Missing::search(const char* checksum) const {
  // Look for checksum in list (binary search)
  ssize_t start  = 0;
  ssize_t end    = _d->data.size() - 1;
  ssize_t middle;
  ssize_t found = -1;
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
  return static_cast<int>(found);
}

void Missing::setRecovered(unsigned int id) {
  _d->data[id].status = recovered;
  _d->modified        = true;
}

void Missing::show() const {
  if (! _d->data.empty()) {
    stringstream s;
    s << "Problematic checksum(s): " << _d->data.size();
    out(warning, msg_standard, s.str().c_str(), -1, NULL);
    for (unsigned int i = 0; i < _d->data.size(); i++) {
      const char* checksum = _d->data[i].checksum.c_str();
      switch (_d->data[i].status) {
        case missing:
          out(debug, msg_standard, "missing", 1, checksum);
          break;
        case inconsistent:
          out(debug, msg_standard, "inconsistent", 1, checksum);
          break;
        default:
          out(debug, msg_standard, "recovered", 1, checksum);
      }
    }
  }
}
