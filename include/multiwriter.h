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

#ifndef _MULTIWRITER_H
#define _MULTIWRITER_H

#include <ireaderwriter.h>

namespace htools {

//! \brief Writer to multiple streams at once
/*!
 * Writing to this stream will write to all underlying streams in sequence.
 * This is most efficient when inserting an AsyncWriter in between this and the
 * streams, so the writes actually happen concurrently.
 */
class MultiWriter : public IReaderWriter {
  struct         Private;
  Private* const _d;
public:
  //! \brief Constructor
  /*!
   * \param child        underlying stream to write to
   * \param delete_child whether to also delete child at destruction
  */
  MultiWriter(IReaderWriter* child, bool delete_child);
  ~MultiWriter();
  int open();
  int close();
  //! \brief Always fail to read, as this is a writer
  ssize_t read(void* buffer, size_t size);
  ssize_t write(const void* buffer, size_t size);
  //! \brief Returns first valid path found, if any
  const char* path() const;
  //! \brief Returns first valid offset found, if any
  long long offset() const;
  //! \brief Add more writers
  /*!
   * \param child        underlying stream to write to
   * \param delete_child whether to also delete child at destruction
  */
  void add(IReaderWriter* child, bool delete_child);
};

};

#endif // _MULTIWRITER_H
