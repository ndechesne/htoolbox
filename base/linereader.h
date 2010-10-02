/*
     Copyright (C) 2010  Herve Fache

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

#ifndef _LINEREADER_H
#define _LINEREADER_H

#include <ireaderwriter.h>

namespace htools {

//! \brief Line per line reader
/*!
 * Allows to read a file line by line, but only this way.
 *
 * Specific getLine() method must be used.
 */
class LineReader : public IReaderWriter {
  struct         Private;
  Private* const _d;
public:
  //! \brief Constructor
  /*!
   * \param child        underlying stream used to get the actual data
   * \param delete_child whether to also delete child at destruction
  */
  LineReader(IReaderWriter* child, bool delete_child);
  ~LineReader();
  int open();
  int close();
  //! \brief Always fail to read, only use getLine
  ssize_t read(void* buffer, size_t size);
  //! \brief Always fail to write, as this is a reader
  ssize_t write(const void* buffer, size_t size);
  //! \brief Read complete line from stream
  /*!
   * getLine() reads  an  entire line from stream, storing the address of the
   * buffer containing the text into *buffer.  The buffer is null-terminated
   * and includes the newline character, if one was found.
   *
   * \param buffer_p    pointer to the current/realloc'd buffer
   * \param capacity_p  *buffer_p's current/updated capacity
   * \return            negative number on failure, positive or null on success
  */
  inline ssize_t getLine(char** buffer_p, size_t* capacity_p) {
    return getDelim(buffer_p, capacity_p, '\n');
  }
  //! \brief Read complete line from stream using specific delimiter
  /*!
   * getdelim() works like getline(), except a line delimiter other than newline
   * can be specified as the delimiter argument.  As  with  getline(), a
   * delimiter character is not added if one was not present in the input before
   * end of file was reached.
   *
   * \param buffer_p    pointer to the current/realloc'd buffer
   * \param capacity_p  *buffer_p's current/updated capacity
   * \param delim       delimiter to use
   * \return            negative number on failure, positive or null on success
  */
  ssize_t getDelim(char** buffer_p, size_t* capacity_p, int delim);
};

};

#endif // _LINEREADER_H
