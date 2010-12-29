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

#ifndef _HASHER_H
#define _HASHER_H

#include "ireaderwriter.h"

namespace htools {

//! \brief Hash computer
/*!
 * Reading/writing from/to this stream will read/write to the underlying stream,
 * with no modificationto the data.
 *
 * The specified hash will be computed from the data on the fly, and will be
 * valid after close() has been called.
 */
class Hasher : public IReaderWriter {
  struct         Private;
  Private* const _d;
public:
  enum Digest {
    md_null,
    md2,
    md4,
    md5,
    sha,
    sha1,
    dss,
    dss1,
    sha224,
    sha256,
    sha384,
    sha512,
    ripemd160
  };
  //! \brief Constructor
  /*!
   * \param child        underlying stream to read from or write to
   * \param delete_child whether to also delete child at destruction
   * \param digest       digest to compute
   * \param hash         pre-allocated buffer where to store the resulting hash
  */
  Hasher(IReaderWriter* child, bool delete_child, Digest digest, char* hash);
  ~Hasher();
  int open();
  int close();
  ssize_t read(void* buffer, size_t size);
  ssize_t write(const void* buffer, size_t size);
};

};

#endif // _HASHER_H
