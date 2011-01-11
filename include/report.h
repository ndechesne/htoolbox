/*
     Copyright (C) 2008-2011  Herve Fache

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

#include <stdlib.h>

namespace htoolbox {

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
    Level             _console_level;
    Level             _file_level;
    struct Private;
    Private* const    _d;
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
    // Add file name and lines to match to regression list
    void addRegressionCondition(
      const char*     file_name,
      size_t          min_line = 0,
      size_t          max_line = 0);
    // Display message on standard output
    int log(
      const char*     file,
      size_t          line,
      Level           level,
      bool            temporary,  // erase this message with next one
      size_t          indent,      // text indentation
      const char*     format,
      ...) __attribute__ ((format (printf, 7, 8)));
  };

  extern Report report;

}


#define hlog_report_is_worth(r, l) \
  ((l) <= (r).level())

#define hlog_report(r, l, f, ...) \
  hlog_report_is_worth((r), (l)) ? \
     (r).log(__FILE__,__LINE__,(l),false,0,(f),##__VA_ARGS__) : 0


#define hlog_is_worth(l) \
  ((l) <= htoolbox::report.level())

#define hlog_generic(l, t, i, f, ...) \
  hlog_is_worth(l) ? \
    htoolbox::report.log(__FILE__,__LINE__,(l),(t),(i),(f),##__VA_ARGS__) : 0


#define hlog_alert(format, ...) \
  hlog_generic(htoolbox::alert,false,0,(format),##__VA_ARGS__)

#define hlog_error(format, ...) \
  hlog_generic(htoolbox::error,false,0,(format),##__VA_ARGS__)

#define hlog_warning(format, ...) \
  hlog_generic(htoolbox::warning,false,0,(format),##__VA_ARGS__)

#define hlog_info(format, ...) \
  hlog_generic(htoolbox::info,false,0,(format),##__VA_ARGS__)

#define hlog_verbose(format, ...) \
  hlog_generic(htoolbox::verbose,false,0,(format),##__VA_ARGS__)

#define hlog_debug(format, ...) \
  hlog_generic(htoolbox::debug,false,0,(format),##__VA_ARGS__)

#define hlog_regression(format, ...) \
  hlog_generic(htoolbox::regression,false,0,(format),##__VA_ARGS__)


#define hlog_info_temp(format, ...) \
  hlog_generic(htoolbox::info,true,0,(format),##__VA_ARGS__)

#define hlog_verbose_temp(format, ...) \
  hlog_generic(htoolbox::verbose,true,0,(format),##__VA_ARGS__)

#define hlog_debug_temp(format, ...) \
  hlog_generic(htoolbox::debug,true,0,(format),##__VA_ARGS__)


#define hlog_verbose_arrow(indent, format, ...) \
  hlog_generic(htoolbox::verbose,false,(indent),(format),##__VA_ARGS__)

#define hlog_debug_arrow(indent, format, ...) \
  hlog_generic(htoolbox::debug,false,(indent),(format),##__VA_ARGS__)

#endif  // _HREPORT_H
