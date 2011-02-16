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

#ifndef _ASYNCWRITER_H
#define _ASYNCWRITER_H

#include <ireaderwriter.h>

namespace htoolbox {

//! \brief Background writer
/*!
 * Writing here actually takes place inside a thread, so write will only block
 * if the thread is busy, allowing for multithreaded stream operations.
 *
 * A call to close() will block until all the data has been written.
 *
 * Important note: the buffer given to write will still be in use by the writer
 * thread after write() has returned, so you should use two buffers
 * alternatively when using this module.
 */
class AsyncWriter : public IReaderWriter {
  struct         Private;
  Private* const _d;
  static void* _write_thread(void* data);
public:
  //! \brief Constructor
  /*!
   * \param child        underlying stream to write to
   * \param delete_child whether to also delete child at destruction
  */
  AsyncWriter(IReaderWriter* child, bool delete_child);
  ~AsyncWriter();
  int open();
  int close();
  //! \brief Always fails to read, as this is a writer
  ssize_t read(void* buffer, size_t size);
  //! \brief Always fails to get, as this is a writer
  ssize_t get(void* buffer, size_t size);
  ssize_t put(const void* buffer, size_t size);
};

};

#endif // _ASYNCWRITER_H
