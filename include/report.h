/*
    Copyright (C) 2008 - 2011  Hervé Fache

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _HREPORT_H
#define _HREPORT_H

#include <stdlib.h>
#include <stdarg.h>

#include <string>
#include <list>

#include <observer.h>
#include <tlv.h>
#include <tlv_helper.h>

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

  class Report : public Observer {
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

    class IOutput : public Observee {
    protected:
      Level             _level;
      bool              _open;
    public:
      IOutput() : _level(info), _open(false) {}
      virtual ~IOutput() {}
      virtual void setLevel(Level level) {
        _level = level;
        notifyObservers();
      }
      virtual Level level() const { return _level; }
      virtual int open() {
        _open = true;
        notifyObservers();
        return 0;
      }
      virtual int close() {
        _open = false;
        notifyObservers();
        return 0;
      }
      virtual bool isOpen() const { return _open; }
      virtual int log(
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
      int log(
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
        size_t          max_lines = 0,      // default: no limit
        size_t          backups   = 0,      // default: no backup
        bool            zip       = false); // default: do no zip backups
      ~FileOutput();
      int open();
      int close();
      bool isOpen() const;
      int log(
        const char*     file,
        size_t          line,
        Level           level,
        bool            temporary,
        size_t          ident,
        const char*     format,
        va_list*        args);
    };

    class TlvOutput : public IOutput {
      tlv::Sender&      _sender;
    public:
      // The log consumes 9 tags
      TlvOutput(tlv::Sender& sender) : _sender(sender) {
        open();
      }
      int log(
        const char*     file,
        size_t          line,
        Level           level,
        bool            temporary,
        size_t          ident,
        const char*     format,
        va_list*        args);
    };

    class TlvManager : public tlv::IReceptionManager {
      std::string           _file;
      size_t                _line;
      int                   _level;
      bool                  _temp;
      size_t                _indent;
      tlv::ReceptionManager _manager;
    public:
      TlvManager(IReceptionManager* next = NULL): IReceptionManager(next) {
        _manager.add(tlv::log_start_tag + 0, _file);
        _manager.add(tlv::log_start_tag + 1, &_line);
        _manager.add(tlv::log_start_tag + 2, &_level);
        _manager.add(tlv::log_start_tag + 3, &_temp);
        _manager.add(tlv::log_start_tag + 4, &_indent);
        _manager.add(tlv::log_start_tag + 5);
      }
      int submit(uint16_t tag, size_t size, const char* val);
    };

    class Filter : public IOutput, public Observer {
      char                  _name[32];
      IOutput*              _output;
      bool                  _auto_delete;
      size_t                _index;
      class Condition;
      std::list<Condition*> _conditions;
    public:
      Filter(const char* name, IOutput* output, bool auto_delete);
      ~Filter();
      // This shall trigger us back to check for macro check level
      void setLevel(Level level) {
        _output->setLevel(level);
        notifyObservers();
      }
      void notify();
      /* This function adds a special condition, either to add some log or to
       * remove some. Examples:
       * - I want to see regression output for file.cpp lines 40 to 123
       *   addCondition(true, "file.cpp", 40, 123, regression, regression);
       * - I want to ignore debug output from main.cpp
       *   addCondition(false, "main.cpp", 0, 0, debug, regression)
       * - I want to ignore regression output all files
       *   addCondition(false, "", 0, 0, regression, regression)
       */
      size_t addCondition(                      // returns condition index
        bool            accept,                 // false to reject logging
        const char*     file_name,              // file name to filter
        size_t          min_line = 0,           // start line to filter
        size_t          max_line = 0,           // end line to filter
        Level           min_level = alert,      // min log level to set
        Level           max_level = regression);// max log level to set
      void removeCondition(size_t index);
      int open() { return _output->open(); }
      int close() { return _output->close(); }
      bool isOpen() const { return _output->isOpen(); }
      int log(
        const char*     file,
        size_t          line,
        Level           level,
        bool            temp,
        size_t          ident,
        const char*     format,
        va_list*        args);
      void show(Level level) const;
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
    void add(IOutput* output) {
      output->registerObserver(this);
    }
    // Remove output from list
    void remove(IOutput* output) {
      output->unregisterObserver(this);
    }
    // Notify of level change
    void notify();
    // Set output verbosity level for all outputs
    void setLevel(Level level);
    // Get current output verbosity level
    Level level() const { return _level; }
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
    Filter            _con_filter;
  public:
    // Console filter accessor
    Filter& consoleFilter() { return _con_filter; }
  };

  extern Report report;
  extern __thread Report* tl_report;
}


#define hlog_is_worth(l) \
  (((htoolbox::tl_report != NULL) && \
   (htoolbox::tl_report->level() > htoolbox::report.level())) \
  ? ((l) <= htoolbox::tl_report->level()) \
  : ((l) <= htoolbox::report.level()))

#define hlog_generic(l, t, i, f, ...) \
  do { \
    if ((htoolbox::tl_report != NULL) && \
     ((l) <= htoolbox::tl_report->level())) \
      htoolbox::tl_report->log(__FILE__,__LINE__,(l),(t),(i),(f),##__VA_ARGS__); \
    if ((l) <= htoolbox::report.level()) \
      htoolbox::report.log(__FILE__,__LINE__,(l),(t),(i),(f),##__VA_ARGS__); \
  } while (0);

#define hlog_report(level, format, ...) \
  hlog_generic((level),false,0,(format),##__VA_ARGS__)

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


#define hlog_global_is_worth(l) \
  ((l) <= htoolbox::report.level())

#define hlog_global_generic(l, t, i, f, ...) \
  hlog_global_is_worth(l) \
  ? htoolbox::report.log(__FILE__,__LINE__,(l),(t),(i),(f),##__VA_ARGS__) \
  : 0

#define hlog_global_report(level, format, ...) \
  hlog_global_generic((level),false,0,(format),##__VA_ARGS__)

#define hlog_global_alert(format, ...) \
  hlog_global_generic(htoolbox::alert,false,0,(format),##__VA_ARGS__)

#define hlog_global_error(format, ...) \
  hlog_global_generic(htoolbox::error,false,0,(format),##__VA_ARGS__)

#define hlog_global_warning(format, ...) \
  hlog_global_generic(htoolbox::warning,false,0,(format),##__VA_ARGS__)

#define hlog_global_info(format, ...) \
  hlog_global_generic(htoolbox::info,false,0,(format),##__VA_ARGS__)

#define hlog_global_verbose(format, ...) \
  hlog_global_generic(htoolbox::verbose,false,0,(format),##__VA_ARGS__)

#define hlog_global_debug(format, ...) \
  hlog_global_generic(htoolbox::debug,false,0,(format),##__VA_ARGS__)

#define hlog_global_regression(format, ...) \
  hlog_global_generic(htoolbox::regression,false,0,(format),##__VA_ARGS__)


#define hlog_global_info_temp(format, ...) \
  hlog_global_generic(htoolbox::info,true,0,(format),##__VA_ARGS__)

#define hlog_global_verbose_temp(format, ...) \
  hlog_global_generic(htoolbox::verbose,true,0,(format),##__VA_ARGS__)

#define hlog_global_debug_temp(format, ...) \
  hlog_global_generic(htoolbox::debug,true,0,(format),##__VA_ARGS__)


#define hlog_global_verbose_arrow(indent, format, ...) \
  hlog_global_generic(htoolbox::verbose,false,(indent),(format),##__VA_ARGS__)

#define hlog_global_debug_arrow(indent, format, ...) \
  hlog_global_generic(htoolbox::debug,false,(indent),(format),##__VA_ARGS__)

#endif  // _HREPORT_H
