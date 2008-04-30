/*
     Copyright (C) 2008  Herve Fache

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

#include <iostream>

using namespace std;

#include "buffer.h"

using namespace hbackup;

int main(void) {
  Buffer buffer(10);
  const char* string1;
  char string2[10];
  int number;

  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer empty? " << (buffer.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << buffer.readable() << endl;

  string1 = "123456";
  memcpy(buffer.writer(), string1, strlen(string1));
  buffer.written(strlen(string1));
  cout << endl << "write: " << string1 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer empty? " << (buffer.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << buffer.readable() << endl;

  number = 3;
  string2[number] = '\0';
  memcpy(string2, buffer.reader(), number);
  buffer.readn(number);
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer empty? " << (buffer.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << buffer.readable() << endl;

  number = 3;
  string2[number] = '\0';
  memcpy(string2, buffer.reader(), number);
  buffer.readn(number);
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer empty? " << (buffer.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << buffer.readable() << endl;

  string1 = "ABCD";
  buffer.write(string1, strlen(string1));
  cout << endl << "write: " << string1 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer empty? " << (buffer.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << buffer.readable() << endl;

  number = 3;
  string2[number] = '\0';
  number = buffer.read(string2, number);
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer empty? " << (buffer.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << buffer.readable() << endl;

  number = 1;
  string2[number] = '\0';
  memcpy(string2, buffer.reader(), number);
  buffer.readn(number);
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer empty? " << (buffer.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << buffer.readable() << endl;

  string1 = "abcdefghij";
  memcpy(buffer.writer(), string1, strlen(string1));
  buffer.written(strlen(string1));
  cout << endl << "write: " << string1 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer empty? " << (buffer.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << buffer.readable() << endl;

  number = 6;
  string2[number] = '\0';
  memcpy(string2, buffer.reader(), number);
  buffer.readn(number);
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer empty? " << (buffer.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << buffer.readable() << endl;

  string1 = "!@#$%^";
  memcpy(buffer.writer(), string1, strlen(string1));
  buffer.written(strlen(string1));
  cout << endl << "write: " << string1 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer empty? " << (buffer.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << buffer.readable() << endl;

  string1 = "klmn";
  number = buffer.write(string1, strlen(string1));
  cout << endl << "wrote: " << number << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer empty? " << (buffer.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << buffer.readable() << endl;

  number = 100;
  string2[number] = '\0';
  number = buffer.read(string2, number);
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer empty? " << (buffer.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << buffer.readable() << endl;

  number = 100;
  string2[number] = '\0';
  number = buffer.read(string2, number);
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer empty? " << (buffer.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << buffer.readable() << endl;

  string1 = "nopqrstuvwxyz";
  number = buffer.write(string1, strlen(string1));
  cout << endl << "wrote: " << number << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer empty? " << (buffer.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << buffer.readable() << endl;

  string1 = "nopqrstuvwxyz";
  number = buffer.write(string1, strlen(string1));
  cout << endl << "wrote: " << number << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer empty? " << (buffer.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << buffer.readable() << endl;

  buffer.empty();
  cout << endl << "emptied" << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer empty? " << (buffer.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << buffer.readable() << endl;

  return 0;
}
