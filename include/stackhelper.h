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

#ifndef _STACKHELPER_H
#define _STACKHELPER_H

#include <ireaderwriter.h>

namespace htoolbox {

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
  ssize_t get(void* buffer, size_t size) {
    return _child->get(buffer, size);
  }
  ssize_t put(const void* buffer, size_t size) {
    return _child->put(buffer, size);
  }
  const char* path() const {
    return _child->path();
  }
  int64_t offset() const {
    return _child->offset();
  }
};

};

#endif // _STACKHELPER_H
