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

#ifndef _HBACKUP_H
#define _HBACKUP_H

namespace hbackup {
  /* Verbosity level */
  extern int verbosity();

  /* Termination required */
  extern int terminating();

  class HBackup {
    struct        Private;
    Private*      _d;
  public:
    HBackup();
    ~HBackup();
    // Add specific client to backup (excludes all non other clients)
    int addClient(
      const char*   client);               // Client name in configuration file
    // Set user path (user-mode backup)
    int setUserPath(
      const char*   home_path);          // Path to user's home
    // Read configuration file (server-mode backup)
    int readConfig(
      const char*   config_path);          // Path to configuration file
    // Check database for missing/corrupted files
    int check(
      bool          thorough     = false); // Check data checksum too (no)
    // Backup
    int backup(
      bool          config_check = false); // Dry run (no)
    // List
    int getList(
      list<string>& records,               // List of elements to display
      const char*   prefix,                // The prefix (list prefixes)
      const char*   path,                  // The path (list paths)
      time_t        date         = 0);     // The date (show)
    // Restore
    int restore(
      const char*   dest,                  // Where the restored path goes
      const char*   prefix,                // The prefix to restore
      const char*   path         = NULL,   // The path to restore (all)
      time_t        date         = 0);     // The date to restore (latest)
  };
}

#endif  // HBACKUP_H
