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

namespace hbackend {

class Sender {
  int _fd;
  Sender(const Sender&);
  const Sender& operator=(const Sender&);
public:
  Sender(int fd) : _fd(fd) {}
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
    ERROR,
    START,
    END,
    DATA,
  };
  typedef void (*read_cb_f)(
    Type        msg_type,
    uint8_t     tag,
    size_t      len,
    const char* val);
  struct            Private;
private:
  Private* const    _d;
  Receiver(const Sender&);
  const Receiver& operator=(const Receiver&);
public:
  Receiver(int fd, read_cb_f cb);
  // Start listening
  int open();
  // Stop listening
  int close();
};

}

#endif // _SENDER_H
