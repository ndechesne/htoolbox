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

#ifndef _ZIPWRITER_H
#define _ZIPWRITER_H

#include <ireaderwriter.h>

namespace htoolbox {

//! \brief Writer that zips on the fly
/*!
 * Writing to this stream will automatically zip the data before writing to the
 * underlying stream.
 */
class ZipWriter : public IReaderWriter {
  struct         Private;
  Private* const _d;
public:
  //! \brief Constructor
  /*!
   * \param child             underlying stream to write to
   * \param delete_child      whether to also delete child at destruction
   * \param compression_level the compression level to apply
  */
  ZipWriter(IReaderWriter* child, bool delete_child, int compression_level);
  ~ZipWriter();
  int open();
  int close();
  // Always fails to read as this is a writer
  ssize_t get(void* buffer, size_t size);
  ssize_t put(const void* buffer, size_t size);
};

};

#endif // _ZIPWRITER_H
