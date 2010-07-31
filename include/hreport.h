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

/* The aim of this part is to concentrate the reporting stuff */

#ifndef _HREPORT_H
#define _HREPORT_H

namespace hreport {

  //! Criticality levels
  enum Level {
    // These go to error output
    alert,        /*!< Your're dead */
    error,        /*!< Big issue, but might recover */
    warning,      /*!< Non-blocking issue */
    // These go to standard output
    info,         /*!< Basic information */
    verbose,      /*!< Extra information */
    debug,        /*!< Developper information */
    regression    /*!< For regression testing purposes */
  };

  class Report {
    //! Current output criticality levels (default: info)
    Level           _console_level;
    Level           _file_level;
    struct Private;
    Private* const  _d;
    static size_t utf8_len(const char* s);
  public:
    Report();
    ~Report();
    static const char* levelString(Level level);
    static int stringToLevel(const char* str, Level* level);

    // Log to console
    void startConsoleLog();
    // Stop logging to console
    void stopConsoleLog();
    // Set console log level
    void setConsoleLogLevel(Level level) { _console_level = level; }
    // Get console log level
    Level consoleLogLevel() const { return _console_level; }

    // Log to file
    int startFileLog(
      const char*     name,
      size_t          max_lines = 0,  // default: no limit
      size_t          backups   = 0); // default: no backup
    // Stop logging to file
    void stopFileLog();
    // Set file log level
    void setFileLogLevel(Level level) { _file_level = level; }
    // Get file log level
    Level fileLogLevel() const { return _file_level; }

    // Set output verbosity level
    void setLevel(Level level) { _console_level = _file_level = level; }
    // Get current output verbosity level
    Level level() const {
      return _console_level > _file_level ? _console_level : _file_level;
    }
    // Display message on standard output
    int log(
      const char*     file,
      int             line,
      Level           level,
      bool            temporary,  // erase this message with next one
      size_t          ident,      // text identation
      const char*     format,
      ...) __attribute__ ((format (printf, 7, 8)));
  };

  extern Report report;

}

#define hlog_is_worth(l) \
  ((l) <= hreport::report.level())


#define hlog_alert(f, ...) \
  hlog_generic(hreport::alert,false,(f),##__VA_ARGS__)

#define hlog_error(f, ...) \
  hlog_generic(hreport::error,false,(f),##__VA_ARGS__)

#define hlog_warning(f, ...) \
  hlog_generic(hreport::warning,false,(f),##__VA_ARGS__)

#define hlog_info(f, ...) \
  hlog_generic(hreport::info,false,(f),##__VA_ARGS__)

#define hlog_verbose(f, ...) \
  hlog_generic(hreport::verbose,false,(f),##__VA_ARGS__)

#define hlog_debug(f, ...) \
  hlog_generic(hreport::debug,false,(f),##__VA_ARGS__)

#define hlog_regression(f, ...) \
  hlog_generic(hreport::regression,false,(f),##__VA_ARGS__)


#define hlog_info_temp(f, ...) \
  hlog_generic(hreport::info,true,(f),##__VA_ARGS__)

#define hlog_verbose_temp(f, ...) \
  hlog_generic(hreport::verbose,true,(f),##__VA_ARGS__)

#define hlog_debug_temp(f, ...) \
  hlog_generic(hreport::debug,true,(f),##__VA_ARGS__)


#define hlog_generic(level, temp, format, ...) \
  hlog_is_worth(level) ? \
    hreport::report.log(__FILE__,__LINE__,(level),(temp),0,(format),##__VA_ARGS__) : 0


#define hlog_verbose_arrow(i, f, ...) \
  hlog_generic_arrow(hreport::verbose,(i),(f),##__VA_ARGS__)

#define hlog_debug_arrow(i, f, ...) \
  hlog_generic_arrow(hreport::debug,(i),(f),##__VA_ARGS__)


#define hlog_generic_arrow(level, ident, format, ...) \
  hlog_is_worth(level) ? \
    hreport::report.log(__FILE__,__LINE__,(level),false,(ident),(format),##__VA_ARGS__) : 0

#endif  // _HREPORT_H
