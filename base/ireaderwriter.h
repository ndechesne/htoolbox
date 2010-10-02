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

#ifndef _IREADERWRITER_H
#define _IREADERWRITER_H

#include <unistd.h>

namespace htools {

//! \brief Foundation interface for the reader/writer modules
/*!
 * This is the interface for a number of modules that use or transform data.
 *
 * They must implement open(), close(), read() and write() in  a standard
 * manner. They  must check and return errors, and set errno appropriately if
 * not set by the called party.
 *
 * They are completely defined at construction or just after, as the open()
 * method must not require any arguments.
 *
 * The read() and write() methods must get or put exactly the given number of
 * bytes, except of course when read reaches the end of the stream.
 *
 * The close() method should report an error if any previous call failed.
 */
class IReaderWriter {
public:
  //! \brief Destructor must also be virtual
  virtual ~IReaderWriter() {}
  //! \brief Open the underlying stream and create all necessary ressources
  /*!
   * \return            negative number on failure, positive or null on success
  */
  virtual int open() = 0;
  //! \brief Dispose of all ressources and close the underlying stream
  /*!
   * \return            negative number on failure, positive or null on success
  */
  virtual int close() = 0;
  //! \brief Read bytes from stream, no less than asked unless end reached
  /*!
   * \param buffer      buffer in which to store the data
   * \param size        required number of bytes
   * \return            negative number on failure, positive or null on success
  */
  virtual ssize_t read(void* buffer, size_t size) = 0;
  //! \brief Write all given bytes to stream
  /*!
   * \param buffer      buffer from which to read the data
   * \param size        provided number of bytes
   * \return            negative number on failure, positive or null on success
  */
  virtual ssize_t write(const void* buffer, size_t size) = 0;
};

};

#endif // _IREADERWRITER_H
