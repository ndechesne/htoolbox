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

#ifndef BUFFER_H
#define BUFFER_H

#include <list>
#include <unistd.h>

namespace hbackup {

class BufferReader;

//! \brief Ring buffer with concurrent read/write support
/*!
  This class implements a ring buffer allowing read and write operations to be
  effected simultaneously. It is simple and highly efficient.
*/
class Buffer {
  char*             _buffer_start;
  const char*       _buffer_end;
  char*             _write_start;
  const char*       _write_end;
  unsigned int      _write_id;
  bool              _full;
  std::list<BufferReader*>
                    _readers;
  unsigned int      _readers_size;
  friend class      BufferReader;
  // Registering
  void registerReader(BufferReader* reader);
  int unregisterReader(const BufferReader* reader);
public:
  //! \brief Constructor
  /*!
    \param size         size of the buffer to create, or 0 for none
  */
  Buffer(size_t size = 0);
  //! \brief Destructor
  ~Buffer();
  // Management
  //! \brief Create the buffer
  /*!
    \param size         size of the buffer to create
  */
  void create(size_t size = 102400);
  //! \brief Destroy
  void destroy();
  //! \brief Empty the buffer of all content
  void empty();
  //! \brief Check whether the write limit can be increased
  void update();
  //! \brief Check whether the buffer was created
  /*!
      \return           true if was created, false otherwise
  */
  bool exists() const;
  //! \brief Get buffer's capacity
  /*!
      \return           total buffer capacity
  */
  size_t capacity() const {
    return _buffer_end - _buffer_start;
  }
  //! \brief Get buffer's current usage
  /*!
      \return           buffer current usage
  */
  size_t usage() const;
  // Status
  //! \brief Check whether the buffer is full
  /*!
      \return           true if full, false otherwise
  */
  bool isFull() const {
    return _full;
  }
  // Writing
  //! \brief Get pointer to where to write
  /*!
      \return           pointer to where to write
  */
  char* writer() {
    return _write_start;
  }
  //! \brief Get buffer's contiguous free space
  /*!
      \return           size of contiguous free space
  */
  size_t writeable() const;
  //! \brief Set how much space was used
  /*!
    \param size         size written
  */
  void written(size_t size);
  //! \brief Write data from an external buffer
  /*!
    \param buffer       pointer to the external buffer
    \param size         maximum size of data to get from the external buffer
    \return             size actually written
  */
  ssize_t write(const void* buffer, size_t size);
};

class BufferReader {
  Buffer&           _buffer;
  const char*       _read_start;
  unsigned int      _empty_id;
  friend class      Buffer;
public:
  //! \brief Constructor
  /*!
    \param buffer       buffer to read from
  */
  BufferReader(Buffer& buffer) : _buffer(buffer) {
    _buffer.registerReader(this);
    empty();
  }
  //! \brief Destructor
  ~BufferReader() {
    _buffer.unregisterReader(this);
  }
  //! \brief Empty the buffer of all content
  void empty();
  //! \brief Check whether the buffer is empty
  /*!
      \return           true if empty, false otherwise
  */
  bool isEmpty() const {
    return _empty_id == _buffer._write_id;
  }
  //! \brief Get pointer to where to read
  /*!
      \return           pointer to where to read
  */
  const char* reader() const;
  //! \brief Get buffer's contiguous used space
  /*!
      \return           size of contiguous used space
  */
  size_t readable() const;
  //! \brief Set how much space was freed
  /*!
    \param size         size read
  */
  void readn(size_t size);
  //! \brief Read data to an external buffer
  /*!
    \param buffer       pointer to the external buffer
    \param size         maximum size of data to put into the external buffer
    \return             size actually read
  */
  ssize_t read(void* buffer, size_t size);
};

}

#endif
