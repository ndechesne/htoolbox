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

#ifndef _QUEUE_H
#define _QUEUE_H

namespace htoolbox {

class Queue {
  struct         Private;
  Private* const _d;
public:
  Queue(const char* name, size_t max_size = 1);
  ~Queue();
  void open();
  void close();
  void wait();
  bool empty() const;
  size_t size() const;
  int push(void* data);
  int pop(void** data);
  void signal();
};

};

#endif // _QUEUE_H
