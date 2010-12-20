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

#ifndef _BACKUP_H
#define _BACKUP_H

namespace hbackup {

class Backup : public Data::Backup {
  struct            Private;
  Private* const    _d;
public:
  Backup(const htools::Path& path);
  ~Backup();
  virtual htools::ConfigObject* configChildFactory(
    const vector<string>& params,
    const char*           file_path = NULL,
    size_t                line_no   = 0);
  int add(const char* path);
  int addHash(const char* hash);
  int removeHash(const char* hash);
};

};

#endif // _BACKUP_H

