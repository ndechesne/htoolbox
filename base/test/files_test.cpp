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
using namespace hreport;

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
  pth0 = new Path;
  cout << pth0->length() << ": " << pth0->c_str() << endl;
  delete pth0;
  pth0 = new Path("");
  cout << pth0->length() << ": " << pth0->c_str() << endl;
  delete pth0;
  pth0 = new Path("123");
  cout << pth0->length() << ": " << pth0->c_str() << endl;
  delete pth0;
  pth0 = new Path("123/456", "");
  cout << pth0->length() << ": " << pth0->c_str() << endl;
  delete pth0;
  pth0 = new Path("", "789");
  cout << pth0->length() << ": " << pth0->c_str() << endl;
  delete pth0;
  pth0 = new Path(Path("123", "456").c_str(), "789");
  cout << pth0->length() << ": " << pth0->c_str() << endl;
  delete pth0;
  pth0 = new Path("123/456", "789/159");
  cout << pth0->length() << ": " << pth0->c_str() << endl;
  delete pth0;
  pth0 = new Path;
  *pth0 = Path("//1123//456", "7899//11599//");
  cout << pth0->length() << ": " << pth0->c_str() << endl;
  delete pth0;

  cout << endl << "Test: basename and dirname" << endl;
  pth0 = new Path("this/is a path/to a/file");
  cout << pth0->c_str();
  cout << " -> base: ";
  cout << pth0->basename();
  cout << ", dir: ";
  cout << pth0->dirname().c_str();
  cout << endl;
  delete pth0;
  pth0 = new Path("this is a file");
  cout << pth0->c_str();
  cout << " -> base: ";
  cout << pth0->basename();
  cout << ", dir: ";
  cout << pth0->dirname().c_str();
  cout << endl;
  delete pth0;
  pth0 = new Path("this is a path/");
  cout << pth0->c_str();
  cout << " -> base: ";
  cout << pth0->basename();
  cout << ", dir: ";
  cout << pth0->dirname().c_str();
  cout << endl;
  delete pth0;

  cout << endl << "Test: fromDos" << endl;
  const char* str = "a:\\backup\\";
  cout << str << " -> ";
  pth0 = new Path(str);
  cout << Path::fromDos(pth0->buffer()) << endl;
  delete pth0;
  str = "z:/backup";
  cout << str << " -> ";
  pth0 = new Path(str);
  cout << Path::fromDos(pth0->buffer()) << endl;
  delete pth0;
  str = "1:/backup";
  cout << str << " -> ";
  pth0 = new Path(str);
  cout << Path::fromDos(pth0->buffer()) << endl;
  delete pth0;
  str = "f;/backup";
  cout << str << " -> ";
  pth0 = new Path(str);
  cout << Path::fromDos(pth0->buffer()) << endl;
  delete pth0;
  str = "l:|backup";
  cout << str << " -> ";
  pth0 = new Path(str);
  cout << Path::fromDos(pth0->buffer()) << endl;
  delete pth0;
  str = "this\\is a path/ Unix\\ cannot cope with/\\";
  cout << str << " -> ";
  pth0 = new Path(str);
  char* str2 = strdup(Path::fromDos(pth0->buffer()));
  cout << str2 << endl;
  cout << str2 << " -> ";
  delete pth0;
  pth0 = new Path(str2);
  cout << Path::fromDos(pth0->buffer()) << endl;
  delete pth0;

  cout << endl << "Test: noTrailingSlashes" << endl;
  free(str2);
  str2 = strdup("this/is a path/ Unix/ cannot cope with//");
  cout << str2 << " -> ";
  pth0 = new Path(str2);
  cout << Path::noTrailingSlashes(pth0->buffer()) << endl;
  free(str2);
  str2 = strdup(Path::fromDos(pth0->buffer()));
  delete pth0;
  cout << str2 << " -> ";
  pth0 = new Path(str2);
  cout << Path::noTrailingSlashes(pth0->buffer()) << endl;
  delete pth0;
  free(str2);
  str2 = strdup("");
  cout << str2 << " -> ";
  pth0 = new Path(str2);
  cout << Path::noTrailingSlashes(pth0->buffer()) << endl;
  delete pth0;
  free(str2);
  str2 = strdup("/");
  cout << str2 << " -> ";
  pth0 = new Path(str2);
  cout << Path::noTrailingSlashes(pth0->buffer()) << endl;
  delete pth0;
  free(str2);

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
  readfile = new Stream("test1/big_file");
  if (readfile->open(O_RDONLY)) {
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
  writefile = new Stream("test1/rwfile_dest");
  if (writefile->open(O_WRONLY)) {
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
    size = writefile->putLine("line");
    if (size < 0) {
      cout << "write failed: " << strerror(errno) << endl;
    }
    write_size += size;
    size = writefile->putLine("this is a long line");
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
  readfile = new Stream("test1/big_file");
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open(O_RDONLY)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open(O_WRONLY)) {
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
  readfile = new Stream("test1/big_file");
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open(O_RDONLY)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open(O_WRONLY, 5)) {
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
    if (writefile->open(O_RDONLY, 1)) {
      cout << "Error re-opening source file: " << strerror(errno) << endl;
    } else {
      char blah;
      writefile->read(&blah, 1);
      writefile->close();
    }
  }
  delete readfile;
  delete writefile;

  cout << endl << "Test: file uncompress (uncompress read + write)" << endl;
  readfile = new Stream("test1/rwfile_dest");
  writefile = new Stream("test1/rwfile_source");
  if (readfile->open(O_RDONLY, 1)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open(O_WRONLY)) {
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
  readfile = new Stream("test1/rwfile_source");
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open(O_RDONLY)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open(O_WRONLY, 5, true)) {
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
    if (writefile->open(O_RDONLY, 1)) {
      cout << "Error re-opening source file: " << strerror(errno) << endl;
    } else {
      char blah;
      writefile->read(&blah, 1);
      writefile->close();
    }
  }
  cout << endl
    << "Test: file recompress (uncompress read + compress write)" << endl;
  {
    Stream* swap = readfile;
    readfile = writefile;
    writefile = swap;
  }
  if (readfile->open(O_RDONLY, 1)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open(O_WRONLY, 5, true)) {
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
    if (writefile->open(O_RDONLY, 1)) {
      cout << "Error re-opening source file: " << strerror(errno) << endl;
    } else {
      char blah;
      writefile->read(&blah, 1);
      writefile->close();
    }
  }
  delete readfile;
  delete writefile;


  cout << endl << "Test: file compare (both compressed)" << endl;
  readfile = new Stream("test1/rwfile_source");
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open(O_RDONLY, 1)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open(O_RDONLY, 1)) {
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
    readfile->open(O_RDONLY, 1);
    writefile->open(O_RDONLY, 1);
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
    readfile->open(O_RDONLY, 1);
    writefile->open(O_RDONLY, 1);
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
    readfile->open(O_RDONLY, 1);
    writefile->open(O_RDONLY, 1);
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

  cout << endl << "Test: file compare (one compressed)" << endl;
  readfile = new Stream("test1/rwfile_source");
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open(O_RDONLY, 1)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open(O_RDONLY)) {
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
    readfile->open(O_RDONLY, 1);
    writefile->open(O_RDONLY);
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
    readfile->open(O_RDONLY, 1);
    writefile->open(O_RDONLY);
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
    readfile->open(O_RDONLY, 1);
    writefile->open(O_RDONLY);
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
  cout << "default read" << endl;
  readfile = new Stream("test1/testfile");
  if (readfile->open(O_RDONLY)) {
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
  readfile = new Stream("test2/testfile2.gz");
  if (readfile->open(O_RDONLY)) {
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
  readfile = new Stream("test2/testfile2.gz");
  if (readfile->open(O_RDONLY, 1)) {
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

  cout << "default getLine" << endl;
  readfile = new Stream("test2/testfile2.gz");
  ssize_t read_size = 0;
  if (readfile->open(O_RDONLY)) {
    return 0;
  }
  readfile->setProgressCallback(read_progress);
  char*  buffer = NULL;
  size_t capacity = 0;
  while (true) {
    bool eol;
    ssize_t rc = readfile->getLine(&buffer, &capacity, &eol);
    if (rc < 0) {
      cout << "Error reading line, " << strerror(errno) << endl;
      return 0;
    }
    read_size += rc + 1;
    if (! eol) {
      read_size--;
      break;
    }
  }
  if (readfile->close()) {
    return 0;
  }
  cout << "Checksum: " << readfile->checksum() << endl;
  cout << "Size: " << read_size << endl;
  delete readfile;

  cout << "default decompress getLine" << endl;
  readfile = new Stream("test2/testfile2.gz");
  read_size = 0;
  if (readfile->open(O_RDONLY, 1)) {
    return 0;
  }
  readfile->setProgressCallback(read_progress);
  while (true) {
    bool eol;
    ssize_t rc = readfile->getLine(&buffer, &capacity, &eol);
    if (rc < 0) {
      cout << "Error reading line, " << strerror(errno) << endl;
      return 0;
    }
    read_size += rc + 1;
    if (! eol) {
      read_size--;
      break;
    }
  }
  if (readfile->close()) {
    return 0;
  }
  cout << "Checksum: " << readfile->checksum() << endl;
  cout << "Size: " << read_size << endl;
  delete readfile;
  free(buffer);

  cout << "default decompress read" << endl;
  readfile = new Stream("test1/rwfile_source");
  if (readfile->open(O_RDONLY, 1)) {
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
  readfile = new Stream("test1/rwfile_source");
  Node("test1/rwfile_dest").remove();
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open(O_RDONLY, 1) || writefile->open(O_WRONLY)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    int rc = readfile->copy(writefile);
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
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
  readfile = new Stream("test1/rwfile_dest");
  writefile = new Stream("test1/rwfile_copy");
  if (readfile->open(O_RDONLY) || writefile->open(O_WRONLY)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    int rc = readfile->copy(writefile);
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
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
  readfile = new Stream("test1/rwfile_copy");
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open(O_RDONLY) || writefile->open(O_WRONLY, 5)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    int rc = readfile->copy(writefile);
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
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
  readfile = new Stream("test1/rwfile_source");
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open(O_RDONLY, 1) || writefile->open(O_WRONLY)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    readfile->setCancelCallback(cancel);
    int rc = readfile->copy(writefile);
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
  Node("test1/rwfile_dest").remove();

  cout << endl << "Test: double copy" << endl;
  readfile = new Stream("test1/rwfile_source");
  writefile = new Stream("test1/rwfile_dest");
  Stream* writefile2 = new Stream("test1/rwfile_dest2");
  if (readfile->open(O_RDONLY, 1) || writefile->open(O_WRONLY, 5)
  || writefile2->open(O_WRONLY, 5)) {
    cout << "Error opening a file: " << strerror(errno) << endl;
  } else {
    int rc = readfile->copy(writefile, writefile2);
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
    if (writefile2->close()) cout << "Error closing write file 2" << endl;
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


  cout << endl << "Test: getLine" << endl;

  writefile = new Stream("test2/testfile");
  if (writefile->open(O_WRONLY)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->write(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  cout << "Checksum: " << writefile->checksum() << endl;
  delete writefile;
  readfile = new Stream("test2/testfile");
  char* line_test = NULL;
  size_t line_test_capacity = 0;
  readfile->open(O_RDONLY);
  cout << "Reading empty file:" << endl;
  while (readfile->getLine(&line_test, &line_test_capacity) > 0) {
    cout << "Line: " << line_test << endl;
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  cout << "Checksum: " << readfile->checksum() << endl;
  delete readfile;

  writefile = new Stream("test2/testfile");
  if (writefile->open(O_WRONLY)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->write("abcdef\nghi\n", 11);
    writefile->write(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  cout << "Checksum: " << writefile->checksum() << endl;
  delete writefile;
  readfile = new Stream("test2/testfile");
  if (readfile->open(O_RDONLY)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  }
  cout << "Reading uncompressed file:" << endl;
  while (readfile->getLine(&line_test, &line_test_capacity) > 0) {
    cout << "Line: " << line_test << endl;
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  cout << "Checksum: " << readfile->checksum() << endl;
  delete readfile;

  writefile = new Stream("test2/testfile");
  if (writefile->open(O_WRONLY, 5)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->write("abcdef\nghi\n", 11);
    writefile->write(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  cout << "Checksum: " << writefile->checksum() << endl;
  delete writefile;
  readfile = new Stream("test2/testfile");
  if (readfile->open(O_RDONLY, 1)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  }
  cout << "Reading compressed file:" << endl;
  while (readfile->getLine(&line_test, &line_test_capacity) > 0) {
    cout << "Line: " << line_test << endl;
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  cout << "Checksum: " << readfile->checksum() << endl;
  delete readfile;

  writefile = new Stream("test2/testfile");
  if (writefile->open(O_WRONLY, 5)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    for (int i = 0; i < 60; i++) {
      writefile->write("1234567890", 10);
    }
    writefile->write("\n1234567890", 10);
    writefile->write(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  cout << "Checksum: " << writefile->checksum() << endl;
  delete writefile;
  readfile = new Stream("test2/testfile");
  if (readfile->open(O_RDONLY, 1)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  }
  cout << "Reading compressed file (line length = 600):" << endl;
  while (readfile->getLine(&line_test, &line_test_capacity) > 0) {
    cout << "Line: " << line_test << endl;
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  cout << "Checksum: " << readfile->checksum() << endl;
  delete readfile;

  readfile = new Stream("test2/testfile");
  if (readfile->open(O_RDONLY, 1, false)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  }
  cout << "Reading compressed file (line length = 600), no checksum:" << endl;
  while (readfile->getLine(&line_test, &line_test_capacity) > 0) {
    cout << "Line: " << line_test << endl;
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  cout << "Checksum: " << readfile->checksum() << endl;
  delete readfile;
  free(line_test);


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
