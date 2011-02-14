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

#ifndef _ZIPPER_H
#define _ZIPPER_H

#include <ireaderwriter.h>

namespace htoolbox {

//! \brief ReaderWriter that (un)zips on the fly
/*!
 * Reading from/writing to this stream will automatically (un)zip the data
 * before reading from/writing to the underlying stream.
 */
class Zipper : public IReaderWriter {
  struct         Private;
  Private* const _d;
public:
  //! \brief Constructor
  /*!
   * \param child             underlying stream
   * \param delete_child      whether to also delete child at destruction
   * \param compression_level the compression level to apply, -1 to uncompress
  */
  Zipper(IReaderWriter* child, bool delete_child, int compression_level = -1);
  ~Zipper();
  int open();
  int close();
  ssize_t get(void* buffer, size_t size);
  ssize_t put(const void* buffer, size_t size);
};

};

#endif // _ZIPPER_H
