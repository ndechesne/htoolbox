/*
     Copyright (C) 2010  Herve Fache

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

#include <string>

#include "report.h"
#include "files.h"

using namespace htools;

#include "hbackup.h"
#include "configuration.h"
#include "backup.h"

using namespace hbackup;

struct Backup::Private {
  Path path;
  Private(const htools::Path& p) : path(p) {}
};

Backup::Backup(const htools::Path& path) : _d(new Private(path)) {
  hlog_debug("Backup::Backup(%s)", _d->path.c_str());
}

Backup::~Backup() {
  delete _d;
}

ConfigObject* Backup::configChildFactory(
    const vector<string>& /*params*/,
    const char*           /*file_path*/,
    size_t                /*line_no*/) {
  return NULL;
}

int Backup::add(const char* path) {
  hlog_debug("Backup::add(%s)", path);
  return 0;
}

int Backup::addHash(const char* hash) {
  hlog_debug("Backup::addHash(%s)", hash);
  return 0;
}

int Backup::removeHash(const char* hash) {
  hlog_debug("Backup::removeHash(%s)", hash);
  return 0;
}
