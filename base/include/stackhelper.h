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

#ifndef _STACKHELPER_H
#define _STACKHELPER_H

#include <ireaderwriter.h>

namespace htools {

//! \brief Helper to create object on stack that destroys heap child
/*!
 * This helper provides an easy way to encapsulate a reader/writer created on
 * the heap with one object on the stack which will destroy the former
 * automatically, thus making memory leaks impossible.
 */
class StackHelper : public IReaderWriter {
public:
  //! \brief Constructor
  /*!
   * \param child        underlying stream to write to
   * \param delete_child whether to also delete child at destruction
  */
  StackHelper(IReaderWriter* child, bool delete_child) :
    IReaderWriter(child, delete_child) {}
  int open() {
    return _child->open();
  }
  int close() {
    return _child->close();
  }
  ssize_t read(void* buffer, size_t size) {
    return _child->read(buffer, size);
  }
  ssize_t write(const void* buffer, size_t size) {
    return _child->write(buffer, size);
  }
  const char* path() const {
    return _child->path();
  }
  long long offset() const {
    return _child->offset();
  }
};

};

#endif // _STACKHELPER_H
