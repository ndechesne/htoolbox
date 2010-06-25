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
    info,         /*!< Normal information, typically if 'quiet' not selected */
    verbose,      /*!< Extra information, typically if 'verbose' selected */
    debug         /*!< Developper information, typically if 'debug' selected */
  };

  //! Message types
  enum MessageType {
    msg_standard, /*!< number represents arrow length */
    msg_errno,    /*!< number represents error no */
    msg_number    /*!< number represents a number (often a line number) */
  };

  //! \brief Function called throughout the code to output data
  /*!
    \param level        the level of verbosity associated with the message
    \param type         type of message (meaning of number)
    \param message      text of the message
    \param number       either line no or error no or arrow length or size
                        special cases:
                          -1: default
                          -2: do not print a colon (':') after the prepended text
                          -3: overwritable string
    \param prepend      prepended text (often the name of the file concerned)
    \return the output stream
  */
  extern void out_hidden(
    const char*     file,
    int             line,
    Level           level,
    MessageType     type,
    const char*     message,
    int             number,
    const char*     prepend);

  class Report {
    //! Current output criticality levels (default: info)
    Level           _out_level;
    struct Private;
    Private* const  _d;
    static size_t utf8_len(const char* s);
    int lock();
    int unlock();
  public:
    Report();
    ~Report();
    // Set output verbosity level
    void setLevel(Level level) { _out_level = level; }
    // Get current output verbosity level
    Level level() { return _out_level; }
    // Display message on standard output
    int log(
      const char*     file,
      int             line,
      Level           level,
      bool            temporary,  // erase this message with next one
      const char*     format,
      ...);
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


#define hlog_info_temp(f, ...) \
  hlog_generic(hreport::info,true,(f),##__VA_ARGS__)

#define hlog_verbose_temp(f, ...) \
  hlog_generic(hreport::verbose,true,(f),##__VA_ARGS__)

#define hlog_debug_temp(f, ...) \
  hlog_generic(hreport::debug,true,(f),##__VA_ARGS__)


#define hlog_generic(level, temp, format, ...) \
  hlog_is_worth(level) ? \
    hreport::report.log(__FILE__,__LINE__,(level),(temp),(format),##__VA_ARGS__) : 0


#define out(level,type,msg,no,prepend) \
  hreport::out_hidden(__FILE__,__LINE__,(level),(type),(msg),(no),(prepend))

#endif  // _HREPORT_H
