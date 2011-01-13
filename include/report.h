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
#include "stdarg.h"

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
    //! Current output criticality level (default: info)
    Level             _level;
    struct Private;
    Private* const    _d;
    static size_t utf8_len(const char* s);
  public:
    Report();
    ~Report();
    static const char* levelString(Level level);
    static int stringToLevel(const char* str, Level* level);

    class IOutput {
      Report*           _observer;
    protected:
      Level             _level;
      bool              _open;
    public:
      IOutput() : _observer(NULL), _level(info), _open(false) {}
      virtual ~IOutput() {}
      void registerObserver(Report* observer) {
        _observer = observer;
      }
      void setLevel(Level level) {
        _level = level;
        if (_observer != NULL) {
          _observer->notify();
        }
      }
      Level level() const { return _level; }
      virtual int open() {
        _open = true;
        if (_observer != NULL) {
          _observer->notify();
        }
        return 0;
      }
      virtual int close() {
        _open = false;
        if (_observer != NULL) {
          _observer->notify();
        }
        return 0;
      }
      virtual bool isOpen() const { return _open; }
      virtual ssize_t log(
        const char*     file,
        size_t          line,
        Level           level,
        bool            temporary,
        size_t          ident,
        const char*     format,
        va_list*        args) = 0;
    };

    class ConsoleOutput : public IOutput {
      size_t            _size_to_overwrite;
    public:
      ConsoleOutput() : _size_to_overwrite(0) {}
      ssize_t log(
        const char*     file,
        size_t          line,
        Level           level,
        bool            temporary,
        size_t          ident,
        const char*     format,
        va_list*        args);
    };

    class FileOutput : public IOutput {
      struct Private;
      Private* const    _d;
    public:
      FileOutput(
        const char*     name,
        size_t          max_lines = 0,  // default: no limit
        size_t          backups   = 0); // default: no backup
      ~FileOutput();
      int open();
      int close();
      bool isOpen() const;
      ssize_t log(
        const char*     file,
        size_t          line,
        Level           level,
        bool            temporary,
        size_t          ident,
        const char*     format,
        va_list*        args);
    };

    // Log to console
    void startConsoleLog();
    // Stop logging to console
    void stopConsoleLog();
    // Set console log level
    void setConsoleLogLevel(Level level) { _console.setLevel(level); }
    // Get console log level
    Level consoleLogLevel() const { return _console.level(); }

    // Add output to list
    void add(IOutput* output);
    // Remove output from list
    void remove(IOutput* output);
    // Notify of level change
    void notify();
    // Set output verbosity level for all outputs
    void setLevel(Level level);
    // Get current output verbosity level
    Level level() const { return _level; }
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
  private:
    ConsoleOutput     _console;
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