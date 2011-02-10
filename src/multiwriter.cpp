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

#include <list>

using namespace std;

#include <multiwriter.h>

using namespace htoolbox;

struct MultiWriter::Private {
  struct Child {
    IReaderWriter* child;
    bool           delete_child;
    Child(IReaderWriter* c, bool d) : child(c), delete_child(d) {}
    // A child destructor here destroys the child as it is enlisted
  };
  list<Child> children;
  const char* path;
  Private() : path("") {}
};

MultiWriter::MultiWriter(IReaderWriter* child, bool delete_child) :
  _d(new Private) {
    add(child, delete_child);
  }

MultiWriter::~MultiWriter() {
  list<Private::Child>::iterator it;
  for (it = _d->children.begin(); it != _d->children.end(); ++it) {
    if (it->delete_child) {
      delete it->child;
    }
  }
  delete _d;
}

void MultiWriter::add(IReaderWriter* child, bool delete_child) {
  _d->children.push_back(Private::Child(child, delete_child));
}

int MultiWriter::open() {
  bool failed = false;
  list<Private::Child>::iterator it;
  for (it = _d->children.begin(); it != _d->children.end(); ++it) {
    if (it->child->open() < 0) {
      _d->path = it->child->path();
      failed = true;
      break;
    }
  }
  if (failed) {
    while (it != _d->children.begin()) {
      --it;
      it->child->close();
    }
    return -1;
  }
  return 0;
}

int MultiWriter::close() {
  bool failed = false;
  list<Private::Child>::iterator it;
  for (it = _d->children.begin(); it != _d->children.end(); ++it) {
    if (it->child->close() < 0) {
      _d->path = it->child->path();
      failed = true;
    }
  }
  if (failed) {
    return -1;
  }
  return 0;
}

ssize_t MultiWriter::get(void*, size_t) {
  return -1;
}

ssize_t MultiWriter::put(const void* buffer, size_t size) {
  bool failed = false;
  list<Private::Child>::iterator it;
  for (it = _d->children.begin(); it != _d->children.end(); ++it) {
    if (it->child->put(buffer, size) < static_cast<ssize_t>(size)) {
      _d->path = it->child->path();
      failed = true;
      break;
    }
  }
  if (failed) {
    return -1;
  }
  return size;
}

const char* MultiWriter::path() const {
  return _d->path;
}

long long MultiWriter::offset() const {
  list<Private::Child>::iterator it;
  for (it = _d->children.begin(); it != _d->children.end(); ++it) {
    long long offset = it->child->offset();
    if (offset >= 0) {
      return offset;
    }
  }
  return -1;
}
