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
  /*! \brief Logging and report manager
   *
   * This class implements a powerful way to report information and log it to
   * the console, to a file or to a different process using a network socket.
   */
  class Report : public Observer {
    //! Current output criticality level (default: info)
    Criticality       _level;
    struct Private;
    Private* const    _d;
    static size_t utf8_len(const char* s);
  public:
    /*! \brief Constructor
    *   \param name the name to call this object by
    */
    Report(const char* name);
    //! \brief Destructor
    ~Report();
    const char* name() const;

    /*! \brief Log output interface
     *
     * This is the interface for all log outputs.
     */
    class IOutput : public Observee {
    protected:
      Criticality       _level;
      bool              _open;
    public:
      /*! \brief Constructor
      *   \param name the name to call this object by
      */
      IOutput(const char* name) :
        Observee(name), _level(info), _open(false) {}
      //! \brief Destructor
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
      /*! \brief Log given data
      *   \param file         the file in which the call is
      *   \param line         the line at which the call is
      *   \param function     the function in which the call is
      *   \param level        the log level at which to print
      *   \param flags        the flags controlling the printing
      *   \param indentation  the indentation level to use
      *   \param thread_id    the ID of the thread in which context the call is
      *   \param buffer_size  the size of the buffer to print
      *   \param buffer       the pointer to the buffer to print
      *   \param format       the format of the message
      *   \param args         the arguments for the message
      */
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
      /*! \brief Show information about this object
      *   \param level        the log level at which to print
      *   \param indentation  the indentation level to use
      */
      virtual void show(Level level, int indentation) const = 0;
    };

    /*! \brief Console log output
     *
     * This output logs to the console.
     */
    class ConsoleOutput : public IOutput {
      size_t            _size_to_overwrite;
      size_t            _size_to_recover;
      int               _last_flags;
      Level             _last_level;
      int               _buffer_flags;
    public:
      /*! \brief Constructor
      *   \param name the name to call this object by
      */
      ConsoleOutput(const char* name) : IOutput(name),
        _size_to_overwrite(0), _last_flags(0), _last_level(alert),
        _buffer_flags(HLOG_BUFFER_COUNT | HLOG_BUFFER_ASCII) {}
      /*! \brief Change the buffer logging behaviour
      *   \param flags  the controlling flags
      */
      void setBufferFlags(int flags) { _buffer_flags = flags; }
      /*! \brief Log given data
      *   \param file         the file in which the call is
      *   \param line         the line at which the call is
      *   \param function     the function in which the call is
      *   \param level        the log level at which to print
      *   \param flags        the flags controlling the printing
      *   \param indentation  the indentation level to use
      *   \param thread_id    the ID of the thread in which context the call is
      *   \param buffer_size  the size of the buffer to print
      *   \param buffer       the pointer to the buffer to print
      *   \param format       the format of the message
      *   \param args         the arguments for the message
      */
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
      /*! \brief Show information about this object
      *   \param level        the log level at which to print
      *   \param indentation  the indentation level to use
      */
      void show(Level level, int indentation = 0) const;
    };

    /*! \brief File log output
     *
     * This output logs to a file.
     */
    class FileOutput : public IOutput {
      struct Private;
      Private* const    _d;
      int               _buffer_flags;
    public:
      /*! \brief Constructor
      *   \param name       the name to call this object by
      *   \param max_lines  maximum lines per log file [no limit]
      *   \param backups    the number of previous log files to keep [none]
      *   \param zip        whether to gzip the previous log files [no]
      */
      FileOutput(
        const char*     name,
        size_t          max_lines = 0,
        size_t          backups   = 0,
        bool            zip       = false);
      //! \brief Destructor
      ~FileOutput();
      const char* name() const;
      int open();
      int close();
      bool isOpen() const;
      void setBufferFlags(int flags) { _buffer_flags = flags; }
      /*! \brief Log given data
      *   \param file         the file in which the call is
      *   \param line         the line at which the call is
      *   \param function     the function in which the call is
      *   \param level        the log level at which to print
      *   \param flags        the flags controlling the printing
      *   \param indentation  the indentation level to use
      *   \param thread_id    the ID of the thread in which context the call is
      *   \param buffer_size  the size of the buffer to print
      *   \param buffer       the pointer to the buffer to print
      *   \param format       the format of the message
      *   \param args         the arguments for the message
      */
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
      /*! \brief Show information about this object
      *   \param level        the log level at which to print
      *   \param indentation  the indentation level to use
      */
      void show(Level level, int indentation = 0) const;
    };

    /*! \brief Network log output
     *
     * This output logs to a network socket using the TLV support.
     */
    class TlvOutput : public IOutput {
      tlv::Sender&      _sender;
    public:
      // The log consumes 10 tags
      /*! \brief Constructor
      *   \param name   the name to call this object by
      *   \param sender the sender object to use to send
      */
      TlvOutput(const char* name, tlv::Sender& sender) :
          IOutput(name), _sender(sender) {
        open();
      }
      /*! \brief Log given data
      *   \param file         the file in which the call is
      *   \param line         the line at which the call is
      *   \param function     the function in which the call is
      *   \param level        the log level at which to print
      *   \param flags        the flags controlling the printing
      *   \param indentation  the indentation level to use
      *   \param thread_id    the ID of the thread in which context the call is
      *   \param buffer_size  the size of the buffer to print
      *   \param buffer       the pointer to the buffer to print
      *   \param format       the format of the message
      *   \param args         the arguments for the message
      */
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
      /*! \brief Show information about this object
      *   \param level        the log level at which to print
      *   \param indentation  the indentation level to use
      */
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
      //! \brief Destructor
      ~Filter();
      const char* name() const { return _name; }
      // This shall trigger us back to check for macro check level
      void setLevel(Level level) {
        _output->setLevel(level);
        notifyObservers();
      }
      //! \brief Notify of a logging level change
      void notify();
      //! Condition mode of operation
      enum Mode {
        reject = 0, // if the rule matches, reject logging
        force  = 1, // if the rule matches, force logging
        accept = 2, // if the rule matches, force logging abiding to output level
      };
      static const char* ALL_FILES;
      //! \brief Add filter condition based on file and line number range
      /*! This function adds a special condition, either to add some log or to
       * remove some, based on file name and line numbers.
       *
       * Examples:
       * - I want to see regression output for file.cpp lines 40 to 123:
       *   addCondition(true, "file.cpp", 40, 123, regression, regression);
       * - I want to ignore debug output from main.cpp:
       *   addCondition(false, "main.cpp", 0, 0, debug, regression)
       * - I want to ignore regression output all files:
       *   addCondition(false, "", 0, 0, regression, regression)
       * The rules are all parsed, in the order they were set: the last match
       * wins.
       *
       * \return condition index
       */
      size_t addCondition(
        //! accept/reject logging
        Mode            mode,
        //! file name to filter
        const char*     file_name,
        //! start line to filter
        size_t          min_line = 0,
        //! end line to filter
        size_t          max_line = 0,
        //! min log level to filter
        Level           min_level = alert,
        //! max log level to filter
        Level           max_level = regression);
      //! \brief Add filter condition based on file and function
      /*! This function adds a special condition, either to add some log or to
       * remove some, based on file name and function name.
       *
       * \return condition index
       */
      size_t addCondition(
        //! accept/reject logging
        Mode            mode,
        //! file name to filter
        const char*     file_name,
        //! function name to filter
        const char*     function_name,
        //! min log level to filter
        Level           min_level = alert,
        //! max log level to filter
        Level           max_level = regression);
      /*! \brief Remove condition given by index
       *  \param index  index of condition as returned by addCondition
       */
      void removeCondition(size_t index);
      //! \brief Open underlying output
      int open() { return _output->open(); }
      //! \brief Close underlying output
      int close() { return _output->close(); }
      /*! \brief Check whether underlying output is open
       * \returns true if the underlying output is open, false otherwise
       */
      bool isOpen() const { return _output->isOpen(); }
      /*! \brief Log given data
      *   \param file         the file in which the call is
      *   \param line         the line at which the call is
      *   \param function     the function in which the call is
      *   \param level        the log level at which to print
      *   \param flags        the flags controlling the printing
      *   \param indentation  the indentation level to use
      *   \param thread_id    the ID of the thread in which context the call is
      *   \param buffer_size  the size of the buffer to print
      *   \param buffer       the pointer to the buffer to print
      *   \param format       the format of the message
      *   \param args         the arguments for the message
      */
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
      /*! \brief Show information about this object
      *   \param level        the log level at which to print
      *   \param indentation  the indentation level to use
      */
      void show(Level level, int indentation = 0) const;
    };

    //! \brief Start logging to console
    void startConsoleLog();
    //! \brief Stop logging to console
    void stopConsoleLog();
    //! \brief Set console log level
    void setConsoleLogLevel(Level level) { _console.setLevel(level); }
    //! \brief Set console flags
    void setConsoleBufferFlags(int flags) { _console.setBufferFlags(flags); }
    //! \brief Get console log level
    Criticality consoleLogLevel() const { return _console.level(); }

    /*! \brief Add output to list
     * \param output the output to add
     */
    void add(IOutput* output) {
      output->registerObserver(this);
    }
    /*! \brief Remove output from list
     * \param output the output to remove
     */
    void remove(IOutput* output) {
      output->unregisterObserver(this);
    }
    //! \brief Notify of a logging level change
    void notify();
    /*! \brief Set output verbosity level for all outputs
     * \param level the verbosity level to set
     */
    void setLevel(Level level);
    //! \brief Get current output verbosity level
    Criticality level() const { return _level; }
    //! \brief Log method flags
    enum {
      //! This message should be overwritten by the next
      HLOG_TEMPORARY    = 1 << 0,
      //! Do not prepend default prefix
      HLOG_NOPREFIX     = 1 << 1,
      //! Do not append a line feed (LF) character
      HLOG_NOLINEFEED   = 1 << 2,
      //! Do not show byte counter on left of buffer output
      HLOG_BUFFER_COUNT = 1 << 3,
      //! Do not show ASCII characters on right of buffer output
      HLOG_BUFFER_ASCII = 1 << 4,
      //! Do not send this other a network socket
      HLOG_TLV_NOSEND   = 1 << 5,
    };
    /*! \brief Log given data
    *   \param file         the file in which the call is
    *   \param line         the line at which the call is
    *   \param function     the function in which the call is
    *   \param level        the log level at which to print
    *   \param flags        the flags controlling the printing
    *   \param indentation  the indentation level to use
    *   \param thread_id    the ID of the thread in which context the call is
    *   \param buffer_size  the size of the buffer to print
    *   \param buffer       the pointer to the buffer to print
    *   \param format       the format of the message
    *   \param ...          the arguments for the message
    */
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
    /*! \brief Show information about this object
    *   \param level        the log level at which to print
    *   \param indentation  the indentation level to use
    *   \param show_closed  whether to show closed outputs too
    */
    void show(Level level, int indentation = 0, bool show_closed = true) const;
  private:
    ConsoleOutput     _console;
    Filter            _con_filter;
  public:
    //! \brief Console filter accessor
    Filter& consoleFilter() { return _con_filter; }
  };
  //! \brief Global report object
  extern Report report;
  //! \brief Thread-local report object
  extern __thread Report* tl_report;
  /*! \brief Thread-local thread ID
   *
   * Must be set by application to a positive value to be used
   */
  extern __thread int tl_thread_id; // Recommended: 0 <= tl_thread_id < 10000000
}

#include <report_macros.h>

#endif  // _HREPORT_H
