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

#ifndef _TLV_H
#define _TLV_H

#include <stdint.h>
#include <ireaderwriter.h>

namespace htoolbox {
namespace tlv {

// Tag max allowed value is 64999
// Message max length = 65535, buffers need to be 65536 long (\0 always added)

const uint16_t BUFFER_MAX = 65535;
const uint16_t log_start_tag = 65520;

class Sender {
  IReaderWriter&  _fd;
  bool            _started;
  bool            _failed;
  Sender(const Sender&);
  const Sender& operator=(const Sender&);
public:
  Sender(IReaderWriter& fd) : _fd(fd), _started(false), _failed(false) {}
  // Start message
  int start();
  // Check other end
  int check();
  // Add data to message
  int write(uint16_t tag, const void* buffer, size_t len);
  int write(uint16_t tag, int32_t number);
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
    CHECK = 2,
    DATA  = 3,
  };
private:
  IReaderWriter& _fd;
  Receiver(const Sender&);
  const Receiver& operator=(const Receiver&);
public:
  Receiver(IReaderWriter& fd) : _fd(fd) {}
  // Get message
  Type receive(
    uint16_t*   tag,
    size_t*     len,
    char        val[65536]);
};

}
}

#endif // _TLV_H
