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

#ifndef _IREADERWRITER_H
#define _IREADERWRITER_H

#include <unistd.h>

namespace htoolbox {

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
 *
 * The path() method tries to return the path to the underlying stream, if any.
 * Otherwise it must return NULL.
 *
 * The offset() method tries to return the offset of the underlying stream, if
 * any. Otherwise it must return -1.
 */
class IReaderWriter {
protected:
  //! \brief Next reader/writer in daisy chain
  IReaderWriter*    _child;
  //! \brief Whether to destroy daisy chained reader/writer at destruction
  bool              _delete_child;
public:
  //! \brief Default constructor
  IReaderWriter(IReaderWriter* child = NULL, bool delete_child = false)
  : _child(child), _delete_child(delete_child) {}
  //! \brief Destructor must also be virtual
  virtual ~IReaderWriter() {
    if (_delete_child) {
      delete _child;
    }
  }
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
  virtual ssize_t get(void* buffer, size_t size) = 0;
  //! \brief Write all given bytes to stream
  /*!
   * \param buffer      buffer from which to read the data
   * \param size        provided number of bytes
   * \return            negative number on failure, positive or null on success
  */
  virtual ssize_t put(const void* buffer, size_t size) = 0;
  //! \brief Get underlying stream path
  /*!
   * \return            path to the underlying stream
  */
  virtual const char* path() const {
    return _child == NULL ? NULL : _child->path();
  }
  //! \brief Get underlying stream offset
  /*!
   * \return            offset in the underlying stream
  */
  virtual long long offset() const {
    return _child == NULL ? -1 : _child->offset();
  }
};

};

#endif // _IREADERWRITER_H
