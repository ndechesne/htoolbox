/*
     Copyright (C) 2006-2007  Herve Fache

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
#include <string>
#include <sys/stat.h>
#include <errno.h>

using namespace std;

#include "strings.h"
#include "files.h"
#include "hbackup.h"

using namespace hbackup;

int parseList(Directory *d, const char* cur_dir) {
  list<Node*>::iterator i = d->nodesList().begin();
  while (i != d->nodesList().end()) {
    Node* payload = *i;
    switch (payload->type()) {
      case 'f': {
        File *f = new File(*payload);
        delete *i;
        *i = f;
      }
      break;
      case 'l': {
        Link *l = new Link(*payload);
        delete *i;
        *i = l;
      }
      break;
      case 'd': {
        Directory *di = new Directory(*payload);
        delete *i;
        *i = di;
        char* dir_path = Node::path(cur_dir, di->name());
        if (! di->createList()) {
          parseList(di, dir_path);
        } else {
          cerr << "Failed to create list for " << di->name() << " in "
            << cur_dir << endl;
        }
        free(dir_path);
      }
      break;
    }
    i++;
  }
  return 0;
}

void showList(const Directory* d, int level = 0);

void defaultShowFile(const Node* g) {
  cout << "Oth.: " << g->name()
    << ", path = " << g->path()
    << ", type = " << g->type()
    << ", mtime = " << (g->mtime() != 0)
    << ", size = " << g->size()
    << ", uid = " << (int)(g->uid() != 0)
    << ", gid = " << (int)(g->gid() != 0)
    << oct << ", mode = " << g->mode() << dec
    << endl;
}

void showFile(const Node* g, int level = 1) {
  int level_no = level;
  cout << " ";
  while (level_no--) cout << "-";
  cout << "> ";
  if (g->parsed()) {
    switch (g->type()) {
      case 'f': {
        const File* f = (const File*) g;
        cout << "File: " << f->name()
          << ", path = " << f->path()
          << ", type = " << f->type()
          << ", mtime = " << (f->mtime() != 0)
          << ", size = " << f->size()
          << ", uid = " << (int)(f->uid() != 0)
          << ", gid = " << (int)(f->gid() != 0)
          << oct << ", mode = " << f->mode() << dec
          << endl;
      } break;
      case 'l': {
        const Link* l = (const Link*) g;
        cout << "Link: " << l->name()
          << ", path = " << l->path()
          << ", type = " << l->type()
          << ", mtime = " << (l->mtime() != 0)
          << ", size = " << l->size()
          << ", uid = " << (int)(l->uid() != 0)
          << ", gid = " << (int)(l->gid() != 0)
          << oct << ", mode = " << l->mode() << dec
          << ", link = " << l->link()
          << endl;
      } break;
      case 'd': {
        Directory* d = (Directory*) g;
        cout << "Dir.: " << d->name()
          << ", path = " << d->path()
          << ", type = " << d->type()
          << ", mtime = " << (d->mtime() != 0)
          << ", size = " << d->size()
          << ", uid = " << (int)(d->uid() != 0)
          << ", gid = " << (int)(d->gid() != 0)
          << oct << ", mode = " << d->mode() << dec
          << endl;
        if (level) {
          showList(d, level);
        }
      } break;
    }
  } else {
    defaultShowFile(g);
  }
}

void showList(const Directory* d, int level) {
  if (level == 0) {
    showFile(d, level);
  }
  ++level;
  list<Node*>::const_iterator i;

  for (i = d->nodesListConst().begin(); i != d->nodesListConst().end(); i++) {
    showFile(*i, level);
  }
}

void createNshowFile(const Node &g) {
  switch (g.type()) {
  case 'f': {
    File *f = new File(g);
    cout << "Name: " << f->name()
      << ", path = " << f->path()
      << ", type = " << f->type()
      << ", mtime = " << (f->mtime() != 0)
      << ", size = " << f->size()
      << ", uid = " << (int)(f->uid() != 0)
      << ", gid = " << (int)(f->gid() != 0)
      << oct << ", mode = " << f->mode() << dec
      << endl;
    delete f; }
    break;
  case 'l': {
    Link *l = new Link(g);
    cout << "Name: " << l->name()
      << ", path = " << l->path()
      << ", type = " << l->type()
      << ", mtime = " << (l->mtime() != 0)
      << ", size = " << l->size()
      << ", uid = " << (int)(l->uid() != 0)
      << ", gid = " << (int)(l->gid() != 0)
      << oct << ", mode = " << l->mode() << dec
      << ", link = " << l->link()
      << endl;
    delete l; }
    break;
  case 'd': {
    Directory *d = new Directory(g);
    d->createList();
    showList(d);
    delete d; }
    break;
  default:
    cout << "Unknown file type: " << g.type() << endl;
  }
}

bool cancel() {
  return true;
}

int main(void) {
  string  line;

  cout << "Tools Test" << endl;

  mode_t mask = umask(0022);
  mask = umask(0022);
  printf("Our mask = 0%03o\n", mask);

  cout << endl << "Test: path" << endl;
  cout << Node::path("a", "b") << endl;
  cout << Node::path("c", "") << endl;
  cout << Node::path("", "d") << endl;

  Stream* readfile;
  Stream* writefile;
  system("dd if=/dev/zero of=test1/rwfile_source bs=1M count=10 status=noxfer 2> /dev/null");

  cout << endl << "Test: file read (no cache)" << endl;
  readfile = new Stream("test1/rwfile_source");
  if (readfile->open("r", -1)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else {
    unsigned char buffer[Stream::chunk];
    size_t read_size = 0;
    do {
      ssize_t size = readfile->read(buffer, Stream::chunk);
      if (size < 0) {
        cout << "broken by read: " << strerror(errno) << endl;
        break;
      }
      if (size == 0) {
        break;
      }
      read_size += size;
    } while (true);
    if (readfile->close()) cout << "Error closing file" << endl;
    cout << "read size: " << read_size
      << " (" << readfile->size() << " -> " <<  readfile->dsize()
      << "), checksum: " << readfile->checksum() << endl;
  }
  delete readfile;

  cout << endl << "Test: file write (with cache)" << endl;
  writefile = new Stream("test1/rwfile_dest");
  if (writefile->open("w", 0)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else {
    ssize_t size = writefile->write("123", 3);
    if (size < 0) {
      cout << "write failed: " << strerror(errno) << endl;
    }
    size = writefile->write("abc\n", 4);
    if (size < 0) {
      cout << "write failed: " << strerror(errno) << endl;
    }
    if (writefile->close()) cout << "Error closing file" << endl;
    cout << "write: "
      << " (" << writefile->size() << " -> " <<  writefile->dsize()
      << "), checksum: " << writefile->checksum() << endl;
  }
  delete writefile;
  system("cat test1/rwfile_dest");

  cout << endl << "Test: file copy (read + write (no cache))" << endl;
  system("dd if=/dev/zero of=test1/rwfile_source bs=1M count=10 status=noxfer 2> /dev/null");
  readfile = new Stream("test1/rwfile_source");
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open("r")) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open("w", -1)) {
    cout << "Error opening dest file: " << strerror(errno) << endl;
  } else {
    unsigned char buffer[Stream::chunk];
    size_t read_size = 0;
    size_t write_size = 0;
    do {
      ssize_t size = readfile->read(buffer, Stream::chunk);
      if (size < 0) {
        cout << "broken by read: " << strerror(errno) << endl;
        break;
      }
      if (size == 0) {
        break;
      }
      read_size += size;
      size = writefile->write(buffer, size);
      if (size < 0) {
        cout << "broken by write" << endl;
        break;
      }
      write_size += size;
    } while (true);
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
    cout << "read size: " << read_size
      << " (" << readfile->size() << " -> " <<  readfile->dsize()
      << "), checksum: " << readfile->checksum() << endl;
    cout << "write size: " << write_size
      << " (" << writefile->dsize() << " -> " <<  writefile->size()
      << "), checksum: " << writefile->checksum() << endl;
  }
  delete readfile;
  delete writefile;

  cout << endl << "Test: file compress (read + compress write), with last zero-size write call" << endl;
  readfile = new Stream("test1/rwfile_source");
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open("r")) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open("w", 5)) {
    cout << "Error opening dest file: " << strerror(errno) << endl;
  } else {
    unsigned char buffer[Stream::chunk];
    size_t read_size = 0;
    size_t write_size = 0;
    do {
      ssize_t size = readfile->read(buffer, Stream::chunk);
      if (size < 0) {
        cout << "broken by read: " << strerror(errno) << endl;
        break;
      }
      bool eof = (size == 0);
      read_size += size;
      size = writefile->write(buffer, size);
      if (size < 0) {
        cout << "broken by write" << endl;
        break;
      }
      write_size += size;
      if (eof) {
        break;
      }
    } while (true);
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
    cout << "read size: " << read_size
      << " (" << readfile->size() << " -> " <<  readfile->dsize()
      << "), checksum: " << readfile->checksum() << endl;
    cout << "write size (wrong): " << write_size
      << " (" << writefile->dsize() << " -> " <<  writefile->size()
      << "), checksum: " << writefile->checksum() << endl;
  }
  delete readfile;
  delete writefile;

  cout << endl << "Test: file uncompress (uncompress read + write)" << endl;
  readfile = new Stream("test1/rwfile_dest");
  writefile = new Stream("test1/rwfile_source");
  if (readfile->open("r", 1)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open("w")) {
    cout << "Error opening dest file: " << strerror(errno) << endl;
  } else {
    unsigned char buffer[Stream::chunk];
    size_t read_size = 0;
    size_t write_size = 0;
    do {
      ssize_t size = readfile->read(buffer, Stream::chunk);
      if (size < 0) {
        cout << "broken by read: " << strerror(errno) << endl;
        break;
      }
      if (size == 0) {
        break;
      }
      read_size += size;
      size = writefile->write(buffer, size);
      if (size < 0) {
        cout << "broken by write" << endl;
        break;
      }
      write_size += size;
    } while (true);
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
    cout << "read size: " << read_size
      << " (" << readfile->size() << " -> " <<  readfile->dsize()
      << "), checksum: " << readfile->checksum() << endl;
    cout << "write size: " << write_size
      << " (" << writefile->dsize() << " -> " <<  writefile->size()
      << "), checksum: " << writefile->checksum() << endl;
  }
  delete readfile;
  delete writefile;

  cout << endl << "Test: file compress (read + compress write)"
    << endl;
  readfile = new Stream("test1/rwfile_source");
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open("r")) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open("w", 5)) {
    cout << "Error opening dest file: " << strerror(errno) << endl;
  } else {
    unsigned char buffer[Stream::chunk];
    size_t read_size = 0;
    size_t write_size = 0;
    do {
      ssize_t size = readfile->read(buffer, Stream::chunk);
      if (size < 0) {
        cout << "broken by read: " << strerror(errno) << endl;
        break;
      }
      if (size == 0) {
        break;
      }
      read_size += size;
      size = writefile->write(buffer, size);
      if (size < 0) {
        cout << "broken by write" << endl;
        break;
      }
      write_size += size;
    } while (true);
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
    cout << "read size: " << read_size
      << " (" << readfile->size() << " -> " <<  readfile->dsize()
      << "), checksum: " << readfile->checksum() << endl;
    cout << "write size (wrong): " << write_size
      << " (" << writefile->dsize() << " -> " <<  writefile->size()
      << "), checksum: " << writefile->checksum() << endl;
  }
  cout << endl
    << "Test: file recompress (uncompress read + compress write), no closing"
    << endl;
  {
    Stream* swap = readfile;
    readfile = writefile;
    writefile = swap;
  }
  if (readfile->open("r", 1)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open("w", 5)) {
    cout << "Error opening dest file: " << strerror(errno) << endl;
  } else {
    unsigned char buffer[Stream::chunk];
    size_t read_size = 0;
    size_t write_size = 0;
    do {
      ssize_t size = readfile->read(buffer, Stream::chunk);
      if (size < 0) {
        cout << "broken by read: " << strerror(errno) << endl;
        break;
      }
      if (size == 0) {
        break;
      }
      read_size += size;
      size = writefile->write(buffer, size);
      if (size < 0) {
        cout << "broken by write" << endl;
        break;
      }
      write_size += size;
    } while (true);
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
    cout << "read size: " << read_size
      << " (" << readfile->size() << " -> " <<  readfile->dsize()
      << "), checksum: " << readfile->checksum() << endl;
    cout << "write size (wrong): " << write_size
      << " (" << writefile->dsize() << " -> " <<  writefile->size()
      << "), checksum: " << writefile->checksum() << endl;
  }
  delete readfile;
  delete writefile;

  cout << endl
    << "Test: file compare (both compressed)"
    << endl;
  readfile = new Stream("test1/rwfile_source");
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open("r", 1)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open("r", 1)) {
    cout << "Error opening dest file: " << strerror(errno) << endl;
  } else {
    int rc;
    cout << "  whole files" << endl;
    rc = writefile->compare(*readfile);
    if (rc < 0) {
      cout << "Error comparing files: " << strerror(errno) << endl;
    } else if (rc > 0) {
      cout << "files differ" << endl;
    } else {
      cout << "files are equal" << endl;
    }
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
    readfile->open("r", 1);
    writefile->open("r", 1);
    cout << "  exact size" << endl;
    rc = writefile->compare(*readfile, 10485760);
    if (rc < 0) {
      cout << "Error comparing files: " << strerror(errno) << endl;
    } else if (rc > 0) {
      cout << "files differ" << endl;
    } else {
      cout << "files are equal" << endl;
    }
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
    readfile->open("r", 1);
    writefile->open("r", 1);
    cout << "  first 2000 bytes" << endl;
    rc = writefile->compare(*readfile, 2000);
    if (rc < 0) {
      cout << "Error comparing files: " << strerror(errno) << endl;
    } else if (rc > 0) {
      cout << "files differ" << endl;
    } else {
      cout << "files are equal" << endl;
    }
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
    readfile->open("r", 1);
    writefile->open("r", 1);
    cout << "  first 100 bytes" << endl;
    rc = writefile->compare(*readfile, 100);
    if (rc < 0) {
      cout << "Error comparing files: " << strerror(errno) << endl;
    } else if (rc > 0) {
      cout << "files differ" << endl;
    } else {
      cout << "files are equal" << endl;
    }
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  delete readfile;
  delete writefile;

  cout << endl
    << "Test: file compare (one compressed)"
    << endl;
  readfile = new Stream("test1/rwfile_source");
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open("r", 1)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open("r")) {
    cout << "Error opening dest file: " << strerror(errno) << endl;
  } else {
    int rc;
    cout << "  whole files" << endl;
    rc = writefile->compare(*readfile);
    if (rc < 0) {
      cout << "Error comparing files: " << strerror(errno) << endl;
    } else if (rc > 0) {
      cout << "files differ" << endl;
    } else {
      cout << "files are equal" << endl;
    }
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
    readfile->open("r", 1);
    writefile->open("r");
    cout << "  exact size" << endl;
    rc = writefile->compare(*readfile, 10208);
    if (rc < 0) {
      cout << "Error comparing files: " << strerror(errno) << endl;
    } else if (rc > 0) {
      cout << "files differ" << endl;
    } else {
      cout << "files are equal" << endl;
    }
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
    readfile->open("r", 1);
    writefile->open("r");
    cout << "  first 2000 bytes" << endl;
    rc = writefile->compare(*readfile, 2000);
    if (rc < 0) {
      cout << "Error comparing files: " << strerror(errno) << endl;
    } else if (rc > 0) {
      cout << "files differ" << endl;
    } else {
      cout << "files are equal" << endl;
    }
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
    readfile->open("r", 1);
    writefile->open("r");
    cout << "  first 100 bytes" << endl;
    rc = writefile->compare(*readfile, 100);
    if (rc < 0) {
      cout << "Error comparing files: " << strerror(errno) << endl;
    } else if (rc > 0) {
      cout << "files differ" << endl;
    } else {
      cout << "files are equal" << endl;
    }
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  delete readfile;
  delete writefile;

  cout << endl << "Test: computeChecksum" << endl;
  readfile = new Stream("test1/testfile");
  if (readfile->computeChecksum()) {
    cout << "Error computing checksum" << endl;
  } else {
    cout << "Checksum: " << readfile->checksum() << endl;
  }
  delete readfile;

  cout << endl << "Test: copy" << endl;
  readfile = new Stream("test1/rwfile_source");
  remove("test1/rwfile_dest");
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open("r", 1) || writefile->open("w", 0)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    int rc = writefile->copy(*readfile);
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
    if (rc) {
      cout << "Error copying file: " << strerror(errno) << endl;
    } else {
      cout << "checksum in: " << readfile->checksum() << endl;
      cout << "checksum out: " << writefile->checksum() << endl;
    }
  }
  delete readfile;
  delete writefile;

  cout << endl << "Test: interrupted copy" << endl;
  readfile = new Stream("test1/rwfile_source");
  remove("test1/rwfile_dest");
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open("r", 1) || writefile->open("w", 0)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    int rc = writefile->copy(*readfile, cancel);
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
    if (rc) {
      cout << "Error copying file: " << strerror(errno) << endl;
    } else {
      cout << "checksum in: " << readfile->checksum() << endl;
      cout << "checksum out: " << writefile->checksum() << endl;
    }
  }
  delete readfile;
  delete writefile;


  cout << endl << "Test: getLine" << endl;

  writefile = new Stream("test2/testfile");
  if (writefile->open("w")) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->write(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  delete writefile;
  readfile = new Stream("test2/testfile");
  string line_test;
  readfile->open("r", 0);
  cout << "Reading empty file:" << endl;
  while (readfile->getLine(line_test) > 0) {
    cout << "Line: " << line_test.c_str() << endl;
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  delete readfile;

  writefile = new Stream("test2/testfile");
  if (writefile->open("w")) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->write("abcdef\nghi\n", 11);
    writefile->write(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  delete writefile;
  readfile = new Stream("test2/testfile");
  line_test = "";
  if (readfile->open("r")) {
    cout << "Error opening file: " << strerror(errno) << endl;
  }
  cout << "Reading uncompressed file:" << endl;
  while (readfile->getLine(line_test) > 0) {
    cout << "Line: " << line_test.c_str() << endl;
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  delete readfile;

  writefile = new Stream("test2/testfile");
  if (writefile->open("w", 5)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->write("abcdef\nghi\n", 11);
    writefile->write(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  delete writefile;
  readfile = new Stream("test2/testfile");
  line_test = "";
  if (readfile->open("r", 1)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  }
  cout << "Reading compressed file:" << endl;
  while (readfile->getLine(line_test) > 0) {
    cout << "Line: " << line_test.c_str() << endl;
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  delete readfile;

  writefile = new Stream("test2/testfile");
  if (writefile->open("w", 5)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    for (int i = 0; i < 60; i++) {
      writefile->write("1234567890", 10);
    }
    writefile->write("\n1234567890", 10);
    writefile->write(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  delete writefile;
  readfile = new Stream("test2/testfile");
  line_test = "";
  if (readfile->open("r", 1)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  }
  cout << "Reading compressed file (line length = 600):" << endl;
  while (readfile->getLine(line_test) > 0) {
    cout << "Line: " << line_test.c_str() << endl;
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  delete readfile;


  cout << "\nreadline" << endl;
  list<string> *params;
  list<string>::iterator i;

  // Start simple: one argument
  line = "a";
  params = new list<string>;
  cout << "readline(" << line << "): " << Stream::decodeLine(line, *params)
    << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Two arguments, test blanks
  line = " \ta \tb";
  params = new list<string>;
  cout << "readline(" << line << "): " << Stream::decodeLine(line, *params)
    << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Not single character argument
  line = "\t ab";
  params = new list<string>;
  cout << "readline(" << line << "): " << Stream::decodeLine(line, *params)
    << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Two of them
  line = "\t ab cd";
  params = new list<string>;
  cout << "readline(" << line << "): " << Stream::decodeLine(line, *params)
    << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Three, with comment
  line = "\t ab cd\tef # blah";
  params = new list<string>;
  cout << "readline(" << line << "): " << Stream::decodeLine(line, *params)
    << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Single quotes
  line = "\t 'ab' 'cd'\t'ef' # blah";
  params = new list<string>;
  cout << "readline(" << line << "): " << Stream::decodeLine(line, *params)
    << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // And double quotes
  line = "\t \"ab\" 'cd'\t\"ef\" # blah";
  params = new list<string>;
  cout << "readline(" << line << "): " << Stream::decodeLine(line, *params)
    << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // With blanks in quotes
  line = "\t ab cd\tef 'gh ij\tkl' \"mn op\tqr\" \t# blah";
  params = new list<string>;
  cout << "readline(" << line << "): " << Stream::decodeLine(line, *params)
    << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // With quotes in quotes
  line = "\t ab cd\tef 'gh \"ij\\\'\tkl' \"mn 'op\\\"\tqr\" \t# blah";
  params = new list<string>;
  cout << "readline(" << line << "): " << Stream::decodeLine(line, *params)
    << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // With escape characters
  line = "\t a\\b cd\tef 'g\\h \"ij\\\'\tkl' \"m\\n 'op\\\"\tqr\" \t# blah";
  params = new list<string>;
  cout << "readline(" << line << "): " << Stream::decodeLine(line, *params)
    << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Missing ending single quote
  line = "\t a\\b cd\tef 'g\\h \"ij\\\'\tkl";
  params = new list<string>;
  cout << "readline(" << line << "): " << Stream::decodeLine(line, *params)
    << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Missing ending double quote
  line = "\t a\\b cd\tef 'g\\h \"ij\\\'\tkl' \"m\\n 'op\\\"\tqr";
  params = new list<string>;
  cout << "readline(" << line << "): " << Stream::decodeLine(line, *params)
    << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // File types
  Node *g;
  g = new Node("test1/testfile");
  createNshowFile(*g);
  delete g;
  g = new Node("test1/testlink");
  createNshowFile(*g);
  delete g;
  g = new Node("test1/testdir");
  createNshowFile(*g);
  delete g;
  g = new Node("test1/subdir");
  createNshowFile(*g);
  delete g;

  cout << endl << "Validity tests" << endl;
  cout << "File is file? " << File("test1/testfile").isValid() << endl;
  cout << "File is dir? " << Directory("test1/testfile").isValid() << endl;
  cout << "File is link? " << Link("test1/testfile").isValid() << endl;
  cout << "Dir is file? " << File("test1/testdir").isValid() << endl;
  cout << "Dir is dir? " << Directory("test1/testdir").isValid() << endl;
  cout << "Dir is link? " << Link("test1/testdir").isValid() << endl;
  cout << "Link is file? " << File("test1/testlink").isValid() << endl;
  cout << "Link is dir? " << Directory("test1/testlink").isValid() << endl;
  cout << "Link is link? " << Link("test1/testlink").isValid() << endl;

  cout << endl << "Creation tests" << endl;
  cout << "File is file? " << File("test1/touchedfile").isValid() << endl;
  cout << "File is dir? " << Directory("test1/touchedfile").isValid() << endl;
  cout << "File is link? " << Link("test1/touchedfile").isValid() << endl;
  cout << "Dir is file? " << File("test1/toucheddir").isValid() << endl;
  cout << "Dir is dir? " << Directory("test1/toucheddir").isValid() << endl;
  cout << "Dir is link? " << Link("test1/toucheddir").isValid() << endl;
  cout << "Link is file? " << File("test1/touchedlink").isValid() << endl;
  cout << "Link is dir? " << Directory("test1/touchedlink").isValid() << endl;
  cout << "Link is link? " << Link("test1/touchedlink").isValid() << endl;

  cout << "Create" << endl;
  if (File("test1/touchedfile").create())
    cout << "failed to create file: " << strerror(errno) << endl;
  if (Directory("test1/toucheddir").create())
    cout << "failed to create dir" << endl;

  cout << "File is file? " << File("test1/touchedfile").isValid() << endl;
  cout << "File is dir? " << Directory("test1/touchedfile").isValid() << endl;
  cout << "File is link? " << Link("test1/touchedfile").isValid() << endl;
  cout << "Dir is file? " << File("test1/toucheddir").isValid() << endl;
  cout << "Dir is dir? " << Directory("test1/toucheddir").isValid() << endl;
  cout << "Dir is link? " << Link("test1/toucheddir").isValid() << endl;
  cout << "Link is file? " << File("test1/touchedlink").isValid() << endl;
  cout << "Link is dir? " << Directory("test1/touchedlink").isValid() << endl;
  cout << "Link is link? " << Link("test1/touchedlink").isValid() << endl;

  cout << "Create again" << endl;
  if (File("test1/touchedfile").create())
    cout << "failed to create file: " << strerror(errno) << endl;
  if (Directory("test1/toucheddir").create())
    cout << "failed to create dir" << endl;

  cout << "File is file? " << File("test1/touchedfile").isValid() << endl;
  cout << "File is dir? " << Directory("test1/touchedfile").isValid() << endl;
  cout << "File is link? " << Link("test1/touchedfile").isValid() << endl;
  cout << "Dir is file? " << File("test1/toucheddir").isValid() << endl;
  cout << "Dir is dir? " << Directory("test1/toucheddir").isValid() << endl;
  cout << "Dir is link? " << Link("test1/toucheddir").isValid() << endl;
  cout << "Link is file? " << File("test1/touchedlink").isValid() << endl;
  cout << "Link is dir? " << Directory("test1/touchedlink").isValid() << endl;
  cout << "Link is link? " << Link("test1/touchedlink").isValid() << endl;

  cout << endl << "Parsing test" << endl;

  Directory* d = new Directory("test1");
  if (d->isValid()) {
    if (! d->createList()) {
      if (! parseList(d, "test1")) {
        showList(d);
      }
    } else {
      cerr << "Failed to create list: " << strerror(errno) << endl;
    }
  }
  delete d;

  cout << endl << "Difference test" << endl;

  Node* f1 = new Node("some_name", 'f', 1, 2, 3, 4, 5);
  Node* f2 = new Node("some_name", 'f', 1, 2, 3, 4, 5);
  if (*f1 != *f2) {
    cout << "f1 and f2 differ" << endl;
  } else {
    cout << "f1 and f2 are identical" << endl;
  }
  delete f2;
  f2 = new Node("other_name", 'f', 1, 2, 3, 4, 5);
  if (*f1 != *f2) {
    cout << "f1 and f2 differ" << endl;
  } else {
    cout << "f1 and f2 are identical" << endl;
  }
  delete f2;
  f2 = new Node("some_name", 'd', 1, 2, 3, 4, 5);
  if (*f1 != *f2) {
    cout << "f1 and f2 differ" << endl;
  } else {
    cout << "f1 and f2 are identical" << endl;
  }
  delete f2;
  f2 = new Node("some_name", 'f', 10, 2, 3, 4, 5);
  if (*f1 != *f2) {
    cout << "f1 and f2 differ" << endl;
  } else {
    cout << "f1 and f2 are identical" << endl;
  }
  delete f2;
  f2 = new Node("some_name", 'f', 1, 20, 3, 4, 5);
  if (*f1 != *f2) {
    cout << "f1 and f2 differ" << endl;
  } else {
    cout << "f1 and f2 are identical" << endl;
  }
  delete f2;
  f2 = new Node("some_name", 'f', 1, 2, 30, 4, 5);
  if (*f1 != *f2) {
    cout << "f1 and f2 differ" << endl;
  } else {
    cout << "f1 and f2 are identical" << endl;
  }
  delete f2;
  f2 = new Node("some_name", 'f', 1, 2, 3, 40, 5);
  if (*f1 != *f2) {
    cout << "f1 and f2 differ" << endl;
  } else {
    cout << "f1 and f2 are identical" << endl;
  }
  delete f2;
  f2 = new Node("some_name", 'f', 1, 2, 3, 4, 50);
  if (*f1 != *f2) {
    cout << "f1 and f2 differ" << endl;
  } else {
    cout << "f1 and f2 are identical" << endl;
  }
  delete f2;
  delete f1;

  Link* l1 = new Link("some_link", 'l', 1, 2, 3, 4, 5, "linked");
  Link* l2 = new Link("some_link", 'l', 1, 2, 3, 4, 5, "linked");
  if (*l1 != *l2) {
    cout << "l1 and l2 differ" << endl;
  } else {
    cout << "l1 and l2 are identical" << endl;
  }
  delete l2;
  l2 = new Link("other_link", 'l', 1, 2, 3, 4, 5, "linked");
  if (*l1 != *l2) {
    cout << "l1 and l2 differ" << endl;
  } else {
    cout << "l1 and l2 are identical" << endl;
  }
  delete l2;
  l2 = new Link("some_link", 'l', 1, 2, 3, 4, 5, "unlinked");
  if (*l1 != *l2) {
    cout << "l1 and l2 differ" << endl;
  } else {
    cout << "l1 and l2 are identical" << endl;
  }
  delete l2;
  delete l1;

  cout << endl << "End of tests" << endl;

  return 0;
}
