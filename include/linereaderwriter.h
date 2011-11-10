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
 * Allows to read a file line by line using getLine() whichs allows you to
 * specify up to too delimiters.  The read() function may also be used at any
 * time.
 */
class LineReaderWriter : public IReaderWriter {
  struct          Private;
  Private* const  _d;
  int64_t         _offset;
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
  ssize_t read(void* buffer, size_t size);
  ssize_t get(void* buffer, size_t size);
  ssize_t put(const void* buffer, size_t size);
  int64_t offset() const { return _offset; }
  int64_t childOffset() const;
  //! \brief Read complete line from stream
  /*!
   * getLine() reads an entire line from the underlying stream, storing the
   * address of the buffer containing the text into *buffer.  A delimiter other
   * than newline can be specified as the delimiter argument, and a second
   * delimiter expected right after the first can also be specified.  The buffer
   * isnull-terminated and includes the delimiter characters, if found.
   *
   * \param buffer_p    pointer to the current/realloc'd buffer
   * \param capacity_p  *buffer_p's current/updated capacity
   * \param delim       delimiter to use
   * \param delim2      second delimiter to use
   * \return            negative number on failure, buffer size on success
  */
  ssize_t getLine(
    char**          buffer_p,
    size_t*         capacity_p,
    int             delim = '\n',
    int             delim2 = -1);
  //! \brief Get number of delimiters found by getLine
  /*!
   * delimsWereFound() returns whether the delimiter(s) was (were) indeed found
   * during the last run of getLine().  If getLine() was never run, its value is
   * undefined.
   *
   * \return            (all) delimiter(s) was (were) found
  */
  bool delimsWereFound() const;
  //! \brief Write a line to stream
  /*!
   * putLine() writes size bytes of buffer to the underlying stream, and adds
   * the delimiter character(s).  A delimiter other than newline can be
   * specified as the delimiter argument, and a second delimiter can be
   * specified.
   *
   * \param buffer      buffer from which to read the data
   * \param size        provided number of bytes
   * \param delim       delimiter to use
   * \param delim2      second delimiter to use
   * \return            negative number on failure, size on success
  */
  ssize_t putLine(
    const void*     buffer,
    size_t          size,
    int             delim = '\n',
    int             delim2 = -1);
};

};

#endif // _LINEREADERWRITER_H
