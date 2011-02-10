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

#ifndef _COPIER_H
#define _COPIER_H

#include <ireaderwriter.h>

namespace htoolbox {

//! \brief Copier
/*!
 * Reading from this stream also copies the data into all destination streams.
 * Writing data directly to the destination streams is also permitted.
 */
class Copier : public IReaderWriter {
  struct          Private;
  Private* const  _d;
  Copier(const Copier&);
  const Copier& operator=(const Copier&);
public:
  //! \brief Constructor
  /*!
   * \param source       underlying stream to read from
   * \param delete_child whether to also delete child at destruction
   * \param chunk_size   underlying buffer size
   */
  Copier(IReaderWriter* source, bool delete_source, size_t chunk_size);
  ~Copier();
  //! \brief Adds a destination stream
  /*!
   * \param dest         underlying stream to write to
   * \param delete_dest  whether to also delete dest at destruction
   */
  void addDest(IReaderWriter* dest, bool delete_dest);
  int open();
  int close();
  //! \brief Copies as it reads. Chunk size copy if size = 0 or size > chunk size
  ssize_t stream(void* buffer = NULL, size_t max_size = 0);
  //! \brief Copies as it reads. Full copy if size = 0
  ssize_t read(void* buffer = NULL, size_t size = 0);
  //! \brief Inserts data into destination
  ssize_t write(const void*, size_t);
  //! \brief Returns the path of the last error if any, or an empty string
  const char* path() const;
};

};

#endif // _COPIER_H
