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

  /* Termination required (string is for debug purposes only) */
  extern int terminating(const char* string = NULL);

  class HBackup {
    struct        Private;
    Private*      _d;
  public:
    HBackup();
    ~HBackup();
    // Add specific client to backup (excludes all non other clients)
    int addClient(
      const char*   client);                // Client to take into account
    // Set user path (user-mode backup), open database
    int setUserPath(
      const char*   home_path);             // Path to user's home
    // Read configuration file (server-mode backup), open database
    int readConfig(
      const char*   config_path);           // Path to configuration file
    // Close database
    void close();
    // Check database for missing/corrupted files
    int check(
      bool          thorough      = false); // Check data checksum too (no)
    // Backup
    int backup(
      bool          config_check  = false); // Dry run (no)
    // List
    int getList(
      list<string>& records,                // List of elements to display
      const char*   client        = NULL,   // The client (list all)
      const char*   path          = NULL,   // The [start of the] path (list all)
      time_t        date          = 0);     // The date (latest)
    // Restore
    int restore(
      const char*   dest,                   // Where the restored path goes
      const char*   client        = NULL,   // The client
      const char*   path          = NULL,   // The path (restore all)
      time_t        date          = 0);     // The date (latest)
  };
}

#endif  // _HBACKUP_H
