/*
     Copyright (C) 2006-2008  Herve Fache

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

#include <list>

namespace hbackup {
  //! Verbosity level
  enum VerbosityLevel {
    // These go to error output
    alert,        /*!< Your're dead */
    error,        /*!< Big issue, but might recover */
    // These go to standard output
    warning,      /*!< Non-blocking issue */
    info,         /*!< Normal information, typically if 'quiet' not selected */
    verbose,      /*!< Extra information, typically if 'verbose' selected */
    debug         /*!< Developper information, typically if 'debug' selected */
  };

  //! \brief Set verbosity level
  /*!
    Sets the level of messages that the use should see
    \param level        the verbosity level
  */
  void setVerbosityLevel(VerbosityLevel level);

  //! \brief Output a string at given verbosity level, preceeded by an arrow
  /*!
    \param level        the level of verbosity associated with the message
    \param arrow_length the length of a preceeding arrow. use 0 to cancel the
                        default prefix for alert, error and warning
    \return the output stream
  */
  extern ostream& out(
    VerbosityLevel    level,
    int               arrow_length = -1);

  //! Termination required (string is for debug purposes only)
  extern int terminating(const char* string = NULL);

  //! \brief The HBackup class provides the full functionality of HBackup.
  /*!
    This class implements all the internals of HBackup, thus allowing for any
    front-end to be easily implemented on top of it. It depends on
    configuration files to behave as expected.
  */
  class HBackup {
    struct        Private;
    Private*      _d;
    //! \brief Read configuration file for server-mode backup, open database
    /*!
      Reads the file specified by config_path opens the backup database in
      read/write mode.
      \param config_path  the server configuration file
      \return 0 on success, -1 on failure
    */
    int readConfig(
      const char*   config_path);
  public:
    //! \brief Constructor
    HBackup();
    //! \brief Destructor
    ~HBackup();
    //! \brief Specify client
    /*!
      - Backup: adds specific client(s) to backup (excludes all non other
        clients)
      - List or restore: specifies client to list/restore the contents of
      \param client       the name of client
      \return 0 on success, -1 on failure
    */
    int addClient(
      const char*   client);
    //! \brief Open database
    /*!
      Open and recover database, initialize if told to.
      \param path         the path to the server configuration file, or
                          the user's home directory
      \param user_mode    backup in user mode
      \return 0 on success, -1 on failure
    */
    int open(
      const char*   path,
      bool          user_mode     = false);
    //! \brief Close database
    /*!
      Makes sure the lists are up-to-date.
    */
    void close();
    //! \brief Check database for missing/obsolete files
    /*!
      Checks every entry in the database to detect missing and obsolete file
      data. This requires full DB access.
      \param remove       remove obsolete data (safe default is false for now)
      \return 0 on success, -1 on failure
    */
    int scan(
      bool          remove        = false);
    //! \brief Check database for missing/corrupted files
    /*!
      Checks every entry in the database to detect corrupted file data.
      \return 0 on success, -1 on failure
    */
    int check();
    //! \brief Backup all accessible clients
    /*!
      Backs up all specified clients, using their configuration file. Also
      deals with expired entries.
      \param initialize   set-up DB if missing
      \param pretend      do not backup, just check configuration files
      \return 0 on success, -1 on failure
    */
    int backup(
      bool          initialize    = false,
      bool          pretend       = false);
    //! \brief List database contents selectively
    /*!
      Lists contents, using the given parameters as filters.
      \param records      list of elements to display
      \param client       client (if none given, list all clients)
      \param path         path (if none given, list all client's paths)
      \param date         date (if none given, show latest)
      \return 0 on success, -1 on failure
    */
    int getList(
      list<string>& records,
      const char*   client        = "",
      const char*   path          = "",
      time_t        date          = 0);
    //! \brief Restore specified database contents
    /*!
      Lists contents, using the given parameters as filters.
      \param destination  path where to restore the data
      \param client       client
      \param path         path (if none given, restore all client's paths)
      \param date         date (if none given, get latest)
      \return 0 on success, -1 on failure
    */
    int restore(
      const char*   destination,
      const char*   client,
      const char*   path          = "",
      time_t        date          = 0);
  };
}

#endif  // _HBACKUP_H
