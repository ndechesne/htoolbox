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
#include <criticality.h>
#include <tlv.h>
#include <tlv_helper.h>

namespace htoolbox {
  class Report : public Observer {
    //! Current output criticality level (default: info)
    Criticality       _level;
    struct Private;
    Private* const    _d;
    static size_t utf8_len(const char* s);
  public:
    Report(const char* name);
    ~Report();
    const char* name() const;

    class IOutput : public Observee {
    protected:
      Criticality       _level;
      bool              _open;
    public:
      IOutput(const char* name) :
        Observee(name), _level(info), _open(false) {}
      virtual ~IOutput() {}
      virtual void setLevel(Level level) {
        _level = level;
        notifyObservers();
      }
      virtual Criticality level() const { return _level; }
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
        const char*     function,
        Level           level,
        int             flags,
        size_t          indentation,
        const char*     format,
        va_list*        args) = 0;
      virtual void show(Level level, size_t indentation) const = 0;
    };

    class ConsoleOutput : public IOutput {
      size_t            _size_to_overwrite;
    public:
      ConsoleOutput(const char* name) :
        IOutput(name), _size_to_overwrite(0) {}
      int log(
        const char*     file,
        size_t          line,
        const char*     function,
        Level           level,
        int             flags,
        size_t          indentation,
        const char*     format,
        va_list*        args);
      void show(Level level, size_t indentation = 0) const;
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
      const char* name() const;
      int open();
      int close();
      bool isOpen() const;
      int log(
        const char*     file,
        size_t          line,
        const char*     function,
        Level           level,
        int             flags,
        size_t          indentation,
        const char*     format,
        va_list*        args);
      void show(Level level, size_t indentation = 0) const;
    };

    class TlvOutput : public IOutput {
      tlv::Sender&      _sender;
    public:
      // The log consumes 9 tags
      TlvOutput(const char* name, tlv::Sender& sender) :
          IOutput(name), _sender(sender) {
        open();
      }
      int log(
        const char*     file,
        size_t          line,
        const char*     function,
        Level           level,
        int             flags,
        size_t          indentation,
        const char*     format,
        va_list*        args);
      void show(Level level, size_t indentation = 0) const;
    };

    class TlvManager : public tlv::IReceptionManager {
      std::string           _file;
      size_t                _line;
      std::string           _function;
      int                   _level;
      int                   _flags;
      size_t                _indent;
      tlv::ReceptionManager _manager;
    public:
      TlvManager(IReceptionManager* next = NULL): _manager(next) {
        _manager.add(tlv::log_start_tag + 0, _file);
        _manager.add(tlv::log_start_tag + 1, &_line);
        _manager.add(tlv::log_start_tag + 2, _function);
        _manager.add(tlv::log_start_tag + 3, &_level);
        _manager.add(tlv::log_start_tag + 4, &_flags);
        _manager.add(tlv::log_start_tag + 5, &_indent);
        _manager.add(tlv::log_start_tag + 6);
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
      const char* name() const { return _name; }
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
       * The rules are all parsed, in the order they were set: the last match
       * wins.
       */
      enum Mode {
        reject = 0, // if the rule matches, reject logging
        force  = 1, // if the rule matches, force logging
        accept = 2, // if the rule matches, force logging abiding to output level
      };
      static const char* ALL_FILES;
      size_t addCondition(                      // returns condition index
        Mode            mode,                   // accept/reject logging
        const char*     file_name,              // file name to filter
        size_t          min_line = 0,           // start line to filter
        size_t          max_line = 0,           // end line to filter
        Level           min_level = alert,      // min log level to set
        Level           max_level = regression);// max log level to set
      size_t addCondition(                      // returns condition index
        Mode            mode,                   // accept/reject logging
        const char*     file_name,              // file name to filter
        const char*     function_name,          // function name to filter
        Level           min_level = alert,      // min log level to set
        Level           max_level = regression);// max log level to set
      void removeCondition(size_t index);
      int open() { return _output->open(); }
      int close() { return _output->close(); }
      bool isOpen() const { return _output->isOpen(); }
      int log(
        const char*     file,
        size_t          line,
        const char*     function,
        Level           level,
        int             flags,
        size_t          indent,
        const char*     format,
        va_list*        args);
      void show(Level level, size_t indentation = 0) const;
    };

    // Log to console
    void startConsoleLog();
    // Stop logging to console
    void stopConsoleLog();
    // Set console log level
    void setConsoleLogLevel(Level level) { _console.setLevel(level); }
    // Get console log level
    Criticality consoleLogLevel() const { return _console.level(); }

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
    Criticality level() const { return _level; }
    // Flags
    enum {
      HLOG_TEMPORARY  = 1 << 0,
      HLOG_NOPREFIX   = 1 << 1,
    };
    // Display message on standard output
    int log(
      const char*     file,
      size_t          line,
      const char*     function,
      Level           level,
      int             flags,
      size_t          indent,      // text indentation
      const char*     format,
      ...) __attribute__ ((format (printf, 8, 9)));
    void show(Level level, size_t indentation = 0, bool show_closed = true) const;
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
      htoolbox::tl_report->log(__FILE__,__LINE__,__FUNCTION__,(l),(t),(i),(f),##__VA_ARGS__); \
    if ((l) <= htoolbox::report.level()) \
      htoolbox::report.log(__FILE__,__LINE__,__FUNCTION__,(l),(t),(i),(f),##__VA_ARGS__); \
  } while (0);

#define hlog_report(level, format, ...) \
  hlog_generic((level),0,0,(format),##__VA_ARGS__)

#define hlog_alert(format, ...) \
  hlog_report(htoolbox::alert,(format),##__VA_ARGS__)

#define hlog_error(format, ...) \
  hlog_report(htoolbox::error,(format),##__VA_ARGS__)

#define hlog_warning(format, ...) \
  hlog_report(htoolbox::warning,(format),##__VA_ARGS__)

#define hlog_info(format, ...) \
  hlog_report(htoolbox::info,(format),##__VA_ARGS__)

#define hlog_verbose(format, ...) \
  hlog_report(htoolbox::verbose,(format),##__VA_ARGS__)

#define hlog_debug(format, ...) \
  hlog_report(htoolbox::debug,(format),##__VA_ARGS__)

#define hlog_regression(format, ...) \
  hlog_report(htoolbox::regression,(format),##__VA_ARGS__)


#define hlog_info_temp(format, ...) \
  hlog_generic(htoolbox::info,htoolbox::Report::HLOG_TEMPORARY,0,(format),##__VA_ARGS__)

#define hlog_verbose_temp(format, ...) \
  hlog_generic(htoolbox::verbose,htoolbox::Report::HLOG_TEMPORARY,0,(format),##__VA_ARGS__)

#define hlog_debug_temp(format, ...) \
  hlog_generic(htoolbox::debug,htoolbox::Report::HLOG_TEMPORARY,0,(format),##__VA_ARGS__)


#define hlog_verbose_arrow(indent, format, ...) \
  hlog_generic(htoolbox::verbose,0,(indent),(format),##__VA_ARGS__)

#define hlog_debug_arrow(indent, format, ...) \
  hlog_generic(htoolbox::debug,0,(indent),(format),##__VA_ARGS__)


#define hlog_global_is_worth(l) \
  ((l) <= htoolbox::report.level())

#define hlog_global_generic(l, t, i, f, ...) \
  hlog_global_is_worth(l) \
  ? htoolbox::report.log(__FILE__,__LINE__,__FUNCTION__,(l),(t),(i),(f),##__VA_ARGS__) \
  : 0

#define hlog_global_report(level, format, ...) \
  hlog_global_generic((level),0,0,(format),##__VA_ARGS__)

#define hlog_global_alert(format, ...) \
  hlog_global_report(htoolbox::alert,(format),##__VA_ARGS__)

#define hlog_global_error(format, ...) \
  hlog_global_report(htoolbox::error,(format),##__VA_ARGS__)

#define hlog_global_warning(format, ...) \
  hlog_global_report(htoolbox::warning,(format),##__VA_ARGS__)

#define hlog_global_info(format, ...) \
  hlog_global_report(htoolbox::info,(format),##__VA_ARGS__)

#define hlog_global_verbose(format, ...) \
  hlog_global_report(htoolbox::verbose,(format),##__VA_ARGS__)

#define hlog_global_debug(format, ...) \
  hlog_global_report(htoolbox::debug,(format),##__VA_ARGS__)

#define hlog_global_regression(format, ...) \
  hlog_global_report(htoolbox::regression,(format),##__VA_ARGS__)


#define hlog_global_info_temp(format, ...) \
  hlog_global_generic(htoolbox::info,htoolbox::Report::HLOG_TEMPORARY,0,(format),##__VA_ARGS__)

#define hlog_global_verbose_temp(format, ...) \
  hlog_global_generic(htoolbox::verbose,htoolbox::Report::HLOG_TEMPORARY,0,(format),##__VA_ARGS__)

#define hlog_global_debug_temp(format, ...) \
  hlog_global_generic(htoolbox::debug,htoolbox::Report::HLOG_TEMPORARY,0,(format),##__VA_ARGS__)


#define hlog_global_verbose_arrow(indent, format, ...) \
  hlog_global_generic(htoolbox::verbose,0,(indent),(format),##__VA_ARGS__)

#define hlog_global_debug_arrow(indent, format, ...) \
  hlog_global_generic(htoolbox::debug,0,(indent),(format),##__VA_ARGS__)

#endif  // _HREPORT_H
