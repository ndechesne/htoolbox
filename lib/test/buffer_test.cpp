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
  Buffer        buffer(10);
  BufferReader  reader(buffer);
  const char*   string1;
  char          string2[10];
  int           number;

  cout << "Test: one reader" << endl;
  cout << endl;

  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;

  string1 = "123456";
  memcpy(buffer.writer(), string1, strlen(string1));
  buffer.written(strlen(string1));
  cout << endl << "write: " << string1 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;

  number = 3;
  string2[number] = '\0';
  memcpy(string2, reader.reader(), number);
  reader.readn(number);
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;

  number = 3;
  string2[number] = '\0';
  memcpy(string2, reader.reader(), number);
  reader.readn(number);
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;

  string1 = "ABCD";
  buffer.write(string1, strlen(string1));
  cout << endl << "write: " << string1 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;

  number = 3;
  string2[number] = '\0';
  number = reader.read(string2, number);
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;

  number = 1;
  string2[number] = '\0';
  memcpy(string2, reader.reader(), number);
  reader.readn(number);
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;

  string1 = "abcdefghij";
  memcpy(buffer.writer(), string1, strlen(string1));
  buffer.written(strlen(string1));
  cout << endl << "write: " << string1 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;

  number = 6;
  string2[number] = '\0';
  memcpy(string2, reader.reader(), number);
  reader.readn(number);
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;

  string1 = "!@#$%^";
  memcpy(buffer.writer(), string1, strlen(string1));
  buffer.written(strlen(string1));
  cout << endl << "write: " << string1 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;

  string1 = "klmn";
  number = buffer.write(string1, strlen(string1));
  cout << endl << "wrote: " << number << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;

  number = 100;
  string2[number] = '\0';
  number = reader.read(string2, number);
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;

  number = 100;
  string2[number] = '\0';
  number = reader.read(string2, number);
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;

  string1 = "nopqrstuvwxyz";
  number = buffer.write(string1, strlen(string1));
  cout << endl << "wrote: " << number << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;

  string1 = "nopqrstuvwxyz";
  number = buffer.write(string1, strlen(string1));
  cout << endl << "wrote: " << number << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;

  buffer.empty();
  cout << endl << "emptied" << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;


  cout << endl;

  cout << "Test: multiple readers" << endl;
  cout << endl;
  BufferReader  reader2(buffer);

  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  string1 = "123456";
  memcpy(buffer.writer(), string1, strlen(string1));
  buffer.written(strlen(string1));
  cout << endl << "write: " << string1 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  number = 3;
  string2[number] = '\0';
  memcpy(string2, reader.reader(), number);
  reader.readn(number);
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  number = 4;
  string2[number] = '\0';
  memcpy(string2, reader2.reader(), number);
  reader2.readn(number);
  cout << endl << "read2: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  number = 3;
  string2[number] = '\0';
  memcpy(string2, reader.reader(), number);
  reader.readn(number);
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  number = 2;
  string2[number] = '\0';
  memcpy(string2, reader2.reader(), number);
  reader2.readn(number);
  cout << endl << "read2: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  string1 = "ABCD";
  buffer.write(string1, strlen(string1));
  cout << endl << "write: " << string1 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  number = 4;
  string2[number] = '\0';
  memcpy(string2, reader2.reader(), number);
  reader2.readn(number);
  cout << endl << "read2: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  number = 3;
  string2[number] = '\0';
  number = reader.read(string2, number);
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  number = 1;
  string2[number] = '\0';
  memcpy(string2, reader.reader(), number);
  reader.readn(number);
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  string1 = "abcdefghij";
  memcpy(buffer.writer(), string1, strlen(string1));
  buffer.written(strlen(string1));
  cout << endl << "write: " << string1 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  number = 6;
  string2[number] = '\0';
  memcpy(string2, reader.reader(), number);
  reader.readn(number);
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  number = 10;
  string2[number] = '\0';
  memcpy(string2, reader2.reader(), number);
  reader2.readn(number);
  cout << endl << "read2: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  string1 = "!@#$%^";
  memcpy(buffer.writer(), string1, strlen(string1));
  buffer.written(strlen(string1));
  cout << endl << "write: " << string1 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  string1 = "klmn";
  number = buffer.write(string1, strlen(string1));
  cout << endl << "wrote: " << number << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  number = 100;
  string2[number] = '\0';
  number = reader.read(string2, number);
  string2[number] = '\0';
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  number = 100;
  string2[number] = '\0';
  number = reader.read(string2, number);
  string2[number] = '\0';
  cout << endl << "read: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  number = 100;
  string2[number] = '\0';
  number = reader2.read(string2, number);
  string2[number] = '\0';
  cout << endl << "read2: " << number << ": " << string2 << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  string1 = "nopqrstuvwxyz";
  number = buffer.write(string1, strlen(string1));
  cout << endl << "wrote: " << number << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  string1 = "nopqrstuvwxyz";
  number = buffer.write(string1, strlen(string1));
  cout << endl << "wrote: " << number << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  buffer.empty();
  cout << endl << "emptied" << endl;
  cout << "Buffer capacity = " << buffer.capacity() << endl;
  cout << "Buffer usage = " << buffer.usage() << endl;
  cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
  cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
  cout << "Buffer writeable = " << buffer.writeable() << endl;
  cout << "Buffer readable = " << reader.readable() << endl;
  cout << "Buffer2 readable = " << reader2.readable() << endl;

  return 0;
}
