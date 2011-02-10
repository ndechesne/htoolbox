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

#ifndef _UNZIPREADER_H
#define _UNZIPREADER_H

#include <ireaderwriter.h>

namespace htoolbox {

//! \brief Reader that unzips on the fly
/*!
 * Reading from this stream will automatically unzip the data read from the
 * underlying stream.
 */
class UnzipReader : public IReaderWriter {
  struct         Private;
  Private* const _d;
public:
  //! \brief Constructor
  /*!
   * \param child        underlying stream to write to
   * \param delete_child whether to also delete child at destruction
  */
  UnzipReader(IReaderWriter* child, bool delete_child);
  ~UnzipReader();
  int open();
  int close();
  ssize_t get(void* buffer, size_t size);
  // Always fails to write as this is a reader
  ssize_t put(const void* buffer, size_t size);
};

};

#endif // _UNZIPREADER_H
