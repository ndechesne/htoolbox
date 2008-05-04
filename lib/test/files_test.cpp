/*
     Copyright (C) 2006-2008  Herve Fache

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
#include <sys/stat.h>
#include <errno.h>

using namespace std;

#include "line.h"
#include "files.h"
#include "hbackup.h"

using namespace hbackup;

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
          parseList(di, dir_path);
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

static void showList(const Directory* d, int level = 0);

static void defaultShowFile(const Node* g) {
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

static void showFile(const Node* g, int level = 1) {
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
          << ", size = " << (d->size() != 0)
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

static void showList(const Directory* d, int level) {
  if (level == 0) {
    showFile(d, level);
  }
  ++level;
  list<Node*>::const_iterator i;
  for (i = d->nodesListConst().begin(); i != d->nodesListConst().end(); i++) {
    if (! strcmp((*i)->name(), "cvs")) continue;
    if (! strcmp((*i)->name(), "svn")) continue;
    if (! strcmp((*i)->name(), "bzr")) continue;
    showFile(*i, level);
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

bool cancel(unsigned short __unused) {
  return true;
}

static void progress(long long previous, long long current, long long total) {
  if ((current <= total) && ((current < total) || (previous != 0))) {
    cout << "Done: " << setw(5) << setiosflags(ios::fixed)
      << setprecision(1) << 100.0 * current /total << "%" << endl;
  }
}

int main(void) {
  string  line;

  cout << "Tools Test" << endl;
  setVerbosityLevel(debug);

  mode_t mask = umask(0022);
  mask = umask(0022);
  printf("Our mask = 0%03o\n", mask);

  cout << endl << "Test: constructors / countBlocks" << endl;
  Path* pth0;
  pth0 = new Path;
  cout << pth0->length() << ": " << *pth0 << endl;
  delete pth0;
  pth0 = new Path("");
  cout << pth0->length() << ": " << *pth0 << endl;
  delete pth0;
  pth0 = new Path("123");
  cout << pth0->length() << ": " << *pth0 << endl;
  delete pth0;
  pth0 = new Path("123/456", "");
  cout << pth0->length() << ": " << *pth0 << endl;
  delete pth0;
  pth0 = new Path("", "789");
  cout << pth0->length() << ": " << *pth0 << endl;
  delete pth0;
  pth0 = new Path(Path("123", "456"), "789");
  cout << pth0->length() << ": " << *pth0 << endl;
  delete pth0;
  pth0 = new Path("123/456", "789/159");
  cout << pth0->length() << ": " << *pth0 << endl;
  delete pth0;
  pth0 = new Path;
  *pth0 = Path("//1123//456", "7899//11599//");
  cout << pth0->length() << ": " << *pth0 << endl;
  delete pth0;

  cout << endl << "Test: basename and dirname" << endl;
  pth0 = new Path("this/is a path/to a/file");
  cout << *pth0;
  cout << " -> base: ";
  cout << pth0->basename();
  cout << ", dir: ";
  cout << pth0->dirname();
  cout << endl;
  delete pth0;
  pth0 = new Path("this is a file");
  cout << *pth0;
  cout << " -> base: ";
  cout << pth0->basename();
  cout << ", dir: ";
  cout << pth0->dirname();
  cout << endl;
  delete pth0;
  pth0 = new Path("this is a path/");
  cout << *pth0;
  cout << " -> base: ";
  cout << pth0->basename();
  cout << ", dir: ";
  cout << pth0->dirname();
  cout << endl;
  delete pth0;

  cout << endl << "Test: fromDos" << endl;
  const char* str = "a:\\backup\\";
  cout << str << " -> ";
  pth0 = new Path(str);
  cout << pth0->fromDos() << endl;
  delete pth0;
  str = "z:/backup";
  cout << str << " -> ";
  pth0 = new Path(str);
  cout << pth0->fromDos() << endl;
  delete pth0;
  str = "1:/backup";
  cout << str << " -> ";
  pth0 = new Path(str);
  cout << pth0->fromDos() << endl;
  delete pth0;
  str = "f;/backup";
  cout << str << " -> ";
  pth0 = new Path(str);
  cout << pth0->fromDos() << endl;
  delete pth0;
  str = "l:|backup";
  cout << str << " -> ";
  pth0 = new Path(str);
  cout << pth0->fromDos() << endl;
  delete pth0;
  str = "this\\is a path/ Unix\\ cannot cope with/\\";
  cout << str << " -> ";
  pth0 = new Path(str);
  char* str2 = strdup(pth0->fromDos());
  cout << str2 << endl;
  cout << str2 << " -> ";
  pth0 = new Path(str2);
  cout << pth0->fromDos() << endl;
  delete pth0;

  cout << endl << "Test: noTrailingSlashes" << endl;
  str2 = strdup("this/is a path/ Unix/ cannot cope with//");
  cout << str2 << " -> ";
  pth0 = new Path(str2);
  cout << pth0->noTrailingSlashes() << endl;
  str2 = strdup(pth0->fromDos());
  delete pth0;
  cout << str2 << " -> ";
  pth0 = new Path(str2);
  cout << pth0->noTrailingSlashes() << endl;
  delete pth0;
  str2 = strdup("");
  cout << str2 << " -> ";
  pth0 = new Path(str2);
  cout << pth0->noTrailingSlashes() << endl;
  delete pth0;
  str2 = strdup("/");
  cout << str2 << " -> ";
  pth0 = new Path(str2);
  cout << pth0->noTrailingSlashes() << endl;
  delete pth0;

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
  if (readfile->open("r", 0, 0)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else {
    readfile->setProgressCallback(progress);
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
    cout << "read size: " << read_size << " (" << readfile->size()
      << "), checksum: " << readfile->checksum() << endl;
  }
  delete readfile;

  cout << endl << "Test: file write (with cache)" << endl;
  writefile = new Stream("test1/rwfile_dest");
  if (writefile->open("w", 0, 3)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else {
    writefile->setProgressCallback(progress);
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
    if (writefile->close()) cout << "Error closing file" << endl;
    cout << "write size: " << write_size << " (" << writefile->size()
      << "), checksum: " << writefile->checksum() << endl;
  }
  delete writefile;
  system("cat test1/rwfile_dest");

  cout << endl << "Test: file copy (read + write (no cache))" << endl;
  readfile = new Stream("test1/big_file");
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open("r", 0, Stream::chunk + 1)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open("w", 0, 0)) {
    cout << "Error opening dest file: " << strerror(errno) << endl;
  } else {
    readfile->setProgressCallback(progress);
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
    cout << "read size: " << read_size << " (" << readfile->size()
      << "), checksum: " << readfile->checksum() << endl;
    cout << "write size: " << write_size << " (" << writefile->size()
      << "), checksum: " << writefile->checksum() << endl;
  }
  delete readfile;
  delete writefile;

  cout << endl << "Test: file compress (read + default compress write), "
    << "with last zero-size write call" << endl;
  readfile = new Stream("test1/big_file");
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open("r", 0, -1)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open("w", 5, -1)) {
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
    cout << "read size: " << read_size << " (" << readfile->size()
      << "), checksum: " << readfile->checksum() << endl;
    cout << "write size (wrong): " << write_size << " (" << writefile->size()
      << "), checksum: " << writefile->checksum() << endl;
  }
  delete readfile;
  delete writefile;

  cout << endl << "Test: file uncompress (uncompress read + write)" << endl;
  readfile = new Stream("test1/rwfile_dest");
  writefile = new Stream("test1/rwfile_source");
  if (readfile->open("r", 1, -1)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open("w", 0, 102)) {
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
    cout << "read size: " << read_size << " (" << readfile->size()
      << "), checksum: " << readfile->checksum() << endl;
    cout << "write size: " << write_size << " (" << writefile->size()
      << "), checksum: " << writefile->checksum() << endl;
  }
  delete readfile;
  delete writefile;

  cout << endl << "Test: file compress (read + cached compress write)"
    << endl;
  readfile = new Stream("test1/rwfile_source");
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open("r", 0, 100)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open("w", 5, 100)) {
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
    cout << "read size: " << read_size << " (" << readfile->size()
      << "), checksum: " << readfile->checksum() << endl;
    cout << "write size (wrong): " << write_size << " (" << writefile->size()
      << "), checksum: " << writefile->checksum() << endl;
  }
  cout << endl
    << "Test: file recompress (uncompress read + uncached compress write), "
      << "no closing" << endl;
  {
    Stream* swap = readfile;
    readfile = writefile;
    writefile = swap;
  }
  if (readfile->open("r", 1, 10)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open("w", 5, 0)) {
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
    cout << "read size: " << read_size << " (" << readfile->size()
      << "), checksum: " << readfile->checksum() << endl;
    cout << "write size (wrong): " << write_size << " (" << writefile->size()
      << "), checksum: " << writefile->checksum() << endl;
  }
  delete readfile;
  delete writefile;


  cout << endl
    << "Test: file compare (both compressed)"
    << endl;
  readfile = new Stream("test1/rwfile_source");
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open("r", 1, -1)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open("r", 1, -1)) {
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
    readfile->open("r", 1, -1);
    writefile->open("r", 1, -1);
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
    readfile->open("r", 1, -1);
    writefile->open("r", 1, -1);
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
    readfile->open("r", 1, -1);
    writefile->open("r", 1, -1);
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
  if (readfile->open("r", 1, -1)) {
    cout << "Error opening source file: " << strerror(errno) << endl;
  } else if (writefile->open("r", 0, -1)) {
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
    readfile->open("r", 1, -1);
    writefile->open("r", 0, -1);
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
    readfile->open("r", 1, -1);
    writefile->open("r", 0, -1);
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
    readfile->open("r", 1, -1);
    writefile->open("r", 0, -1);
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
  if (readfile->open("r", 0, -1)) {
    return 0;
  }
  readfile->setProgressCallback(progress);
  if (readfile->computeChecksum()) {
    cout << "Error computing checksum, " << strerror(errno) << endl;
    return 0;
  }
  if (readfile->close()) {
    return 0;
  }
  cout << "Checksum: " << readfile->checksum() << endl;
  delete readfile;

  readfile = new Stream("test2/testfile2.gz");
  if (readfile->open("r", 0, -1)) {
    return 0;
  }
  readfile->setProgressCallback(progress);
  if (readfile->computeChecksum()) {
    cout << "Error computing checksum, " << strerror(errno) << endl;
    return 0;
  }
  if (readfile->close()) {
    return 0;
  }
  cout << "Checksum: " << readfile->checksum() << endl;
  delete readfile;

  readfile = new Stream("test2/testfile2.gz");
  if (readfile->open("r", 1, -1)) {
    return 0;
  }
  readfile->setProgressCallback(progress);
  if (readfile->computeChecksum()) {
    cout << "Error computing checksum, " << strerror(errno) << endl;
    return 0;
  }
  if (readfile->close()) {
    return 0;
  }
  cout << "Checksum: " << readfile->checksum() << endl;
  delete readfile;

  readfile = new Stream("test2/testfile2.gz");
  ssize_t read_size = 0;
  if (readfile->open("r", 0, -1)) {
    return 0;
  }
  readfile->setProgressCallback(progress);
  while (true) {
    Line line;
    bool eol;
    int rc = readfile->getLine(line, &eol);
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

  readfile = new Stream("test2/testfile2.gz");
  read_size = 0;
  if (readfile->open("r", 1, -1)) {
    return 0;
  }
  readfile->setProgressCallback(progress);
  while (true) {
    Line line;
    bool eol;
    int rc = readfile->getLine(line, &eol);
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

  readfile = new Stream("test1/rwfile_source");
  if (readfile->open("r", 1, -1)) {
    return 0;
  }
  readfile->setProgressCallback(progress);
  if (readfile->computeChecksum()) {
    cout << "Error computing checksum, " << strerror(errno) << endl;
    return 0;
  }
  if (readfile->close()) {
    return 0;
  }
  cout << "Checksum: " << readfile->checksum() << endl;
  delete readfile;


  cout << endl << "Test: copy" << endl;
  readfile = new Stream("test1/rwfile_source");
  Node("test1/rwfile_dest").remove();
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open("r", 1, -1) || writefile->open("w", 0, -1)) {
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
      cout << "size in: " << readfile->size() << endl;
      cout << "size out: " << writefile->size() << endl;
    }
  }
  delete readfile;
  delete writefile;
  readfile = new Stream("test1/rwfile_dest");
  writefile = new Stream("test1/rwfile_copy");
  if (readfile->open("r", 0, -1) || writefile->open("w", 0, -1)) {
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
      cout << "size in: " << readfile->size() << endl;
      cout << "size out: " << writefile->size() << endl;
    }
  }
  delete readfile;
  delete writefile;
  Node("test1/rwfile_dest").remove();
  readfile = new Stream("test1/rwfile_copy");
  writefile = new Stream("test1/rwfile_dest");
  if (readfile->open("r", 0, -1) || writefile->open("w", 5, -1)) {
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
  if (readfile->open("r", 1, -1) || writefile->open("w", 0, -1)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->setCancelCallback(cancel);
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


  cout << endl << "Test: getLine" << endl;

  writefile = new Stream("test2/testfile");
  if (writefile->open("w", 0, -1)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->write(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  cout << "Checksum: " << writefile->checksum() << endl;
  delete writefile;
  readfile = new Stream("test2/testfile");
  char*        line_test = NULL;
  unsigned int line_test_capacity = 0;
  readfile->open("r", 0, -1);
  cout << "Reading empty file:" << endl;
  while (readfile->getLine(&line_test, &line_test_capacity) > 0) {
    cout << "Line: " << line_test << endl;
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  cout << "Checksum: " << readfile->checksum() << endl;
  delete readfile;

  writefile = new Stream("test2/testfile");
  if (writefile->open("w", 0, -1)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->write("abcdef\nghi\n", 11);
    writefile->write(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  cout << "Checksum: " << writefile->checksum() << endl;
  delete writefile;
  readfile = new Stream("test2/testfile");
  if (readfile->open("r", 0, -1)) {
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
  if (writefile->open("w", 5, -1)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  } else {
    writefile->write("abcdef\nghi\n", 11);
    writefile->write(NULL, 0);
    if (writefile->close()) cout << "Error closing write file" << endl;
  }
  cout << "Checksum: " << writefile->checksum() << endl;
  delete writefile;
  readfile = new Stream("test2/testfile");
  if (readfile->open("r", 1, -1)) {
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
  if (writefile->open("w", 5, -1)) {
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
  if (readfile->open("r", 1, -1)) {
    cout << "Error opening file: " << strerror(errno) << endl;
  }
  cout << "Reading compressed file (line length = 600):" << endl;
  while (readfile->getLine(&line_test, &line_test_capacity) > 0) {
    cout << "Line: " << line_test << endl;
  }
  if (readfile->close()) cout << "Error closing read file" << endl;
  cout << "Checksum: " << readfile->checksum() << endl;
  delete readfile;


  cout << endl << "extractParams" << endl;
  vector<string> *params;
  vector<string>::iterator i;

  // Start simple: one argument
  line = "a";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Stream::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Two arguments, test blanks
  line = " \ta \tb";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Stream::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Not single character argument
  line = "\t ab";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Stream::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Two of them
  line = "\t ab cd";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Stream::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Three, with comment
  line = "\t ab cd\tef # blah";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Stream::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Single quotes
  line = "\t 'ab' 'cd'\t'ef' # blah";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Stream::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // And double quotes
  line = "\t \"ab\" 'cd'\t\"ef\" # blah";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Stream::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // With blanks in quotes
  line = "\t ab cd\tef 'gh ij\tkl' \"mn op\tqr\" \t# blah";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Stream::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // With quotes in quotes
  line = "\t ab cd\tef 'gh \"ij\\\'\tkl' \"mn 'op\\\"\tqr\" \t# blah";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Stream::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // With escape characters
  line = "\t a\\b cd\tef 'g\\h \"ij\\\'\tkl' \"m\\n 'op\\\"\tqr\" \t# blah";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Stream::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Missing ending single quote
  line = "\t a\\b cd\tef 'g\\h \"ij\\\'\tkl";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Stream::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Missing ending double quote
  line = "\t a\\b cd\tef 'g\\h \"ij\\\'\tkl' \"m\\n 'op\\\"\tqr";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Stream::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // The DOS catch: undealt with
  line = "path \"C:\\Backup\\\"";
  params = new vector<string>;
  cout << "readline(" << line << "): " <<
    Stream::extractParams(line.c_str(), *params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // The DOS catch: ignoring escape characters altogether
  line = "path \"C:\\Backup\\\"";
  params = new vector<string>;
  cout << "readline(" << line << "): "
    << Stream::extractParams(line.c_str(), *params, Stream::flags_no_escape)
    << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // The DOS catch: dealing with the particular case
  line = "path \"C:\\Backup\\\"";
  params = new vector<string>;
  cout << "readline(" << line << "): "
    << Stream::extractParams(line.c_str(), *params, Stream::flags_dos_catch)
    << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Extract a line where spaces matter
  line = " this is a path \"C:\\Backup\"   and  spaces matter ";
  params = new vector<string>;
  cout << "readline(" << line << "): "
    << Stream::extractParams(line.c_str(), *params, Stream::flags_dos_catch
      | Stream::flags_empty_params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // Extract part of a line where spaces matter
  line = " this is a path \"C:\\Backup\"   and  spaces matter ";
  params = new vector<string>;
  cout << "readline(" << line << "): "
    << Stream::extractParams(line.c_str(), *params, Stream::flags_dos_catch
      | Stream::flags_empty_params, 5) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // And another
  line = "\t\tf\tblah\t''";
  params = new vector<string>;
  cout << "readline(" << line << "): "
    << Stream::extractParams(line.c_str(), *params, Stream::flags_dos_catch
      | Stream::flags_empty_params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // And one missing the ending quote
  line = "\t\tf\tblah\t'";
  params = new vector<string>;
  cout << "readline(" << line << "): "
    << Stream::extractParams(line.c_str(), *params, Stream::flags_dos_catch
      | Stream::flags_empty_params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;

  // And another one missing with empty last argument
  line = "\t\tf\tblah\t\t";
  params = new vector<string>;
  cout << "readline(" << line << "): "
    << Stream::extractParams(line.c_str(), *params, Stream::flags_dos_catch
      | Stream::flags_empty_params) << endl;
  for (i = params->begin(); i != params->end(); i++) {
    cout << *i << endl;
  }
  cout << endl;
  delete params;


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

  cout << endl << "Validity tests" << endl;
  cout << "File is file? " << File("test1/testfile", -1).isValid() << endl;
  cout << "File is dir? " << Directory("test1/testfile").isValid() << endl;
  cout << "File is link? " << Link("test1/testfile").isValid() << endl;
  cout << "Dir is file? " << File("test1/testdir").isValid() << endl;
  cout << "Dir is dir? " << Directory("test1/testdir",
    strlen("test1/testdir")).isValid() << endl;
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


  cout << endl << "getParams" << endl;
  Stream config("etc/hbackup.conf");
  if (! config.open("r", 0, -1)) {
    int rc = 1;
    while (rc > 0) {
      params = new vector<string>;
      rc = config.getParams(*params);
      if (rc > 0) {
        for (vector<string>::iterator i = params->begin(); i != params->end();
            i++) {
          cout << *i << "\t";
        }
        cout << endl;
      }
      delete params;
    }
    config.close();
  }


  cout << endl << "Stream select" << endl;
  Stream* selected;
  unsigned int which;
  vector<string> extensions;
  extensions.push_back("");
  extensions.push_back(".gz");
  extensions.push_back(".bz2");
  selected = Stream::select("data", extensions, &which);
  if (selected != NULL) {
    cout << "Found (" << which << "): " << selected->path() << endl;
    delete selected;
  } else {
    cout << "Not found." << endl;
  }

  Stream datagz("data.gz");
  if (! datagz.open("w", 5, -1)) {
    datagz.write("compressed\n", strlen("compressed\n"));
    datagz.close();
  }
  selected = Stream::select("data", extensions, &which);
  if (selected != NULL) {
    cout << "Found (" << which << "): " << selected->path() << endl;
    delete selected;
  } else {
    cout << "Not found." << endl;
  }

  Stream data("data");
  if (! data.open("w", 0, -1)) {
    data.write("not compressed\n", strlen("not compressed\n"));
    data.close();
  }
  selected = Stream::select("data", extensions, &which);
  if (selected != NULL) {
    cout << "Found (" << which << "): " << selected->path() << endl;
    delete selected;
  } else {
    cout << "Not found." << endl;
  }


  cout << endl << "End of tests" << endl;

  return 0;
}
