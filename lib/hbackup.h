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
    // These should go to error output
    alert,        /*!< Your're dead */
    error,        /*!< Big issue, but might recover */
    // These should go to standard output
    warning,      /*!< Non-blocking issue */
    info,         /*!< Normal information, typically if 'quiet' not selected */
    verbose,      /*!< Extra information, typically if 'verbose' selected */
    debug         /*!< Developper information, typically if 'debug' selected */
  };
  enum MessageType {
    msg_standard, /*!< number represents arrow length */
    msg_errno,    /*!< number represents error no */
    msg_number    /*!< number represents a number (often a line number) */
  };
  //! \brief Type for message callback function
  /*!
    \param level        the verbosity level
    \param type         the message type
    \param message      the message
    \param number       the arrows/error/line number
    \param prepend      the prepended text/file name
  */
  typedef void (*message_f)(
    VerbosityLevel  level,
    MessageType     type,
    const char*     message,
    int             number,
    const char*     prepend);

  //! \brief Set the message callback
  /*!
    \param message      the function to be called
  */
  void setMessageCallback(
    message_f       message);

  //! \brief Type for progress callback function
  /*!
    \param previous     the previous reported size
    \param current      the current size
    \param total        the total size
  */
  typedef void (*progress_f)(
    long long       previous,
    long long       current,
    long long       total);

  //! \brief Set the progress callback
  /*!
    \param progress     the function to be called
  */
  void setProgressCallback(
    progress_f      progress);

  //! \brief Set verbosity level
  /*!
    Sets the level of messages that the use should see
    \param level        the verbosity level
  */
  void setVerbosityLevel(
    VerbosityLevel  level);

  //! \brief Tell any process to stop
  /*!
    \param test         this parameter is for testing purposes only, DO NOT USE
  */
  void abort(
    unsigned short  test = 0);

  //! \brief We were told to abort
  /*!
    \param test         this parameter is for testing purposes only, DO NOT USE
  */
  bool aborting(
    unsigned short  test = 0);

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
      \param check_config just check given configuration file
      \return 0 on success, -1 on failure
    */
    int open(
      const char*   path,
      bool          user_mode     = false,
      bool          check_config  = false);
    //! \brief Close database
    /*!
      Makes sure the lists are up-to-date.
    */
    void close();
    //! \brief Check database lists for interrupted backup
    /*!
      Checks every client for interrupted backup.
      \return 0 on success, -1 on failure
    */
    int fix();
    //! \brief Check database for missing/obsolete files
    /*!
      Checks every entry in the database to detect missing and obsolete file
      data. This requires full DB access.
      \param remove       remove obsolete data
      \return 0 on success, -1 on failure
    */
    int scan(
      bool          remove        = true);
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
      \return 0 on success, -1 on failure
    */
    int backup(
      bool          initialize    = false);
    //! \brief List database contents selectively
    /*!
      Lists contents, using the given parameters as filters.
      \param records      list of elements to display
      \param path         path (if none given, list all client's paths)
      \param date         date (negative: use relative time from now, zero: all)
      \return 0 on success, -1 on failure
    */
    int getList(
      std::list<std::string>&
                    records,
      const char*   path          = "",
      time_t        date          = 0);
    //! \brief Restore specified database contents
    /*!
      Lists contents, using the given parameters as filters.
      \param destination  path where to restore the data
      \param path         path (if none given, restore all client's paths)
      \param date         date (negative: use relative time from now, zero: all)
      \return 0 on success, -1 on failure
    */
    int restore(
      const char*   destination,
      const char*   path          = "",
      time_t        date          = 0);
    //! \brief Show configuration
    /*!
      Lists recursively all configuration data. Only works if verbosity level
      is at least verbose.
      \param level        Length of arrow for top level items
    */
    void show(int level = 0) const;
  };
}

#endif  // _HBACKUP_H
