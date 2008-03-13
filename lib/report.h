/*
     Copyright (C) 2008  Herve Fache

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

#ifndef REPORT_H
#define REPORT_H

#include <string>
#include <sstream>

namespace hbackup {

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
  VerbosityLevel  level,
  MessageType     type,
  const char*     message,
  int             number        = -1,
  const char*     prepend       = NULL);

class Report {
  static Report*    _self;
  VerbosityLevel    _level;
  unsigned int      _size_to_overwrite;
  message_f         _message;
  Report() {
    // Display all non-debug messages
    _level             = info;
    _size_to_overwrite = 0;
    _message           = NULL;
  }
public:
  // Create/get current instance of this singleton
  static Report* self();
  // Set output verbosity level
  void setVerbosityLevel(
    VerbosityLevel  level);
  void setMessageCallback(
    message_f       message);
  // Report message
  void out(
    VerbosityLevel  level,
    MessageType     type,
    const char*     message,
    int             number  = -1, // Either line no or error no or arrow length
    const char*     prepend = NULL);
};

}

#endif
