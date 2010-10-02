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

#ifndef _ZIPWRITER_H
#define _ZIPWRITER_H

#include <ireaderwriter.h>

namespace htools {

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
  ssize_t read(void* buffer, size_t size);
  ssize_t write(const void* buffer, size_t size);
};

};

#endif // _ZIPWRITER_H
