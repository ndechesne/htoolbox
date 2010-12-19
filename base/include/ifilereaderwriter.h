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

#ifndef _IFILEREADERWRITER_H
#define _IFILEREADERWRITER_H

#include <ireaderwriter.h>

namespace htools {

//! \brief Interface for the storage access reader/writer modules
/*!
 * This interface adds access to the path of the stream and the current offset.
 *
 * The documentation is the same as for IReaderWriter except for the two added
 * methods path() and offset() described below.
*/
class IFileReaderWriter : public IReaderWriter {
public:
  virtual ~IFileReaderWriter() {}
  //! \brief Open the stream
  /*!
   * \return            negative number on failure, positive or null on success
  */
  virtual int open() = 0;
  //! \brief Close the stream
  /*!
   * \return            negative number on failure, positive or null on success
  */
  virtual int close() = 0;
  virtual ssize_t read(void* buffer, size_t size) = 0;
  virtual ssize_t write(const void* buffer, size_t size) = 0;
  //! \brief Get stream path
  /*!
   * \return            path to the underlying stream
  */
  virtual const char* path() const = 0;
  //! \brief Get stream offset
  /*!
   * \return            offset in the underlying stream
  */
  virtual long long offset() const = 0;
};

};

#endif // _IFILEREADERWRITER_H
