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
#include <cstdlib>
#include <cstring>

#include <unistd.h>

namespace hbackup {

template<class T>
class BufferReader;

//! \brief Ring buffer with concurrent read/write support
/*!
  This class implements a ring buffer allowing read and write operations to be
  effected simultaneously. It is simple and highly efficient.
*/
template<class T>
class Buffer {
  T*                _buffer_start;
  const T*          _buffer_end;
  unsigned int      _capacity;
  T*                _write_start;
  unsigned int      _write_count;
  unsigned int      _read_count;
  std::list<BufferReader<T>*>
                    _readers;
  unsigned int      _readers_size;
  bool              _readers_update;
  friend class      BufferReader<T>;
  // Registering
  void registerReader(BufferReader<T>* reader) {
    _readers.push_back(reader);
    _readers_size = _readers.size();
  }
  int unregisterReader(const BufferReader<T>* reader) {
    bool found = false;
    for (typename std::list<BufferReader<T>*>::iterator i = _readers.begin();
    i != _readers.end(); i++) {
      if (*i == reader) {
        _readers.erase(i);
        found = true;
        break;
      }
    }
    _readers_size = _readers.size();
    return found ? 0 : -1;
  }
public:
  //! \brief Constructor
  /*!
    \param size         size of the buffer to create, or 0 for none
  */
  Buffer(size_t size = 0) {
    _write_count = 0;
    _readers_size = 0;
    if (size > 0) {
      create(size);
    } else {
      _buffer_start = NULL;
    }
  }
  //! \brief Destructor
  ~Buffer() {
    if (exists()) {
      destroy();
    }
  }
  // Management
  //! \brief Create the buffer
  /*!
    \param size         size of the buffer to create
  */
  void create(size_t size = 102400) {
    // Multithread irrelevant: must be done synchronously
    _buffer_start = static_cast<T*>(malloc(size * sizeof(T)));
    _buffer_end   = &_buffer_start[size];
    _capacity     = size;
    empty();
  }
  //! \brief Destroy
  void destroy() {
    // Multithread irrelevant: must be done synchronously
    free(_buffer_start);
    _buffer_start = NULL;
  }
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
  bool exists() const {
    // Multithread safe
    return _buffer_start != NULL;
  }
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
  inline T* writer() {
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
  ssize_t write(const T* buffer, size_t size);
    // Multithread safe: only uses other methods
};

template<class T>
class BufferReader {
  Buffer<T>&         _buffer;
  const T*          _read_start;
  unsigned int      _read_count;
  bool              _auto_unreg;
  friend class      Buffer<T>;
public:
  //! \brief Constructor
  /*!
    \param buffer       buffer to read from
  */
  BufferReader(Buffer<T>& buffer, bool auto_unreg = true) :
      _buffer(buffer), _auto_unreg(auto_unreg) {
    // Multithread irrelevant: must be done synchronously
    _buffer.registerReader(this);
    empty();
  }
  //! \brief Destructor
  ~BufferReader() {
    // Multithread irrelevant: must be done synchronously
    if (_auto_unreg) {
      unregister();
    }
  }
  void unregister() {
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
  inline const T* reader() const {
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
  ssize_t read(T* buffer, size_t size) {
    // Multithread safe: only uses other methods
    size_t really = 0;
    T* next = buffer;
    while ((size > 0) && (readable() > 0)) {
      size_t can;
      if (size > readable()) {
        can = readable();
      } else {
        can = size;
      }
      next = static_cast<T*>(mempcpy(next, reader(), can * sizeof(T)));
      readn(can);
      really += can;
      size -= really;
    }
    return really;
  }
};

template<class T>
void Buffer<T>::empty() {
  _write_start    = _buffer_start;
  _write_count    = 0;
  _read_count     = 0;
  _readers_update = false;
  for (typename std::list<BufferReader<T>*>::iterator i = _readers.begin();
      i != _readers.end(); i++) {
    (*i)->empty();
  }
}

template<class T>
void Buffer<T>::update() {
  int max_used = 0;
  for (typename std::list<BufferReader<T>*>::iterator i = _readers.begin();
      i != _readers.end(); i++) {
    // Unread space for this reader
    int this_used = _write_count - (*i)->_read_count;;
    if (this_used > max_used) {
      max_used = this_used;
    }
  }
  _read_count = _write_count - max_used;
}

template<class T>
ssize_t Buffer<T>::write(const T* buffer, size_t size) {
  size_t really = 0;
  const T* next = buffer;
  while ((size > 0) && (writeable() > 0)) {
    size_t can;
    if (size > writeable()) {
      can = writeable();
    } else {
      can = size;
    }
    memcpy(writer(), next, can * sizeof(T));
    next += can;
    written(can);
    really += can;
    size -= really;
  }
  return really;
}

}

#endif
