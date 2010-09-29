/*
     Copyright (C) 2008-2010  Herve Fache

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

#include "string.h"
#include "wchar.h"

#include "buffer.h"

using namespace htools;

int main(void) {
  {
    cout << "Test: char (" << sizeof(char) << ")" << endl;
    cout << endl;
    Buffer<char>        buffer(10);
    BufferReader<char>  reader(buffer);
    const char*         string1;
    char                string2[16];
    size_t              number;

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
    memcpy(string2, reader.reader(), number);
    reader.readn(number);
    string2[number] = '\0';
    cout << endl << "read: " << number << ": " << string2 << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;

    number = 3;
    memcpy(string2, reader.reader(), number);
    reader.readn(number);
    string2[number] = '\0';
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
    number = reader.read(string2, number);
    string2[number] = '\0';
    cout << endl << "read: " << number << ": " << string2 << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;

    number = 1;
    memcpy(string2, reader.reader(), number);
    reader.readn(number);
    string2[number] = '\0';
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
    memcpy(string2, reader.reader(), number);
    reader.readn(number);
    string2[number] = '\0';
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
    number = reader.read(string2, number);
    string2[number] = '\0';
    cout << endl << "read: " << number << ": " << string2 << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;

    number = 100;
    number = reader.read(string2, number);
    string2[number] = '\0';
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
    BufferReader<char>  reader2(buffer);

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
    memcpy(string2, reader.reader(), number);
    reader.readn(number);
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

    number = 4;
    memcpy(string2, reader2.reader(), number);
    reader2.readn(number);
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

    number = 3;
    memcpy(string2, reader.reader(), number);
    reader.readn(number);
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

    number = 2;
    memcpy(string2, reader2.reader(), number);
    reader2.readn(number);
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
    memcpy(string2, reader2.reader(), number);
    reader2.readn(number);
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

    number = 3;
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

    number = 1;
    memcpy(string2, reader.reader(), number);
    reader.readn(number);
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
    memcpy(string2, reader.reader(), number);
    reader.readn(number);
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

    number = 10;
    memcpy(string2, reader2.reader(), number);
    reader2.readn(number);
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
  }
  cout << endl;
  {
    cout << "Test: wchar_t (" << sizeof(wchar_t) << ")" << endl;
    cout << endl;
    Buffer<wchar_t>       buffer(10);
    BufferReader<wchar_t> reader(buffer);
    const wchar_t*        string1;
    wchar_t               string2[16];
    char                  stringd[100];
    ssize_t               number;

    cout << "Test: one reader" << endl;
    cout << endl;

    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;

    string1 = L"123456";
    wmemcpy(buffer.writer(), string1, wcslen(string1));
    buffer.written(wcslen(string1));
    wcstombs(stringd, string1, 100);
    cout << endl << "write: " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;

    number = 3;
    wmemcpy(string2, reader.reader(), number);
    reader.readn(number);
    string2[number] = L'\0';
    wcstombs(stringd, string2, 100);
    cout << endl << "read: " << number << ": " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;

    number = 3;
    wmemcpy(string2, reader.reader(), number);
    reader.readn(number);
    string2[number] = L'\0';
    wcstombs(stringd, string2, 100);
    cout << endl << "read: " << number << ": " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;

    string1 = L"ABCD";
    buffer.write(string1, wcslen(string1));
    wcstombs(stringd, string1, 100);
    cout << endl << "write: " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;

    number = 3;
    number = reader.read(string2, number);
    string2[number] = L'\0';
    wcstombs(stringd, string2, 100);
    cout << endl << "read: " << number << ": " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;

    number = 1;
    wmemcpy(string2, reader.reader(), number);
    reader.readn(number);
    string2[number] = L'\0';
    wcstombs(stringd, string2, 100);
    cout << endl << "read: " << number << ": " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;

    string1 = L"abcdefghij";
    wmemcpy(buffer.writer(), string1, wcslen(string1));
    buffer.written(wcslen(string1));
    wcstombs(stringd, string1, 100);
    cout << endl << "write: " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;

    number = 6;
    wmemcpy(string2, reader.reader(), number);
    reader.readn(number);
    string2[number] = L'\0';
    wcstombs(stringd, string2, 100);
    cout << endl << "read: " << number << ": " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;

    string1 = L"!@#$%^";
    wmemcpy(buffer.writer(), string1, wcslen(string1));
    buffer.written(wcslen(string1));
    wcstombs(stringd, string1, 100);
    cout << endl << "write: " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;

    string1 = L"klmn";
    number = buffer.write(string1, wcslen(string1));
    cout << endl << "wrote: " << number << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;

    number = 100;
    number = reader.read(string2, number);
    string2[number] = L'\0';
    wcstombs(stringd, string2, 100);
    cout << endl << "read: " << number << ": " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;

    number = 100;
    number = reader.read(string2, number);
    string2[number] = L'\0';
    wcstombs(stringd, string2, 100);
    cout << endl << "read: " << number << ": " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;

    string1 = L"nopqrstuvwxyz";
    number = buffer.write(string1, /*wcslen(string1)*/5);
    cout << endl << "wrote: " << number << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;

    string1 = L"nopqrstuvwxyz";
    number = buffer.write(string1, wcslen(string1));
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
    BufferReader<wchar_t> reader2(buffer);

    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;
    cout << "Buffer2 readable = " << reader2.readable() << endl;

    string1 = L"123456";
    wmemcpy(buffer.writer(), string1, wcslen(string1));
    buffer.written(wcslen(string1));
    wcstombs(stringd, string1, 100);
    cout << endl << "write: " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;
    cout << "Buffer2 readable = " << reader2.readable() << endl;

    number = 3;
    wmemcpy(string2, reader.reader(), number);
    reader.readn(number);
    string2[number] = L'\0';
    wcstombs(stringd, string2, 100);
    cout << endl << "read: " << number << ": " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;
    cout << "Buffer2 readable = " << reader2.readable() << endl;

    number = 4;
    wmemcpy(string2, reader2.reader(), number);
    reader2.readn(number);
    string2[number] = L'\0';
    wcstombs(stringd, string2, 100);
    cout << endl << "read2: " << number << ": " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;
    cout << "Buffer2 readable = " << reader2.readable() << endl;

    number = 3;
    wmemcpy(string2, reader.reader(), number);
    reader.readn(number);
    string2[number] = L'\0';
    wcstombs(stringd, string2, 100);
    cout << endl << "read: " << number << ": " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;
    cout << "Buffer2 readable = " << reader2.readable() << endl;

    number = 2;
    wmemcpy(string2, reader2.reader(), number);
    reader2.readn(number);
    string2[number] = L'\0';
    wcstombs(stringd, string2, 100);
    cout << endl << "read2: " << number << ": " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;
    cout << "Buffer2 readable = " << reader2.readable() << endl;

    string1 = L"ABCD";
    buffer.write(string1, wcslen(string1));
    wcstombs(stringd, string1, 100);
    cout << endl << "write: " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;
    cout << "Buffer2 readable = " << reader2.readable() << endl;

    number = 4;
    wmemcpy(string2, reader2.reader(), number);
    reader2.readn(number);
    string2[number] = L'\0';
    wcstombs(stringd, string2, 100);
    cout << endl << "read2: " << number << ": " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;
    cout << "Buffer2 readable = " << reader2.readable() << endl;

    number = 3;
    number = reader.read(string2, number);
    string2[number] = L'\0';
    wcstombs(stringd, string2, 100);
    cout << endl << "read: " << number << ": " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;
    cout << "Buffer2 readable = " << reader2.readable() << endl;

    number = 1;
    wmemcpy(string2, reader.reader(), number);
    reader.readn(number);
    string2[number] = L'\0';
    wcstombs(stringd, string2, 100);
    cout << endl << "read: " << number << ": " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;
    cout << "Buffer2 readable = " << reader2.readable() << endl;

    string1 = L"abcdefghij";
    wmemcpy(buffer.writer(), string1, wcslen(string1));
    buffer.written(wcslen(string1));
    wcstombs(stringd, string1, 100);
    cout << endl << "write: " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;
    cout << "Buffer2 readable = " << reader2.readable() << endl;

    number = 6;
    wmemcpy(string2, reader.reader(), number);
    reader.readn(number);
    string2[number] = L'\0';
    wcstombs(stringd, string2, 100);
    cout << endl << "read: " << number << ": " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;
    cout << "Buffer2 readable = " << reader2.readable() << endl;

    number = 10;
    wmemcpy(string2, reader2.reader(), number);
    reader2.readn(number);
    string2[number] = L'\0';
    wcstombs(stringd, string2, 100);
    cout << endl << "read2: " << number << ": " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;
    cout << "Buffer2 readable = " << reader2.readable() << endl;

    string1 = L"!@#$%^";
    wmemcpy(buffer.writer(), string1, wcslen(string1));
    buffer.written(wcslen(string1));
    wcstombs(stringd, string1, 100);
    cout << endl << "write: " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;
    cout << "Buffer2 readable = " << reader2.readable() << endl;

    string1 = L"klmn";
    number = buffer.write(string1, wcslen(string1));
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
    number = reader.read(string2, number);
    string2[number] = L'\0';
    wcstombs(stringd, string2, 100);
    cout << endl << "read: " << number << ": " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;
    cout << "Buffer2 readable = " << reader2.readable() << endl;

    number = 100;
    number = reader.read(string2, number);
    string2[number] = L'\0';
    wcstombs(stringd, string2, 100);
    cout << endl << "read: " << number << ": " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;
    cout << "Buffer2 readable = " << reader2.readable() << endl;

    number = 100;
    number = reader2.read(string2, number);
    string2[number] = L'\0';
    wcstombs(stringd, string2, 100);
    cout << endl << "read2: " << number << ": " << stringd << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;
    cout << "Buffer2 readable = " << reader2.readable() << endl;

    string1 = L"nopqrstuvwxyz";
    number = buffer.write(string1, wcslen(string1));
    cout << endl << "wrote: " << number << endl;
    cout << "Buffer capacity = " << buffer.capacity() << endl;
    cout << "Buffer usage = " << buffer.usage() << endl;
    cout << "Buffer empty? " << (reader.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer2 empty? " << (reader2.isEmpty() ? "yes" : "no") << endl;
    cout << "Buffer full? " << (buffer.isFull() ? "yes" : "no") << endl;
    cout << "Buffer writeable = " << buffer.writeable() << endl;
    cout << "Buffer readable = " << reader.readable() << endl;
    cout << "Buffer2 readable = " << reader2.readable() << endl;

    string1 = L"nopqrstuvwxyz";
    number = buffer.write(string1, wcslen(string1));
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
  }

  return 0;
}
