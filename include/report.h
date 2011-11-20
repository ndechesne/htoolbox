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
        int             indentation,
        int             thread_id,
        size_t          buffer_size,
        const void*     buffer,
        const char*     format,
        va_list*        args) = 0;
      virtual void show(Level level, int indentation) const = 0;
    };

    class ConsoleOutput : public IOutput {
      size_t            _size_to_overwrite;
      size_t            _size_to_recover;
      int               _last_flags;
      Level             _last_level;
      int               _buffer_flags;
    public:
      ConsoleOutput(const char* name) : IOutput(name),
        _size_to_overwrite(0), _last_flags(0), _last_level(alert),
        _buffer_flags(HLOG_BUFFER_COUNT | HLOG_BUFFER_ASCII) {}
      void setBufferFlags(int flags) { _buffer_flags = flags; }
      int log(
        const char*     file,
        size_t          line,
        const char*     function,
        Level           level,
        int             flags,
        int             indentation,
        int             thread_id,
        size_t          buffer_size,
        const void*     buffer,
        const char*     format,
        va_list*        args);
      void show(Level level, int indentation = 0) const;
    };

    class FileOutput : public IOutput {
      struct Private;
      Private* const    _d;
      int               _buffer_flags;
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
      void setBufferFlags(int flags) { _buffer_flags = flags; }
      int log(
        const char*     file,
        size_t          line,
        const char*     function,
        Level           level,
        int             flags,
        int             indentation,
        int             thread_id,
        size_t          buffer_size,
        const void*     buffer,
        const char*     format,
        va_list*        args);
      void show(Level level, int indentation = 0) const;
    };

    class TlvOutput : public IOutput {
      tlv::Sender&      _sender;
    public:
      // The log consumes 10 tags
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
        int             indentation,
        int             thread_id,
        size_t          buffer_size,
        const void*     buffer,
        const char*     format,
        va_list*        args);
      void show(Level level, int indentation = 0) const;
    };

    class TlvManager : public tlv::IReceptionManager {
      tlv::ReceptionManager _manager;
      std::string           _file;
      size_t                _line;
      std::string           _function;
      int                   _level;
      int                   _flags;
      int                   _indent;
      int                   _thread_id;
      size_t                _previous_buffer_size;
      size_t                _buffer_size;
      char*                 _buffer;
    public:
      TlvManager(IReceptionManager* next = NULL): _manager(next),
          _previous_buffer_size(0), _buffer_size(0), _buffer(NULL) {
        _manager.add(tlv::log_start_tag + 0, _file);
        _manager.add(tlv::log_start_tag + 1, &_line);
        _manager.add(tlv::log_start_tag + 2, _function);
        _manager.add(tlv::log_start_tag + 3, &_level);
        _manager.add(tlv::log_start_tag + 4, &_flags);
        _manager.add(tlv::log_start_tag + 5, &_indent);
        _manager.add(tlv::log_start_tag + 6, &_thread_id);
        _manager.add(tlv::log_start_tag + 7, &_buffer_size);
        _manager.add(tlv::log_start_tag + 8, _buffer, _buffer_size);
        _manager.add(tlv::log_start_tag + 9);
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
        int             indentation,
        int             thread_id,
        size_t          buffer_size,
        const void*     buffer,
        const char*     format,
        va_list*        args);
      void show(Level level, int indentation = 0) const;
    };

    // Log to console
    void startConsoleLog();
    // Stop logging to console
    void stopConsoleLog();
    // Set console log level
    void setConsoleLogLevel(Level level) { _console.setLevel(level); }
    // Set console flags
    void setConsoleBufferFlags(int flags) { _console.setBufferFlags(flags); }
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
      HLOG_TEMPORARY    = 1 << 0,
      HLOG_NOPREFIX     = 1 << 1,
      HLOG_NOLINEFEED   = 1 << 2,
      HLOG_BUFFER_COUNT = 1 << 3,
      HLOG_BUFFER_ASCII = 1 << 4,
      HLOG_TLV_NOSEND   = 1 << 5,
    };
    // Display message on standard output
    int log(
      const char*     file,
      size_t          line,
      const char*     function,
      Level           level,
      int             flags,
      int             indentation,
      int             thread_id,
      size_t          buffer_size,
      const void*     buffer,
      const char*     format,
      ...) __attribute__ ((format (printf, 11, 12)));
    void show(Level level, int indentation = 0, bool show_closed = true) const;
  private:
    ConsoleOutput     _console;
    Filter            _con_filter;
  public:
    // Console filter accessor
    Filter& consoleFilter() { return _con_filter; }
  };
  // Global and thread-local reports
  extern Report report;
  extern __thread Report* tl_report;
  // Thread-local thread ID, used once set by application to a positive value
  extern __thread int tl_thread_id; // Recommended: 0 <= tl_thread_id < 10000000
}

#include <report_macros.h>

#endif  // _HREPORT_H
