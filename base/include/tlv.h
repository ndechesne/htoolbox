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

#ifndef _SENDER_H
#define _SENDER_H

#include <ireaderwriter.h>

namespace htools {
namespace tlv {

// Message max length = 65536

class Sender {
  IReaderWriter&  _fd;
  bool            _failed;
  Sender(const Sender&);
  const Sender& operator=(const Sender&);
public:
  Sender(IReaderWriter& fd) : _fd(fd), _failed(false) {}
  // Start message
  int start();
  // Add data to message
  int write(uint8_t tag, const void* buffer, size_t len = 0);
  int write(uint8_t tag, int32_t number);
  // End message
  int end();
};

class Receiver {
public:
  // Return pointer to string
  enum Type {
    ERROR = -1,
    END   = 0,
    START = 1,
    DATA  = 2,
  };
private:
  IReaderWriter& _fd;
  Receiver(const Sender&);
  const Receiver& operator=(const Receiver&);
public:
  Receiver(IReaderWriter& fd) : _fd(fd) {}
  // Get message
  Type receive(
    uint8_t*    tag,
    size_t*     len,
    char        val[65536]);
};

}
}

#endif // _SENDER_H
