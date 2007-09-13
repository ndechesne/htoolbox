/*
     Copyright (C) 2006-2007  Herve Fache

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

#ifndef HBACKUP_H
#define HBACKUP_H

namespace hbackup {

/* Verbosity level */
extern int verbosity(void);

/* Termination required */
extern int terminating(void);

class HBackup {
  struct            Private;
  Private*          _d;
public:
  HBackup();
  ~HBackup();
  // Added specific client to backup (excludes all non added clients)
  int addClient(const char* client);
  // Read configuration file
  int readConfig(const char* path);
  // Check database
  int check(bool thorough = false);
  // Backup
  int backup(bool config_check = false);
};

}
#endif
