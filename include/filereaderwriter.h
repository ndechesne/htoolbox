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

#ifndef _FILEREADERWRITER_H
#define _FILEREADERWRITER_H

#include <ireaderwriter.h>

namespace htoolbox {

//! \brief File reader/writer
/*!
 * This is the file accessor to actually read data from or store data to disk.
 */
class FileReaderWriter : public IReaderWriter {
  struct         Private;
  Private* const _d;
public:
  //! \brief Constructor
  /*!
   * \param path        path to the file to open a stream from
   * \param writer      whether write to or read from stream
  */
  FileReaderWriter(const char* path, bool writer);
  ~FileReaderWriter();
  int open();
  int close();
  ssize_t get(void* buffer, size_t size);
  ssize_t put(const void* buffer, size_t size);
  const char* path() const;
  long long offset() const;
};

};

#endif // _FILEREADERWRITER_H
