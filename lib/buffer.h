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
  unsigned int      _capacity;
  char*             _write_start;
  unsigned int      _write_count;
  unsigned int      _read_count;
  std::list<BufferReader*>
                    _readers;
  unsigned int      _readers_size;
  bool              _readers_update;
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
    // Multithread irrelevant: must be done synchronously
  //! \brief Destroy
  void destroy();
    // Multithread irrelevant: must be done synchronously
  //! \brief Empty the buffer of all content
  void empty();
    // Multithread irrelevant: must be done synchronously
  //! \brief Check whether the write limit can be increased
  inline void reader_update(unsigned int reader_count) {
    // Multithread safe
    if (_readers_size == 1) {
      _read_count = reader_count;
    } else {
      _readers_update = true;
    }
  }
  //! \brief Check whether the write limit can be increased
  void update();
    // Multithread safe
  //! \brief Check whether the buffer was created
  /*!
      \return           true if was created, false otherwise
  */
  bool exists() const;
    // Multithread safe
  //! \brief Get buffer's capacity
  /*!
      \return           total buffer capacity
  */
  inline size_t capacity() const {
    // Multithread safe
    return _capacity;
  }
  //! \brief Get buffer's current usage
  /*!
      \return           buffer current usage
  */
  inline size_t usage() {
    // Multithread safe
    if (_readers_update) {
      update();
      _readers_update = false;
    }
    return _write_count - _read_count;
  }
  // Status
  //! \brief Check whether the buffer is full
  /*!
      \return           true if full, false otherwise
  */
  inline bool isFull() {
    // Multithread safe
    return usage() == _capacity;
  }
  // Writing
  //! \brief Get pointer to where to write
  /*!
      \return           pointer to where to write
  */
  inline char* writer() {
    // Multithread safe
    return _write_start;
  }
  //! \brief Get buffer's contiguous free space
  /*!
      \return           size of contiguous free space
  */
  inline size_t writeable() {
    // Multithread safe
    unsigned int total_free    = _capacity - usage();
    unsigned int straight_free = _buffer_end - _write_start;
    return (straight_free < total_free) ? straight_free : total_free;
  }
  //! \brief Set how much space was used
  /*!
    \param size         size written
  */
  inline void written(size_t size) {
    // Multithread safe
    // Update counter
    _write_count += size;
    // Update pointer
    _write_start += size;
    if (_write_start >= _buffer_end) {
      _write_start = _buffer_start;
    }
  }
  //! \brief Write data from an external buffer
  /*!
    \param buffer       pointer to the external buffer
    \param size         maximum size of data to get from the external buffer
    \return             size actually written
  */
  ssize_t write(const void* buffer, size_t size);
    // Multithread safe: only uses other methods
};

class BufferReader {
  Buffer&           _buffer;
  const char*       _read_start;
  unsigned int      _read_count;
  friend class      Buffer;
public:
  //! \brief Constructor
  /*!
    \param buffer       buffer to read from
  */
  BufferReader(Buffer& buffer) : _buffer(buffer) {
    // Multithread irrelevant: must be done synchronously
    _buffer.registerReader(this);
    empty();
  }
  //! \brief Destructor
  ~BufferReader() {
    // Multithread irrelevant: must be done synchronously
    _buffer.unregisterReader(this);
  }
  //! \brief Empty the buffer of all content
  inline void empty() {
    // Multithread irrelevant: must be done synchronously
    _read_start = _buffer._write_start;
    _read_count = _buffer._write_count;
  }
  //! \brief Check whether the buffer is empty
  /*!
      \return           true if empty, false otherwise
  */
  inline bool isEmpty() const {
    // Multithread safe: only _buffer._write_count can change, read only once
    return _read_count == _buffer._write_count;
  }
  //! \brief Get pointer to where to read
  /*!
      \return           pointer to where to read
  */
  inline const char* reader() const {
    // Multithread safe: nothing can change
    return _read_start;
  }
  //! \brief Get buffer's contiguous used space
  /*!
      \return           size of contiguous used space
  */
  inline size_t readable() const {
    // Multithread safe: only _buffer._write_count can change, read only once
    unsigned int total_free    = _buffer._write_count - _read_count;
    unsigned int straight_free = _buffer._buffer_end - _read_start;
    return (straight_free < total_free) ? straight_free : total_free;
  }
  //! \brief Set how much space was freed
  /*!
    \param size         size read
  */
  inline void readn(size_t size) {
    // Multithread safe: nothing can change
    // Update counter
    _read_count += size;
    // Tell write to find new end
    _buffer.reader_update(_read_count);
    // Update pointer
    _read_start += size;
    if (_read_start >= _buffer._buffer_end) {
      _read_start = _buffer._buffer_start;
    }
  }
  //! \brief Read data to an external buffer
  /*!
    \param buffer       pointer to the external buffer
    \param size         maximum size of data to put into the external buffer
    \return             size actually read
  */
  ssize_t read(void* buffer, size_t size);
    // Multithread safe: only uses other methods
};

}

#endif
