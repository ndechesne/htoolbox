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
#include <string>
#include <list>

#include <cctype>
#include <cstdio>

#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

using namespace std;

#include "buffer.h"
#include "line.h"
#include "filereader.h"
#include "unzipreader.h"
#include "filewriter.h"
#include "md5sumhasher.h"
#include "zipwriter.h"
#include "files.h"

using namespace hbackup;

Path::Path(const char* dir, const char* name) {
  *this = dir;
  if (name[0] != '\0') {
    *this += "/";
    *this += name;
  }
}

Path::Path(const Path& path, const char* name) {
  *this = path;
  *this += "/";
  *this += name;
}

const char* Path::basename(const char* path) {
  const char* name = strrchr(path, '/');
  if (name != NULL) {
    name++;
  } else {
    name = path;
  }
  return name;
}

Path Path::dirname() const {
  int pos = rfind('/');
  if (pos < 0) {
    return ".";
  }
  Path rc = *this;
  rc.erase(pos);
  return rc;
}

const Path& Path::fromDos() {
  fromDos(buffer());
  return *this;
}

const Path& Path::noTrailingSlashes() {
  noTrailingSlashes(buffer());
  return *this;
}

const char* Path::fromDos(char* path) {
  bool proven = false;
  size_t size = 0;
  for (char* reader = path; *reader != '\0'; ++reader) {
    if (*reader == '\\') {
      *reader = '/';
      proven = true;
    }
    ++size;
  }
  // Upper case drive letter
  if (proven && (size >= 3)) {
    if ((path[1] == ':') && (path[2] == '/') && islower(path[0])) {
      path[0] = static_cast<char>(toupper(path[0]));
    }
  }
  return path;
}

const char* Path::noTrailingSlashes(char* path) {
  char* end = &path[strlen(path)];
  while ((--end >= path) && (*end == '/')) {}
  end[1] = '\0';
  return path;
}

int Path::compare(const char* s1, const char* s2, ssize_t length) {
  while (true) {
    if (length-- == 0) {
      return 0;
    }
    if (*s1 == *s2) {
      if (*s1 != '\0') {
        s1++;
        s2++;
        continue;
      } else {
        return 0;
      }
    }
    if (*s1 == '\0') {
      return -1;
    } else
    if (*s2 == '\0') {
      return 1;
    } else
    if (*s1 == '/') {
      // For TAB and LF
      if ((*s2 == '\t') || (*s2 == '\n')) {
        return 1;
      } else {
        return -1;
      }
    } else
    if (*s2 == '/') {
      if ((*s1 == '\t') || (*s1 == '\n')) {
        return -1;
      } else {
        return 1;
      }
    } else {
      return *s1 < *s2 ? -1 : 1;
    }
  }
}

int Node::stat() {
  struct stat64 metadata;
  int rc = lstat64(_path, &metadata);
  if (rc) {
    // errno set by lstat
    _type = '?';
  } else {
    if (S_ISREG(metadata.st_mode))       _type = 'f';
    else if (S_ISDIR(metadata.st_mode))  _type = 'd';
    else if (S_ISCHR(metadata.st_mode))  _type = 'c';
    else if (S_ISBLK(metadata.st_mode))  _type = 'b';
    else if (S_ISFIFO(metadata.st_mode)) _type = 'p';
    else if (S_ISLNK(metadata.st_mode))  _type = 'l';
    else if (S_ISSOCK(metadata.st_mode)) _type = 's';
    else                                 _type = '?';
    // Fill in file information
    _size  = metadata.st_size;
    _mtime = metadata.st_mtime;
    _uid   = metadata.st_uid;
    _gid   = metadata.st_gid;
    _mode  = metadata.st_mode & ~S_IFMT;
  }
  return rc;
}

bool Node::operator!=(const Node& right) const {
  return (_type != right._type)   || (_mtime != right._mtime)
      || (_size != right._size)   || (_uid != right._uid)
      || (_gid != right._gid)     || (_mode != right._mode)
      || (strcmp(_path.basename(), right._path.basename()) != 0);
}

int Node::remove() {
  int rc = ::remove(_path);
  if (! rc) {
    _type = '?';
  }
  return rc;
}

int File::create() {
  if (_type == '?') {
    stat();
  }
  if (isValid()) {
    errno = EEXIST;
    // Only a warning
    return 1;
  } else {
    int readfile = ::open(_path, O_WRONLY | O_CREAT, 0666);
    if (readfile < 0) {
      return -1;
    }
    close(readfile);
    stat();
  }
  return 0;
}

static int direntFilter(const struct dirent* a) {
  if (a->d_name[0] != '.') return 1;
  if (a->d_name[1] == '\0') return 0;
  if (a->d_name[1] != '.') return 1;
  if (a->d_name[2] == '\0') return 0;
  return 1;
}

static int direntCompare(const dirent** a, const dirent** b) {
  return Path::compare((*a)->d_name, (*b)->d_name);
}

int Directory::createList() {
  if (_nodes != NULL) {
    errno = EBUSY;
    return -1;
  }
  // Create list
  struct dirent** direntList;
  int size = scandir(_path, &direntList, direntFilter, direntCompare);
  if (size < 0) {
    return -1;
  }
  // Cleanup list
  int size2 = size;
  while (size2--) {
    free(direntList[size2]);
  }
  free(direntList);
  // Bug in CIFS client, usually gets detected by two subsequent dir reads
  size2 = scandir(_path, &direntList, direntFilter, direntCompare);
  if (size != size2) {
    errno = EAGAIN;
    size = size2;
    size2 = -1;
  }
  if (size2 < 0) {
    while (size--) {
      free(direntList[size]);
    }
    free(direntList);
    return -1;
  }
  // Ok, let's parse
  _nodes = new list<Node*>;
  bool failed = false;
  while (size--) {
    Node *g = new Node(Path(_path, direntList[size]->d_name));
    free(direntList[size]);
    if (! g->stat()) {
      switch (g->type()) {
        case 'f': {
          File *f = new File(*g);
          delete g;
          g = f;
        } break;
        case 'd': {
          Directory *d = new Directory(*g);
          delete g;
          g = d;
        } break;
        case 'l': {
          Link *l = new Link(*g);
          delete g;
          g = l;
        } break;
        default:;
      }
    } else {
      failed = true;
    }
    _nodes->push_front(g);
  }
  free(direntList);
  return failed ? -1 : 0;
}

void Directory::deleteList() {
  if (_nodes != NULL) {
    list<Node*>::iterator i = _nodes->begin();
    while (i != _nodes->end()) {
      delete *i;
      i++;
    }
    delete _nodes;
    _nodes = NULL;
  }
}

int Directory::create() {
  if (_type == '?') {
    stat();
  }
  if (isValid()) {
    errno = EEXIST;
    // Only a warning
    return 1;
  } else {
    if (mkdir(_path, 0777)) {
      return -1;
    }
    stat();
  }
  return 0;
}


bool Link::operator!=(const Link& right) const {
  if (static_cast<Node>(*this) != static_cast<Node>(right)) {
    return true;
  }
  return strcmp(_link, right._link) != 0;
}


class GzipExtraFieldSize {
  union {
    char            bytes[12];
    struct {
      char          si1;
      char          si2;
      short         len;
      long long     size;
    } fields;
  } extra;
public:
  GzipExtraFieldSize(long long size) {
    extra.fields.si1  = 'S';
    extra.fields.si2  = 'Z';
    extra.fields.len  = 8;
    extra.fields.size = size;
  }
  GzipExtraFieldSize(const unsigned char* field) {
    extra.fields.size = -1;
    if ((field[0] == 'S') && (field[1] == 'Z')) {
      memcpy(extra.bytes, field, 4);
      if (extra.fields.len <= 8) {
        memcpy(&extra.bytes[4], &field[4], extra.fields.len);
      } else {
        extra.fields.len  = 0;
      }
    } else {
      extra.fields.si1  = '-';
    }
  }
  operator unsigned char* () {
    return reinterpret_cast<unsigned char*>(extra.bytes);
  }
  operator long long () {
    return extra.fields.size;
  }
  int length() const {
    if (extra.fields.si1 != 'S') {
      return -1;
    }
    if (extra.fields.len == 0) {
      return 0;
    }
    return extra.fields.len + 4;
  }
};


struct Stream::Private {
  FileReader*         fr;                 // file reader
  UnzipReader*        zr;                 // unzip
  IReader*            r;                  // reader
  FileWriter*         fw;                 // file writer
  ZipWriter*          zw;                 // zip
  IWriter*            w;                  // writer
  MD5SumHasher*       hh;                 // hash computation
  char                hash[64];           // file hash
  long long           size;               // uncompressed file data size (bytes)
  long long           progress;           // transfered size, for progress
  Buffer<char>        buffer_data;        // Buffer for data
  BufferReader<char>  reader_data;        // Reader for buffer above
  progress_f          progress_callback;  // function to report progress
  cancel_f            cancel_callback;    // function to check for cancellation
  Private() : reader_data(buffer_data) {}
};

Stream::Stream(Path path) : File(path), _d(new Private) {
  _d->fr                = NULL;
  _d->zr                = NULL;
  _d->r                 = NULL;
  _d->fw                = NULL;
  _d->zw                = NULL;
  _d->w                 = NULL;
  _d->hh                = NULL;
  _d->progress_callback = NULL;
  _d->cancel_callback   = NULL;
  _d->size              = -1;
}

Stream::~Stream() {
  if (isOpen()) {
    close();
  }
  delete _d;
}

bool Stream::isOpen() const {
  return (_d->r != NULL) || (_d->w != NULL);
}

bool Stream::isWriteable() const {
  return _d->w != NULL;
}

int Stream::open(
    int             flags,
    unsigned int    compression,
    bool            checksum) {
  if (isOpen()) {
    errno = EBUSY;
    return -1;
  }
  _d->size     = 0;
  _d->progress = 0;
  int rc = 0;
  switch (flags) {
  case O_WRONLY:
    _size = 0;
    _d->w = _d->fw = new FileWriter(_path);
    if (checksum) {
      _d->w = _d->hh = new MD5SumHasher(*_d->w, _d->hash);
    } else {
      _d->hh = NULL;
    }
    if (compression > 0) {
      _d->w = _d->zw = new ZipWriter(*_d->w, compression);
    } else {
      _d->zw = NULL;
    }
    rc = _d->w->open();
    if (rc < 0) {
      if (_d->zw != NULL) {
        delete _d->zw;
      }
      if (_d->hh != NULL) {
        delete _d->hh;
      }
      delete _d->fw;
      _d->w = NULL;
    }
    break;
  case O_RDONLY:
    _d->r = _d->fr = new FileReader(_path);
    if (compression > 0) {
      _d->r = _d->zr = new UnzipReader(*_d->r);
    } else {
      _d->zr = NULL;
    }
    if (checksum) {
      _d->r = _d->hh = new MD5SumHasher(*_d->r, _d->hash);
    } else {
      _d->hh = NULL;
    }
    rc = _d->r->open();
    if (rc < 0) {
      if (_d->hh != NULL) {
        delete _d->hh;
      }
      if (_d->zr != NULL) {
        delete _d->zr;
      }
      delete _d->fr;
      _d->r = NULL;
    }
    break;
  default:
    errno = EINVAL;
    rc = -1;
  }
  return rc;
}

int Stream::close() {
  if (! isOpen()) {
    errno = EBADF;
    return -1;
  }
  int rc = 0;
  if (_d->r != NULL) {
    rc = _d->r->close();
    if (_d->hh != NULL) {
      free(_checksum);
      _checksum = strdup(_d->hash);
      delete _d->hh;
    }
    if (_d->zr != NULL) {
      delete _d->zr;
    }
    delete _d->fr;
    _d->r = NULL;
    // Destroy cacheing buffer if any
    if (_d->buffer_data.exists()) {
      _d->buffer_data.destroy();
    }
  }
  if (_d->w != NULL) {
    rc = _d->w->close();
    if (_d->zw != NULL) {
      delete _d->zw;
    }
    if (_d->hh != NULL) {
      free(_checksum);
      _checksum = strdup(_d->hash);
      delete _d->hh;
    }
    delete _d->fw;
    _d->w = NULL;
    // Update metadata
    stat();
  }
  return rc;
}

long long Stream::progress() const {
  return _d->progress;
}

void Stream::setProgressCallback(progress_f progress) {
  _d->progress_callback = progress;
}

ssize_t Stream::read(void* buffer, size_t asked) {
  if (! isOpen()) {
    errno = EBADF;
    return -1;
  }
  if (isWriteable()) {
    errno = EPERM;
    return -1;
  }
  // Get data
  ssize_t size = _d->r->read(buffer, asked);
  // Check result
  if (size < 0) {
    // errno set by read
    return -1;
  }
  // Update progress indicator (size read)
  if (size > 0) {
    long long previous = _d->progress;
    _d->progress = _d->r->offset();
    if ((_d->progress_callback != NULL) && (_d->progress != previous)) {
      (*_d->progress_callback)(previous, _d->progress, _size);
    }
    _d->size += size;
  }
  return size;
}

ssize_t Stream::write(
    const void*     buffer,
    size_t          size) {
  if (! isOpen()) {
    errno = EBADF;
    return -1;
  }
  if (! isWriteable()) {
    errno = EPERM;
    return -1;
  }
  // Write data to file
  ssize_t ssize = size;
  if (_d->w->write(buffer, size) != ssize) {
    return -1;
  }
  // Update progress indicator (size written)
  if (size > 0) {
    long long previous = _d->progress;
    _d->progress += size;
    if (_d->progress_callback != NULL) {
      (*_d->progress_callback)(previous, _d->progress, _size);
    }
    // Writing, so uncompressed data size and size are the same
    _size += size;
    _d->size += size;
  }
  return size;
}

ssize_t Stream::getLine(
    char**          buffer,
    size_t*         buffer_capacity,
    bool*           end_of_line_found) {
  // Need a buffer to speed things up
  if (! _d->buffer_data.exists()) {
    _d->buffer_data.create();
  }
  // Initialize return values
  bool         found  = false;
  unsigned int count  = 0;
  char*        writer = *buffer;
  // Make sure we have a buffer
  if (*buffer == NULL) {
    *buffer_capacity = 1024;
    *buffer = static_cast<char*>(malloc(*buffer_capacity));
    writer = *buffer;
  }
  // Find end of line or end of file
  do {
    // Fill up the buffer
    ssize_t size = _d->reader_data.readable();
    if (size == 0) {
      size = read(_d->buffer_data.writer(), _d->buffer_data.writeable());
      if (size < 0) {
        return size;
      }
      if (size == 0) {
        break;
      }
      _d->buffer_data.written(size);
    }
    const char* reader = _d->reader_data.reader();
    size_t      length = size;
    while (length > 0) {
      length--;
      // Check for space
      if (count >= *buffer_capacity) {
        *buffer_capacity <<= 1;
        *buffer = static_cast<char*>(realloc(*buffer, *buffer_capacity));
        writer = &(*buffer)[count];
      }
      if (*reader == '\n') {
        found = true;
        break;
      }
      *writer++ = *reader++;
      count++;
    }
    _d->reader_data.readn(size - length);
  } while (! found);
  *writer = '\0';

  if (end_of_line_found != NULL) {
    *end_of_line_found = found;
  }
  return count;
}

ssize_t Stream::putLine(
    const char*     buffer) {
  ssize_t length = strlen(buffer);
  ssize_t size   = write(buffer, length);
  if (size < length) {
    return -1;
  }
  if (write("\n", 1) != 1) {
    return -1;
  }
  return size + 1;
}

void Stream::setCancelCallback(cancel_f cancel) {
  _d->cancel_callback = cancel;
}

int Stream::computeChecksum() {
  if (! isOpen()) {
    errno = EBADF;
    return -1;
  }
  ssize_t length;
  do {
    char buffer[102400];
    length = read(buffer, sizeof(buffer));
    if (length < 0) {
      return length;
    }
    if ((_d->cancel_callback != NULL) && ((*_d->cancel_callback)(1))) {
      errno = ECANCELED;
      return -1;
    }
  } while (length > 0);
  return 0;
}

struct CopyData {
  // Global
  Stream*             stream;
  Buffer<char>&       buffer;
  sem_t&              read_sem;
  bool&               eof;
  bool&               failed;
  // Local
  BufferReader<char>  reader;
  sem_t               write_sem;
  CopyData(Stream* d, Buffer<char>& b, sem_t& s, bool& eof, bool& failed) :
      stream(d), buffer(b), read_sem(s), eof(eof), failed(failed),
      // No auto-unregistration: segentation fault may occur
      reader(buffer, false) {
    sem_init(&write_sem, 0, 0);
  }
  ~CopyData() {
    sem_destroy(&write_sem);
  }
};

struct ReadData {
  sem_t         read_sem;
  ReadData() {
    sem_init(&read_sem, 0, 0);
  }
  ~ReadData() {
    sem_destroy(&read_sem);
  }
};

static void* write_task(void* data) {
  CopyData& cd = *static_cast<CopyData*>(data);
  do {
    // Reset semaphore before checking for emptiness
    if (! cd.reader.isEmpty()) {
      // Write as much as possible
      ssize_t l = cd.stream->write(cd.reader.reader(), cd.reader.readable());
      if (l < 0) {
        cd.failed = true;
      } else {
        cd.reader.readn(l);
      }
      // Allow read task to run
      sem_post(&cd.read_sem);
    } else {
      // Now either a read occurred and we won't lock, or we shall wait
      if (sem_wait(&cd.write_sem)) {
        cd.failed = true;
      }
    }
  } while (! cd.failed && (! cd.eof || ! cd.reader.isEmpty()));
  return NULL;
}

int Stream::copy(Stream* dest1, Stream* dest2) {
  errno = 0;
  if (! isOpen() || ! dest1->isOpen()
  || ((dest2 != NULL) && ! dest2->isOpen())) {
    errno = EBADF;
    return -1;
  }
  Buffer<char>  buffer(1 << 20);
  ReadData      rd;
  bool          eof    = false;
  bool          failed = false;
  CopyData*     cd1 = NULL;
  CopyData*     cd2 = NULL;
  long long     size = 0;
  pthread_t     child1;
  pthread_t     child2;
  cd1 = new CopyData(dest1, buffer, rd.read_sem, eof, failed);
  int rc = pthread_create(&child1, NULL, write_task, cd1);
  if (rc != 0) {
    delete cd1;
    errno = rc;
    return -1;
  }
  if (dest2 != NULL) {
    cd2 = new CopyData(dest2, buffer, rd.read_sem, eof, failed);
    rc = pthread_create(&child2, NULL, write_task, cd2);
    if (rc != 0) {
      delete cd2;
      errno = rc;
      // Tell other task to die and wait for it
      failed = true;
      pthread_join(child1, NULL);
      delete cd1;
      return -1;
    }
  }

  // Read loop
  do {
    // Reset semaphore before checking for fullness
    if (! buffer.isFull()) {
      // Fill up only up to half of capacity, for simultenous read and write
      ssize_t length = buffer.capacity() >> 1;
      if (static_cast<size_t>(length) > buffer.writeable()) {
        length = buffer.writeable();
      }
      length = read(buffer.writer(), length);
      if (length < 0) {
        failed = true;
      } else {
        // Update writer before signaling eof
        buffer.written(length);
        // Allow write task to run
        if (length == 0) {
          eof = true;
        }
        size += length;
      }
      sem_post(&cd1->write_sem);
      if (dest2 != NULL) {
        sem_post(&cd2->write_sem);
      }
    } else {
      // Now either a write occurred and we won't lock, or we shall wait
      if (sem_wait(&rd.read_sem)) {
        failed = true;
      }
    }
    if ((_d->cancel_callback != NULL) && ((*_d->cancel_callback)(2))) {
      errno = ECANCELED;
      failed = true;
      sem_post(&cd1->write_sem);
      if (dest2 != NULL) {
        sem_post(&cd2->write_sem);
      }
      break;
    }
  } while (! failed && ! eof);

  // Wait for write tasks to terminate, check status
  rc = pthread_join(child1, NULL);
  delete cd1;
  if (rc != 0) {
    errno = rc;
  } else
  // Check that data sizes match
  if ((errno == 0) && (dataSize() != dest1->dataSize())) {
    errno = EAGAIN;
  }
  if (dest2 != NULL) {
    rc = pthread_join(child2, NULL);
    delete cd2;
    if (rc != 0) {
      errno = rc;
    } else
    // Check that data sizes match
    if ((errno == 0) && (dataSize() != dest2->dataSize())) {
      errno = EAGAIN;
    }
  }
  // Check that sizes match
  if (! failed && (errno == 0) && (size != dataSize())) {
    errno = EAGAIN;
  }
  return (errno != 0) ? -1 : 0;
}

int Stream::compare(Stream& source, long long length) {
  if ((! isOpen()) || (! source.isOpen())) {
    errno = EBADF;
    return -1;
  }
  int             buf_size = 1024;
  unsigned char*  buffer1  = static_cast<unsigned char*>(malloc(buf_size));
  unsigned char*  buffer2  = static_cast<unsigned char*>(malloc(buf_size));
  int             rc = -2;

  while (rc == -2) {
    bool eof;
    // Make we stop when needed
    if ((length >= 0) && (length < buf_size)) {
      buf_size = static_cast<int>(length);
    }
    // Fill in buffer1
    ssize_t size1 = 0;
    eof = false;
    while (! eof && (size1 < buf_size)) {
      ssize_t size = read(&buffer1[size1], buf_size - size1);
      if (size < 0) {
        rc = -1;
        break;
      } else if (size == 0) {
        eof = true;
      }
      size1 += size;
    }
    // Fill in buffer2
    ssize_t size2 = 0;
    if (rc == -2) {
      eof = false;
      while (! eof && (size2 < buf_size)) {
        ssize_t size = source.read(&buffer2[size2], buf_size - size2);
        if (size < 0) {
          rc = -1;
          break;
        } else if (size == 0) {
          eof = true;
        }
        size2 += size;
      }
    }
    if (rc == -2) {
      // Compare size
      if (size1 != size2) {
        // Differ in size
        rc = 1;
      } else
      // Check end of files
      if (size1 == 0) {
        rc = 0;
      } else
      // Compare buffers
      if (memcmp(buffer1, buffer2, size1) != 0) {
        // Differ in contents
        rc = 1;
      } else
      // Check buffer fill
      if (size1 < buf_size) {
        rc = 0;
      } else
      // How much is left to compare?
      if (length >= 0) {
        length -= size1;
        if (length <= 0) {
          rc = 0;
        }
      }
    }
    if ((_d->cancel_callback != NULL) && ((*_d->cancel_callback)(3))) {
      errno = ECANCELED;
      return -1;
    }
  }
  free(buffer1);
  free(buffer2);
  return rc;
}

long long Stream::dataSize() const {
  return _d->size;
}
