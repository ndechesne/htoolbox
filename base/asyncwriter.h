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

#ifndef _ASYNCWRITER_H
#define _ASYNCWRITER_H

#include <ireaderwriter.h>

namespace htools {

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
  ssize_t read(void* buffer, size_t size);
  ssize_t write(const void* buffer, size_t size);
};

};

#endif // _ASYNCWRITER_H
