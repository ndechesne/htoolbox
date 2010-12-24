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
  int open();
  // Add data to message
  int write(uint8_t tag, const char* string, size_t len = 0);
  int write(uint8_t tag, int32_t number);
  // End message
  int close();
};

}

#endif // _SENDER_H
