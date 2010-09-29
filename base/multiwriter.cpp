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

#include <list>

using namespace std;

#include <multiwriter.h>

using namespace hbackup;

struct MultiWriter::Private {
  struct Child {
    IReaderWriter* child;
    bool           delete_child;
    Child(IReaderWriter* c, bool d) : child(c), delete_child(d) {}
    ~Child() {
      if (delete_child) {
        delete child;
      }
    }
  };
  list<Child> children;
};

MultiWriter::MultiWriter(IReaderWriter* child, bool delete_child) :
  _d(new Private) {
    add(child, delete_child);
  }

MultiWriter::~MultiWriter() {
  delete _d;
}

int MultiWriter::open() {
  bool failed = false;
  list<Private::Child>::iterator it;
  for (it = _d->children.begin(); it != _d->children.end(); ++it) {
    if (it->child->open() < 0) {
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
      failed = true;
    }
  }
  if (failed) {
    return -1;
  }
  return 0;
}

ssize_t MultiWriter::read(void*, size_t) {
  return -1;
}

ssize_t MultiWriter::write(const void* buffer, size_t size) {
  bool failed = false;
  list<Private::Child>::iterator it;
  for (it = _d->children.begin(); it != _d->children.end(); ++it) {
    if (it->child->write(buffer, size) < 0) {
      failed = true;
      break;
    }
  }
  if (failed) {
    return -1;
  }
  return 0;
}

void MultiWriter::add(IReaderWriter* child, bool delete_child) {
  _d->children.push_back(Private::Child(child, delete_child));
}
