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

#ifndef _MULTIWRITER_H
#define _MULTIWRITER_H

#include <ireaderwriter.h>

namespace htoolbox {

//! \brief Writer to multiple streams at once
/*!
 * Writing to this stream will write to all underlying streams in sequence.
 * This is most efficient when inserting an AsyncWriter in between this and the
 * streams, so the writes actually happen concurrently.
 */
class MultiWriter : public IReaderWriter {
  struct         Private;
  Private* const _d;
  MultiWriter(const MultiWriter&);
  const MultiWriter& operator=(const MultiWriter&);
public:
  //! \brief Constructor
  /*!
   * \param child        underlying stream to write to
   * \param delete_child whether to also delete child at destruction
   */
  MultiWriter(IReaderWriter* child, bool delete_child);
  ~MultiWriter();
  //! \brief Add more writers
  /*!
   * \param child        underlying stream to write to
   * \param delete_child whether to also delete child at destruction
   */
  void add(IReaderWriter* child, bool delete_child);
  int open();
  int close();
  //! \brief Always fails to read, as this is a writer
  ssize_t read(void* buffer, size_t size);
  //! \brief Always fails to get, as this is a writer
  ssize_t get(void* buffer, size_t size);
  ssize_t put(const void* buffer, size_t size);
  //! \brief Returns the path of the last error if any, or an empty string
  const char* path() const;
  //! \brief Returns the first valid offset found, if any
  long long offset() const;
};

};

#endif // _MULTIWRITER_H
