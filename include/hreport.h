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
  extern void out(
    Level           level,
    MessageType     type,
    const char*     message,
    int             number        = -1,
    const char*     prepend       = NULL);

  class Report {
    static Report*    _self;
    //! Current output criticality levels (default: info)
    static Level      _out_level;
    char              _buffer[1024];
    unsigned int      _size_to_overwrite;
    static size_t utf8_len(const char* s);
    Report() : _size_to_overwrite(0) {}
  public:
    // Create/get current instance of this singleton
    static Report* self();
    // Set output verbosity level
    static void setLevel(Level level) { _out_level = level; }
    // Get current output verbosity level
    static Level level() { return _out_level; }
    // Display message on standard output
    static int out(
      Level           level,
      bool            temporary,  // erase this message with next one
      const char*     format,
      ...);
  };

}

#define hout_is_worth(l) \
  ((l) <= hreport::Report::level())

#define hout_generic(level, temp, format, ...) \
  hout_is_worth(level) ? \
    hreport::Report::out((level),(temp),(format),##__VA_ARGS__) : 0

#define hout_alert(t, f, ...) \
  hout_generic(hreport::alert,(t),(f),##__VA_ARGS__)

#define hout_error(t, f, ...) \
  hout_generic(hreport::error,(t),(f),##__VA_ARGS__)

#define hout_warning(t, f, ...) \
  hout_generic(hreport::warning,(t),(f),##__VA_ARGS__)

#define hout_info(t, f, ...) \
  hout_generic(hreport::info,(t),(f),##__VA_ARGS__)

#define hout_verbose(t, f, ...) \
  hout_generic(hreport::verbose,(t),(f),##__VA_ARGS__)

#define hout_debug(t, f, ...) \
  hout_generic(hreport::debug,(t),(f),##__VA_ARGS__)

#endif  // _HREPORT_H
