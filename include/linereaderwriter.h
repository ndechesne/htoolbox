/*
    Copyright (C) 2010 - 2011  Hervé Fache

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

#ifndef _LINEREADERWRITER_H
#define _LINEREADERWRITER_H

#include <ireaderwriter.h>

namespace htoolbox {

//! \brief Line per line reader
/*!
 * Allows to read a file line by line using getLine() or, if you need to specify
 * the delimiter, using getDelim().  The read() function may also be used at any
 * time.
 */
class LineReaderWriter : public IReaderWriter {
  struct         Private;
  Private* const _d;
public:
  //! \brief Constructor
  /*!
   * \param child        underlying stream used to get the actual data
   * \param delete_child whether to also delete child at destruction
  */
  LineReaderWriter(IReaderWriter* child, bool delete_child);
  ~LineReaderWriter();
  int open();
  int close();
  ssize_t get(void* buffer, size_t size);
  ssize_t put(const void* buffer, size_t size);
  //! \brief Read complete line from stream
  /*!
   * getLine() reads an entire line from the underlying stream, storing the
   * address of the buffer containing the text into *buffer.  A delimiter other
   * than newline can be specified as the delimiter argument.  The buffer is
   * null-terminated and includes the delimiter character, if one was found.
   *
   * \param buffer_p    pointer to the current/realloc'd buffer
   * \param capacity_p  *buffer_p's current/updated capacity
   * \param delim       delimiter to use
   * \return            negative number on failure, buffer size on success
  */
  ssize_t getLine(char** buffer_p, size_t* capacity_p, int delim = '\n');
  //! \brief Write a line to stream
  /*!
   * putLine() writes size bytes of buffer to the underlying stream, and adds
   * the delimiter character.  A delimiter other than newline can be specified
   * as the delimiter argument.
   *
   * \param buffer      buffer from which to read the data
   * \param size        provided number of bytes
   * \param delim       delimiter to use
   * \return            negative number on failure, size on success
  */
  ssize_t putLine(const void* buffer, size_t size, int delim = '\n');
};

};

#endif // _LINEREADERWRITER_H
