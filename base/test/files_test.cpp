/*
     Copyright (C) 2006-2010  Herve Fache

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
#include <iomanip>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>

using namespace std;

#include "hreport.h"
#include "files.h"
#include "filereaderwriter.h"
#include "unzipreader.h"
#include "zipwriter.h"
#include "hasher.h"

using namespace hbackup;
using namespace htools;

static int parseList(Directory *d, const char* cur_dir) {
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
        Path dir_path(cur_dir, di->name());
        if (! di->createList()) {
          parseList(di, dir_path.c_str());
        } else {
          cerr << "Failed to create list for " << di->name() << " in "
            << cur_dir << endl;
        }
      }
      break;
    }
    i++;
  }
  return 0;
}

static void showList(const Directory& d, int level = 0);

static void defaultShowFile(const Node& g) {
  cout << "Oth.: " << g.name()
    << ", path = " << g.path()
    << ", type = " << g.type()
    << ", mtime = " << (g.mtime() != 0)
    << ", size = " << g.size()
    << ", uid = " << static_cast<int>(g.uid() != 0)
    << ", gid = " << static_cast<int>(g.gid() != 0)
    << oct << ", mode = " << g.mode() << dec
    << endl;
}

static void showFile(const Node& g, int level = 1) {
  int level_no = level;
  cout << " ";
  while (level_no--) cout << "-";
  cout << "> ";
  if (g.parsed()) {
    switch (g.type()) {
      case 'f': {
        const File& f = static_cast<const File&>(g);
        cout << "File: " << f.name()
          << ", path = " << f.path()
          << ", type = " << f.type()
          << ", mtime = " << (f.mtime() != 0)
          << ", size = " << f.size()
          << ", uid = " << static_cast<int>(f.uid() != 0)
          << ", gid = " << static_cast<int>(f.gid() != 0)
          << oct << ", mode = " << f.mode() << dec
          << endl;
      } break;
      case 'l': {
        const Link& l = static_cast<const Link&>(g);
        cout << "Link: " << l.name()
          << ", path = " << l.path()
          << ", type = " << l.type()
          << ", mtime = " << (l.mtime() != 0)
          << ", size = " << l.size()
          << ", uid = " << static_cast<int>(l.uid() != 0)
          << ", gid = " << static_cast<int>(l.gid() != 0)
          << oct << ", mode = " << l.mode() << dec
          << ", link = " << l.link()
          << endl;
      } break;
      case 'd': {
        const Directory& d = static_cast<const Directory&>(g);
        cout << "Dir.: " << d.name()
          << ", path = " << d.path()
          << ", type = " << d.type()
          << ", mtime = " << (d.mtime() != 0)
          << ", size = " << (d.size() != 0)
          << ", uid = " << static_cast<int>(d.uid() != 0)
          << ", gid = " << static_cast<int>(d.gid() != 0)
          << oct << ", mode = " << d.mode() << dec
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

static void showList(const Directory& d, int level) {
  if (level == 0) {
    showFile(d, level);
  }
  ++level;
  list<Node*>::const_iterator i;
  for (i = d.nodesListConst().begin(); i != d.nodesListConst().end(); i++) {
    if (! strcmp((*i)->name(), "cvs")) continue;
    if (! strcmp((*i)->name(), "svn")) continue;
    if (! strcmp((*i)->name(), "bzr")) continue;
    showFile(**i, level);
  }
}

static void createNshowFile(const Node &g) {
  switch (g.type()) {
  case 'f': {
    File *f = new File(g);
    cout << "Name: " << f->name()
      << ", path = " << f->path()
      << ", type = " << f->type()
      << ", mtime = " << (f->mtime() != 0)
      << ", size = " << f->size()
      << ", uid = " << static_cast<int>(f->uid() != 0)
      << ", gid = " << static_cast<int>(f->gid() != 0)
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
      << ", uid = " << static_cast<int>(l->uid() != 0)
      << ", gid = " << static_cast<int>(l->gid() != 0)
      << oct << ", mode = " << l->mode() << dec
      << ", link = " << l->link()
      << endl;
    delete l; }
    break;
  case 'd': {
    Directory d(g);
    d.createList();
    showList(d);
    } break;
  default:
    cout << "Unknown file type: " << g.type() << endl;
  }
}

bool cancel(unsigned short __unused) {
  (void) __unused;
  return true;
}

static void read_progress(long long previous, long long current, long long total) {
  if ((current != total) || (previous != 0)) {
    printf("Done: %5.1lf%%\n",
      100 * static_cast<double>(current) / static_cast<double>(total));
  }
}

static void write_progress(long long previous, long long current, long long total) {
  if ((current != total) || (previous != 0)) {
    printf("Done: %2lld\n", current);
  }
}

int main(void) {
  string  line;
  int     sys_rc;

  cout << "Tools Test" << endl;

  mode_t mask = umask(0022);
  mask = umask(0022);
  printf("Our mask = 0%03o\n", mask);

  cout << endl << "Test: constructors" << endl;
  Path* pth0;
  pth0 = new Path("");
  cout << pth0->c_str() << endl;
  delete pth0;
  pth0 = new Path("123");
  cout << pth0->c_str() << endl;
  delete pth0;
  pth0 = new Path("123/456", "");
  cout << pth0->c_str() << endl;
  delete pth0;
  pth0 = new Path("", "789");
  cout << pth0->c_str() << endl;
  delete pth0;
  pth0 = new Path(Path("123", "456").c_str(), "789");
  cout << pth0->c_str() << endl;
  delete pth0;
  pth0 = new Path("123/456", "789/159");
  cout << pth0->c_str() << endl;
  delete pth0;
  pth0 = new Path("//1123//456", "7899//11599//");
  cout << pth0->c_str() << endl;
  delete pth0;

  cout << endl << "Test: basename and dirname" << endl;
  pth0 = new Path("this/is a path/to a/file");
  cout << pth0->c_str();
  cout << " -> base: ";
  cout << basename(pth0->c_str());
  cout << ", dir: ";
  cout << pth0->dirname().c_str();
  cout << endl;
  delete pth0;
  pth0 = new Path("this is a file");
  cout << pth0->c_str();
  cout << " -> base: ";
  cout << basename(pth0->c_str());
  cout << ", dir: ";
  cout << pth0->dirname().c_str();
  cout << endl;
  delete pth0;
  pth0 = new Path("this is a path/");
  cout << pth0->c_str();
  cout << " -> base: ";
  cout << basename(pth0->c_str());
  cout << ", dir: ";
  cout << pth0->dirname().c_str();
  cout << endl;
  delete pth0;

  cout << endl << "Test: fromDos" << endl;
  char c_str[256];
  strcpy(c_str, "a:\\backup\\");
  cout << c_str << " -> ";
  cout << Path::fromDos(c_str) << endl;
  strcpy(c_str, "z:/backup");
  cout << c_str << " -> ";
  cout << Path::fromDos(c_str) << endl;
  strcpy(c_str, "1:/backup");
  cout << c_str << " -> ";
  cout << Path::fromDos(c_str) << endl;
  strcpy(c_str, "f;/backup");
  cout << c_str << " -> ";
  cout << Path::fromDos(c_str) << endl;
  strcpy(c_str, "l:|backup");
  cout << c_str << " -> ";
  cout << Path::fromDos(c_str) << endl;
  strcpy(c_str, "this\\is a path/ Unix\\ cannot cope with/\\");
  cout << c_str << " -> ";
  cout << Path::fromDos(c_str) << endl;
  cout << c_str << " -> ";
  cout << Path::fromDos(c_str) << endl;

  cout << endl << "Test: noTrailingSlashes" << endl;
  strcpy(c_str, "this/is a path/ Unix/ cannot cope with//");
  cout << c_str << " -> ";
  cout << Path::noTrailingSlashes(c_str) << endl;
  Path::fromDos(c_str);
  cout << c_str << " -> ";
  cout << Path::noTrailingSlashes(c_str) << endl;
  strcpy(c_str, "");
  cout << c_str << " -> ";
  cout << Path::noTrailingSlashes(c_str) << endl;
  strcpy(c_str, "/");
  cout << c_str << " -> ";
  cout << Path::noTrailingSlashes(c_str) << endl;

  cout << endl << "Test: compare" << endl;
  cout << "a <> a: " << Path::compare("a", "a") << endl;
  cout << "a <> b: " << Path::compare("a", "b") << endl;
  cout << "b <> a: " << Path::compare("b", "a") << endl;
  cout << "a1 <> b: " << Path::compare("a1", "b") << endl;
  cout << "b <> a1: " << Path::compare("b", "a1") << endl;
  cout << "a1 <> a: " << Path::compare("a1", "a") << endl;
  cout << "a <> a1: " << Path::compare("a", "a1") << endl;
  cout << "a/ <> a: " << Path::compare("a/", "a") << endl;
  cout << "a <> a/: " << Path::compare("a", "a/") << endl;
  cout << "a\t <> a/: " << Path::compare("a\t", "a/") << endl;
  cout << "a/ <> a\t " << Path::compare("a/", "a\t") << endl;
  cout << "a\t <> a\t " << Path::compare("a\t", "a\t") << endl;
  cout << "a\n <> a/: " << Path::compare("a\n", "a/") << endl;
  cout << "a/ <> a\n " << Path::compare("a/", "a\n") << endl;
  cout << "a\n <> a\n " << Path::compare("a\n", "a\n") << endl;
  cout << "a/ <> a.: " << Path::compare("a/", "a.") << endl;
  cout << "a. <> a/: " << Path::compare("a.", "a/") << endl;
  cout << "a/ <> a-: " << Path::compare("a/", "a-") << endl;
  cout << "a- <> a/: " << Path::compare("a-", "a/") << endl;
  cout << "a/ <> a/: " << Path::compare("a/", "a/") << endl;
  cout << "abcd <> abce, 3: " << Path::compare("abcd", "abce", 3) << endl;
  cout << "abcd <> abce, 4: " << Path::compare("abcd", "abce", 4) << endl;
  cout << "abcd <> abce, 5: " << Path::compare("abcd", "abce", 5) << endl;


  Stream* readfile;
  Stream* writefile;

  cout << endl << "Test: file read (no cache)" << endl;
  readfile = new Stream("test1/big_file", false, true);
  if (readfile->open()) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else {
    readfile->setProgressCallback(read_progress);
    unsigned char buffer[1 << 20];
    size_t read_size = 0;
    do {
      ssize_t size = readfile->read(buffer, 1 << 20);
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
    cout << "read size: " << read_size << " (" << readfile->size() << ", "
      << readfile->dataSize() << "), checksum: " << readfile->checksum()
      << endl;
  }
  delete readfile;

  cout << endl << "Test: file write" << endl;
  writefile = new Stream("test1/rwfile_dest", true, true);
  if (writefile->open()) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else {
    writefile->setProgressCallback(write_progress);
    size_t write_size = 0;
    ssize_t size = writefile->write("123", 3);
    if (size < 0) {
      cout << "write failed: " << strerror(errno) << endl;
    }
    write_size += size;
    size = writefile->write("abc\n", 4);
    if (size < 0) {
      cout << "write failed: " << strerror(errno) << endl;
    }
    write_size += size;
    size = writefile->write("line\n", 5);
    if (size < 0) {
      cout << "write failed: " << strerror(errno) << endl;
    }
    write_size += size;
    size = writefile->write("this is a long line\n", 20);
    if (size < 0) {
      cout << "write failed: " << strerror(errno) << endl;
    }
    write_size += size;
    if (writefile->close()) cout << "Error closing file" << endl;
    cout << "write size: " << write_size << " (" << writefile->size() << ", "
      << writefile->dataSize() << "), checksum: " << writefile->checksum()
      << endl;
  }
  delete writefile;
  sys_rc = system("cat test1/rwfile_dest");

  cout << endl << "Test: file copy (read (cache) + write (no cache))" << endl;
  readfile = new Stream("test1/big_file", false, true);
  writefile = new Stream("test1/rwfile_dest", true, true);
  if (readfile->open()) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open()) {
    cout << "Error opening dest file: " << strerror(errno) << endl;
  } else {
    readfile->setProgressCallback(read_progress);
    unsigned char buffer[512000];
    size_t read_size = 0;
    size_t write_size = 0;
    do {
      ssize_t size = readfile->read(buffer, sizeof(buffer));
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
    cout << "read size: " << read_size << " (" << readfile->size() << ", "
      << readfile->dataSize() << "), checksum: " << readfile->checksum()
      << endl;
    cout << "write size: " << write_size << " (" << writefile->size() << ", "
      << writefile->dataSize() << "), checksum: " << writefile->checksum()
      << endl;
  }
  delete readfile;
  delete writefile;

  cout << endl << "Test: file compress (read + default compress write), "
    << "with last zero-size write call" << endl;
  readfile = new Stream("test1/big_file", false, true);
  writefile = new Stream("test1/rwfile_dest", true, true, 5);
  if (readfile->open()) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open()) {
    cout << "Error opening dest file: " << strerror(errno) << endl;
  } else {
    unsigned char buffer[512000];
    size_t read_size = 0;
    size_t write_size = 0;
    do {
      ssize_t size = readfile->read(buffer, sizeof(buffer));
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
    cout << "read size: " << read_size << " (" << readfile->size() << ", "
      << readfile->dataSize() << "), checksum: " << readfile->checksum()
      << endl;
    cout << "write size: " << write_size << " (" << writefile->size() << ", "
      << writefile->dataSize() << "), checksum: " << writefile->checksum()
      << endl;
    Stream temp(writefile->path(), false, false, 1);
    if (temp.open()) {
      cout << "Error re-opening source file: " << strerror(errno) << endl;
    } else {
      char blah;
      temp.read(&blah, 1);
      temp.close();
    }
  }
  delete readfile;
  delete writefile;

  cout << endl << "Test: file uncompress (uncompress read + write)" << endl;
  readfile = new Stream("test1/rwfile_dest", false, true, 1);
  writefile = new Stream("test1/rwfile_source", true, true);
  if (readfile->open()) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open()) {
    cout << "Error opening dest file: " << strerror(errno) << endl;
  } else {
    unsigned char buffer[512000];
    size_t read_size = 0;
    size_t write_size = 0;
    do {
      ssize_t size = readfile->read(buffer, sizeof(buffer));
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
    cout << "read size: " << read_size << " (" << readfile->size() << ", "
      << readfile->dataSize() << "), checksum: " << readfile->checksum()
      << endl;
    cout << "write size: " << write_size << " (" << writefile->size() << ", "
      << writefile->dataSize() << "), checksum: " << writefile->checksum()
      << endl;
  }
  delete readfile;
  delete writefile;

  cout << endl << "Test: file compress (read + compress write)" << endl;
  readfile = new Stream("test1/rwfile_source", false, true);
  writefile = new Stream("test1/rwfile_dest", true, true, 5);
  if (readfile->open()) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open()) {
    cout << "Error opening dest file: " << strerror(errno) << endl;
  } else {
    unsigned char buffer[512000];
    size_t read_size = 0;
    size_t write_size = 0;
    do {
      ssize_t size = readfile->read(buffer, sizeof(buffer));
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
    cout << "read size: " << read_size << " (" << readfile->size() << ", "
      << readfile->dataSize() << "), checksum: " << readfile->checksum()
      << endl;
    cout << "write size: " << write_size << " (" << writefile->size() << ", "
      << writefile->dataSize() << "), checksum: " << writefile->checksum()
      << endl;
    Stream temp(writefile->path(), false, false, 1);
    if (temp.open()) {
      cout << "Error re-opening source file: " << strerror(errno) << endl;
    } else {
      char blah;
      temp.read(&blah, 1);
      temp.close();
    }
  }
  cout << endl
    << "Test: file recompress (uncompress read + compress write)" << endl;
  {
    string rfp = readfile->path();
    string wfp = writefile->path();
    delete readfile;
    delete writefile;
    readfile = new Stream(wfp.c_str(), false, true, 1);
    writefile = new Stream(rfp.c_str(), true, true, 5);
  }
  if (readfile->open()) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open()) {
    cout << "Error opening dest file: " << strerror(errno) << endl;
  } else {
    unsigned char buffer[512000];
    size_t read_size = 0;
    size_t write_size = 0;
    do {
      ssize_t size = readfile->read(buffer, sizeof(buffer));
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
    cout << "read size: " << read_size << " (" << readfile->size() << ", "
      << readfile->dataSize() << "), checksum: " << readfile->checksum()
      << endl;
    cout << "write size: " << write_size << " (" << writefile->size() << ", "
      << writefile->dataSize() << "), checksum: " << writefile->checksum()
      << endl;
    Stream temp(writefile->path(), false, false, 1);
    if (temp.open()) {
      cout << "Error re-opening source file: " << strerror(errno) << endl;
    } else {
      char blah;
      temp.read(&blah, 1);
      temp.close();
    }
  }
  delete readfile;
  delete writefile;


  cout << endl << "Test: file compare (both compressed)" << endl;
  readfile = new Stream("test1/rwfile_source", false, true, 1);
  Stream* readfile2 = new Stream("test1/rwfile_dest", false, true, 1);
  if (readfile->open()) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (readfile2->open()) {
    cout << "Error opening dest file: " << strerror(errno) << endl;
  } else {
    int rc;
    cout << "  whole files" << endl;
    rc = readfile2->compare(*readfile);
    if (rc < 0) {
      cout << "Error comparing files: " << strerror(errno) << endl;
    } else if (rc > 0) {
      cout << "files differ" << endl;
    } else {
      cout << "files are equal" << endl;
    }
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (readfile2->close()) cout << "Error closing write file" << endl;
    readfile->open();
    readfile2->open();
    cout << "  exact size" << endl;
    rc = readfile2->compare(*readfile, 10485760);
    if (rc < 0) {
      cout << "Error comparing files: " << strerror(errno) << endl;
    } else if (rc > 0) {
      cout << "files differ" << endl;
    } else {
      cout << "files are equal" << endl;
    }
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (readfile2->close()) cout << "Error closing write file" << endl;
    readfile->open();
    readfile2->open();
    cout << "  first 2000 bytes" << endl;
    rc = readfile2->compare(*readfile, 2000);
    if (rc < 0) {
      cout << "Error comparing files: " << strerror(errno) << endl;
    } else if (rc > 0) {
      cout << "files differ" << endl;
    } else {
      cout << "files are equal" << endl;
    }
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (readfile2->close()) cout << "Error closing write file" << endl;
    readfile->open();
    readfile2->open();
    cout << "  first 100 bytes" << endl;
    rc = readfile2->compare(*readfile, 100);
    if (rc < 0) {
      cout << "Error comparing files: " << strerror(errno) << endl;
    } else if (rc > 0) {
      cout << "files differ" << endl;
    } else {
      cout << "files are equal" << endl;
    }
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (readfile2->close()) cout << "Error closing write file" << endl;
  }
  delete readfile;
  delete readfile2;

  cout << endl << "Test: file compare (one compressed)" << endl;
  readfile = new Stream("test1/rwfile_source", false, true, 1);
  readfile2 = new Stream("test1/rwfile_dest", false, true);
  if (readfile->open()) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (readfile2->open()) {
    cout << "Error opening dest file: " << strerror(errno) << endl;
  } else {
    int rc;
    cout << "  whole files" << endl;
    rc = readfile2->compare(*readfile);
    if (rc < 0) {
      cout << "Error comparing files: " << strerror(errno) << endl;
    } else if (rc > 0) {
      cout << "files differ" << endl;
    } else {
      cout << "files are equal" << endl;
    }
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (readfile2->close()) cout << "Error closing write file" << endl;
    readfile->open();
    readfile2->open();
    cout << "  exact size" << endl;
    rc = readfile2->compare(*readfile, 10208);
    if (rc < 0) {
      cout << "Error comparing files: " << strerror(errno) << endl;
    } else if (rc > 0) {
      cout << "files differ" << endl;
    } else {
      cout << "files are equal" << endl;
    }
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (readfile2->close()) cout << "Error closing write file" << endl;
    readfile->open();
    readfile2->open();
    cout << "  first 2000 bytes" << endl;
    rc = readfile2->compare(*readfile, 2000);
    if (rc < 0) {
      cout << "Error comparing files: " << strerror(errno) << endl;
    } else if (rc > 0) {
      cout << "files differ" << endl;
    } else {
      cout << "files are equal" << endl;
    }
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (readfile2->close()) cout << "Error closing write file" << endl;
    readfile->open();
    readfile2->open();
    cout << "  first 100 bytes" << endl;
    rc = readfile2->compare(*readfile, 100);
    if (rc < 0) {
      cout << "Error comparing files: " << strerror(errno) << endl;
    } else if (rc > 0) {
      cout << "files differ" << endl;
    } else {
      cout << "files are equal" << endl;
    }
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (readfile2->close()) cout << "Error closing write file" << endl;
  }
  delete readfile;
  delete readfile2;


  cout << endl << "Test: computeChecksum" << endl;
  cout << "default read" << endl;
  readfile = new Stream("test1/testfile", false, true);
  if (readfile->open()) {
    return 0;
  }
  readfile->setProgressCallback(read_progress);
  if (readfile->computeChecksum()) {
    cout << "Error computing checksum, " << strerror(errno) << endl;
    return 0;
  }
  if (readfile->close()) {
    return 0;
  }
  cout << "Checksum: " << readfile->checksum() << endl;
  cout << "Data size: " << readfile->dataSize() << endl;
  delete readfile;

  cout << "default read" << endl;
  readfile = new Stream("test2/testfile2.gz", false, true);
  if (readfile->open()) {
    return 0;
  }
  readfile->setProgressCallback(read_progress);
  if (readfile->computeChecksum()) {
    cout << "Error computing checksum, " << strerror(errno) << endl;
    return 0;
  }
  if (readfile->close()) {
    return 0;
  }
  cout << "Checksum: " << readfile->checksum() << endl;
  cout << "Data size: " << readfile->dataSize() << endl;
  delete readfile;

  cout << "default decompress read" << endl;
  readfile = new Stream("test2/testfile2.gz", false, true, 1);
  if (readfile->open()) {
    return 0;
  }
  readfile->setProgressCallback(read_progress);
  if (readfile->computeChecksum()) {
    cout << "Error computing checksum, " << strerror(errno) << endl;
    return 0;
  }
  if (readfile->close()) {
    return 0;
  }
  cout << "Checksum: " << readfile->checksum() << endl;
  cout << "Data size: " << readfile->dataSize() << endl;
  delete readfile;

  cout << "default decompress read" << endl;
  readfile = new Stream("test1/rwfile_source", false, true, 1);
  if (readfile->open()) {
    return 0;
  }
  readfile->setProgressCallback(read_progress);
  if (readfile->computeChecksum()) {
    cout << "Error computing checksum, " << strerror(errno) << endl;
    return 0;
  }
  if (readfile->close()) {
    return 0;
  }
  cout << "Checksum: " << readfile->checksum() << endl;
  cout << "Data size: " << readfile->dataSize() << endl;
  delete readfile;


  cout << endl << "Test: copy" << endl;
  readfile = new Stream("test1/rwfile_source", false, true, 1);
  Node("test1/rwfile_dest").remove();
  writefile = new Stream("test1/rwfile_dest", true, true);
  if (readfile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    int rc = readfile->copy(writefile);
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (rc) {
      cout << "Error copying file: " << strerror(errno) << endl;
    } else {
      cout << "checksum in: " << readfile->checksum() << endl;
      cout << "checksum out: " << writefile->checksum() << endl;
      cout << "size in: " << readfile->size() << endl;
      cout << "size out: " << writefile->size() << endl;
    }
  }
  delete readfile;
  delete writefile;
  readfile = new Stream("test1/rwfile_dest", false, true);
  writefile = new Stream("test1/rwfile_copy", true, true);
  if (readfile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    int rc = readfile->copy(writefile);
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (rc) {
      cout << "Error copying file: " << strerror(errno) << endl;
    } else {
      cout << "checksum in: " << readfile->checksum() << endl;
      cout << "checksum out: " << writefile->checksum() << endl;
      cout << "size in: " << readfile->size() << endl;
      cout << "size out: " << writefile->size() << endl;
    }
  }
  delete readfile;
  delete writefile;
  Node("test1/rwfile_dest").remove();
  readfile = new Stream("test1/rwfile_copy", false, true);
  writefile = new Stream("test1/rwfile_dest", true, true, 5);
  if (readfile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    int rc = readfile->copy(writefile);
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (rc) {
      cout << "Error copying file: " << strerror(errno) << endl;
    } else {
      cout << "checksum in: " << readfile->checksum() << endl;
      cout << "checksum out: " << writefile->checksum() << endl;
      cout << "size in: " << readfile->size() << endl;
      cout << "size out: " << writefile->size() << endl;
    }
  }
  delete readfile;
  delete writefile;
  Node("test1/rwfile_dest").remove();
  Node("test1/rwfile_copy").remove();

  cout << endl << "Test: interrupted copy" << endl;
  readfile = new Stream("test1/rwfile_source", false, true, 1);
  writefile = new Stream("test1/rwfile_dest", true, true);
  if (readfile->open()) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    readfile->setCancelCallback(cancel);
    int rc = readfile->copy(writefile);
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (rc) {
      cout << "Error copying file: " << strerror(errno) << endl;
    } else {
      cout << "checksum in: " << readfile->checksum() << endl;
      cout << "checksum out: " << writefile->checksum() << endl;
    }
  }
  delete readfile;
  delete writefile;
  Node("test1/rwfile_dest").remove();

  cout << endl << "Test: double copy" << endl;
  readfile = new Stream("test1/rwfile_source", false, true, 1);
  writefile = new Stream("test1/rwfile_dest", true, true, 5);
  Stream* writefile2 = new Stream("test1/rwfile_dest2", true, true, 5);
  if (readfile->open()) {
    cout << "Error opening a file: " << strerror(errno) << endl;
  } else {
    int rc = readfile->copy(writefile, writefile2);
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (rc) {
      cout << "Error copying file: " << strerror(errno) << endl;
    } else {
      cout << "checksum in: " << readfile->checksum() << endl;
      cout << "checksum out: " << writefile->checksum() << endl;
      cout << "checksum out 2: " << writefile2->checksum() << endl;
      cout << "data in: " << readfile->dataSize() << endl;
      cout << "data out: " << writefile->dataSize() << endl;
      cout << "data out 2: " << writefile2->dataSize() << endl;
      cout << "file out: " << writefile->size() << endl;
      cout << "file out 2: " << writefile2->size() << endl;
    }
  }
  delete readfile;
  writefile->remove();
  delete writefile;
  writefile2->remove();
  delete writefile2;


  cout << endl << "File types" << endl;
  // File types
  Node *g;
  g = new Node("test1/testfile");
  g->stat();
  createNshowFile(*g);
  delete g;
  g = new Node("test1/testlink");
  g->stat();
  createNshowFile(*g);
  delete g;
  g = new Node("test1/testdir");
  g->stat();
  createNshowFile(*g);
  delete g;
  g = new Node("test1/subdir");
  g->stat();
  createNshowFile(*g);
  delete g;


  cout << endl << "Link change" << endl;
  Link* l = new Link("test1/testlink");
  l->stat();
  showFile(*l);
  char max_link[PATH_MAX];
  memset(max_link, '?', sizeof(max_link) - 1);
  max_link[sizeof(max_link) - 1] = '\0';
  l->setLink(max_link);
  showFile(*l);
  delete l;


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
    cout << "failed to create dir: " << strerror(errno) << endl;

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
        showList(*d);
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



  cout << endl << "Stacking read/writers test" << endl;
  report.setLevel(regression);
  {
    char buffer[4096];
    memset(buffer, 0, 4096);
    ssize_t rc;
    FileReaderWriter r("test1/testfile", false);
    char checksum[64] = "";
    Hasher fr(&r, false, Hasher::md5, checksum);
    if (fr.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      rc = fr.read(buffer, sizeof(buffer));
      if (rc < 0) {
        hlog_regression("%s reading file", strerror(errno));
      } else {
        hlog_regression("read %zd bytes: '%s'", rc, buffer);
      }
      if (fr.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      }
      hlog_regression("checksum = '%s'", checksum);
    }

    FileReaderWriter w("test1/writeback", true);
    memset(checksum, 0, sizeof(checksum));
    Hasher fw(&w, false, Hasher::md5, checksum);
    if (fw.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      rc = fw.write(buffer, rc);
      if (rc < 0) {
        hlog_regression("%s writing file", strerror(errno));
      } else {
        hlog_regression("written %zd bytes", rc);
      }
      if (fw.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      }
      hlog_regression("checksum = '%s'", checksum);
    }

    if (fr.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      rc = fr.read(buffer, sizeof(buffer));
      if (rc < 0) {
        hlog_regression("%s reading file", strerror(errno));
      } else {
        hlog_regression("read %zd bytes: '%s'", rc, buffer);
      }
      if (fr.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      }
    }
  }

  {
    char buffer[4096];
    memset(buffer, 0, 4096);
    ssize_t rc;
    system("gzip -c test1/testfile > test1/testfile.gz");
    FileReaderWriter r("test1/testfile.gz", false);
    UnzipReader fr(&r, false);
    if (fr.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      rc = fr.read(buffer, sizeof(buffer));
      if (rc < 0) {
        hlog_regression("%s reading file", strerror(errno));
      } else {
        hlog_regression("read %zd bytes: '%s'", rc, buffer);
      }
      if (fr.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      }
    }

    FileReaderWriter w("test1/writeback", true);
    ZipWriter fw(&w, false, 5);
    if (fw.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      rc = fw.write(buffer, rc);
      if (rc < 0) {
        hlog_regression("%s writing file", strerror(errno));
      } else {
        hlog_regression("written %zd bytes", rc);
      }
      if (fw.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      }
    }

    system("zcat test1/testfile.gz");
  }


  cout << endl << "End of tests" << endl;

  return 0;
}
