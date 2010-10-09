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

#include <string>

using namespace std;

#include "stdio.h"
#include "stdlib.h"
#include "limits.h"
#include "stdarg.h"
#include "string.h"
#include "errno.h"
#include "pthread.h"

#include "sys/stat.h"

#include "hreport.h"

#include <files.h>
#include <filereaderwriter.h>
#include <zipwriter.h>
#include <asyncwriter.h>

using namespace htools;

Report htools::report;

struct Report::Private {
  pthread_mutex_t mutex;
  size_t          size_to_overwrite;
  bool            console_log;
  bool            file_log;
  string          file_name;
  size_t          max_lines;
  size_t          max_files;
  FILE*           fd;
  size_t          lines;
  Private() : size_to_overwrite(0), console_log(true), file_log(false),
      max_lines(0), max_files(0), fd(NULL) {
    pthread_mutex_init(&mutex, NULL);
  }
  int lock() {
    return pthread_mutex_lock(&mutex);
  }
  int unlock() {
    return pthread_mutex_unlock(&mutex);
  }
  int checkRotate(bool init = false) {
    bool need_open = false;
    // check file still exists and size
    struct stat stat_buf;
    if (stat(file_name.c_str(), &stat_buf) < 0) {
      if (file_log) {
        fclose(fd);
      }
      need_open = true;
    } else
    if (init) {
      if (stat_buf.st_size != 0) {
        rotate();
      }
      need_open = true;
    } else
    if ((max_lines != 0) && (lines >= max_lines)) {
      fclose(fd);
      need_open = true;
      rotate();
    }
    // re-open file
    if (need_open) {
      fd = fopen(file_name.c_str(), "w");
      lines = 0;
    }
    return (fd == NULL) ? -1 : 0;
  }
  int zip(char* name, size_t length) {
    FileReaderWriter fr(name, false);
    sprintf(&name[length], ".gz");
    FileReaderWriter fw(name, true);
    ZipWriter zw(&fw, false, 5);
    AsyncWriter aw(&zw, false);
    bool failed = false;
    if (fr.open() < 0) {
      failed = true;
    } else {
      if (aw.open() < 0) {
        failed = true;
      } else {
        // Copy
        enum { BUFFER_SIZE = 102400 };  // Too big and we end up wasting time
        char buffer1[BUFFER_SIZE];      // odd buffer
        char buffer2[BUFFER_SIZE];      // even buffer
        char* buffer = buffer1;         // currently unused buffer
        ssize_t size;                   // Size actually read at loop begining
        do {
          // size will be BUFFER_SIZE unless the end of file has been reached
          size = fr.read(buffer, BUFFER_SIZE);
          if (size <= 0) {
            if (size < 0) {
              failed = true;
            }
            break;
          }
          if (aw.write(buffer, size) < 0) {
            failed = true;
            break;
          }
          // Swap unused buffers
          if (buffer == buffer1) {
            buffer = buffer2;
          } else {
            buffer = buffer1;
          }
        } while (size == BUFFER_SIZE);
        if (aw.close() < 0) {
          failed = true;
        }
      }
      if (fr.close() < 0) {
        failed = true;
      }
    }
    return failed ? -1 : 0;
  }
  int rotate() {
    int rc = 0;
    /* Max size for: file_name-N.gz, where N can be up to 10 digits (32 bits) */
    size_t len = file_name.length();
    char name[PATH_MAX];
    char new_name[PATH_MAX];
    strcpy(name, file_name.c_str());
    strcpy(new_name, file_name.c_str());
    /* Loop over potential files */
    size_t i = max_files;
    do {
      // Complete name
      int num_chars;
      if (i != 0) {
        num_chars = sprintf(&name[len], "-%zu", i);
      } else {
        name[len] = '\0';
        num_chars = 0;
      }
      const char* extensions[] = { "", ".gz", NULL };
      int no = Node::findExtension(name, extensions, len + num_chars);
      /* File found */
      if (no >= 0) {
        if (i == max_files) {
          remove(name);
        } else {
          num_chars = sprintf(&new_name[len], "-%zu", i + 1);
          if (no > 0) {
            sprintf(&new_name[len + num_chars], ".gz");
          }
          if (rename(name, new_name)) {
            rc = -1;
            break;
          }
          if (no == 0) {
            /* zip the new file */
            zip(new_name, len + num_chars);
          }
        }
      }
    } while (i-- != 0);
    return rc;
  }
  int consoleLog(
      FILE*           fd,
      const char*     file,
      int             line,
      Level           level,
      bool            temporary,
      size_t          ident,
      const char*     format,
      va_list*        args) {
    (void) file;
    (void) line;
    char buffer[1024];
    size_t offset = 0;
    // prefix
    switch (level) {
      case alert:
        offset += sprintf(&buffer[offset], "ALERT! ");
        break;
      case error:
        offset += sprintf(&buffer[offset], "Error: ");
        break;
      case warning:
        offset += sprintf(&buffer[offset], "Warning: ");
        break;
      case verbose:
      case debug:
        // add arrow
        if (ident > 0) {
          /* " --n--> " */
          buffer[offset++] = ' ';
          for (size_t i = 0; i < ident; ++i) {
            buffer[offset++] = '-';
          }
          buffer[offset++] = '>';
          buffer[offset++] = ' ';
        }
        break;
      case regression:
        break;
      default:
        ;
    }
    // message
    offset += vsnprintf(&buffer[offset], sizeof(buffer) - offset, format, *args);
    size_t buffer_size = sizeof(buffer) - 1;
    /* offset is what _would_ have been written, had there been enough space */
    if (offset >= buffer_size) {
      const char ending[] = "... [truncated]";
      const size_t ending_length = sizeof(ending) - 1;
      strcpy(&buffer[buffer_size - ending_length], ending);
      offset = buffer_size;
    }
    buffer[buffer_size] = '\0';
    // compute UTF-8 string length
    size_t size = 0;
    if (temporary || (size_to_overwrite != 0)) {
      size = utf8_len(buffer);
    }
    // if previous length stored, overwrite end of previous line
    if ((size_to_overwrite != 0) && (size_to_overwrite > size)) {
      size_t diff = size_to_overwrite - size;
      if (diff > (buffer_size - offset)) {
        diff = buffer_size - offset;
      }
      memset(&buffer[offset], ' ', diff);
      offset += diff;
    }
    // print
    fwrite(buffer, offset, 1, fd);
    // end
    if (temporary) {
      fprintf(fd, "\r");
    } else {
      fprintf(fd, "\n");
    }
    fflush(fd);
    // if temp, store length (should be UTF-8 length...)
    if (temporary) {
      size_to_overwrite = size;
    } else {
      size_to_overwrite = 0;
    }
    return static_cast<int>(offset);
  }
  int fileLog(
      FILE*           fd,
      const char*     file,
      int             line,
      Level           level,
      bool            temporary,
      size_t          ident,
      const char*     format,
      va_list*        args) {
    (void) ident;
    if (temporary) {
      return 0;
    }
    if (checkRotate() < 0) {
      return -1;
    }
    int rc = 0;
    // time
    time_t epoch = time(NULL);
    struct tm date;
    localtime_r(&epoch, &date); // voluntarily ignore error case
    rc += fprintf(fd, "%04d-%02d-%02d %02d:%02d:%02d ", date.tm_year + 1900,
      date.tm_mon + 1, date.tm_mday, date.tm_hour, date.tm_min, date.tm_sec);
    // level
    switch (level) {
      case alert:
        rc += fprintf(fd, "ALERT ");
        break;
      case error:
        rc += fprintf(fd, "ERROR ");
        break;
      case warning:
        rc += fprintf(fd, "WARN  ");
        break;
      case info:
        rc += fprintf(fd, "INFO  ");
        break;
      case verbose:
        rc += fprintf(fd, "VERB  ");
        break;
      case debug:
        rc += fprintf(fd, "DEBUG ");
        break;
      case regression:
        rc += fprintf(fd, "REGR  ");
        break;
    }
    // location
    rc += fprintf(fd, "%s:%d ", file, line);
    // message
    rc += vfprintf(fd, format, *args);
    // end
    rc += fprintf(fd, "\n");
    fflush(fd);
    ++lines;
    return rc;
  }
};

size_t Report::utf8_len(const char* s) {
  size_t size = 0;
  while (*s) {
    if ((*s & 0xc0) != 0x80) {
      ++size;
    }
    ++s;
  }
  return size;
}

Report::Report() : _console_level(info), _file_level(info), _d(new Private) {}

Report::~Report() {
  stopFileLog();
  delete _d;
}

const char* Report::levelString(Level level) {
  switch (level) {
    case alert:
      return "alert";
    case error:
      return "error";
    case warning:
      return "warning";
    case info:
      return "info";
    case verbose:
      return "verbose";
    case debug:
      return "debug";
    case regression:
      return "regression";
  }
  return "unknown";
}

int Report::stringToLevel(const char* str, Level* level) {
  switch (str[0]) {
    case 'a':
    case 'A':
      *level = alert;
      return 0;
    case 'e':
    case 'E':
      *level = error;
      return 0;
    case 'w':
    case 'W':
      *level = warning;
      return 0;
    case 'i':
    case 'I':
      *level = info;
      return 0;
    case 'v':
    case 'V':
      *level = verbose;
      return 0;
    case 'd':
    case 'D':
      *level = debug;
      return 0;
    default:
      return -1;
  }
}

void Report::startConsoleLog() {
  _d->console_log = true;
}

void Report::stopConsoleLog() {
  _d->console_log = false;
}

int Report::startFileLog(const char* name, size_t max_lines, size_t max_files) {
  stopFileLog();
  _d->file_name = name;
  _d->max_lines = max_lines;
  _d->max_files = max_files;
  _d->lines = 0;
  if (_d->checkRotate(true) < 0) {
    hlog_error("%s creating log file: '%s'", strerror(errno),
      _d->file_name.c_str());
    return -1;
  }
  _d->file_log = true;
  return 0;
}

void Report::stopFileLog() {
  if (_d->file_log) {
    _d->file_log = false;
    fclose(_d->fd);
  }
}

int Report::log(
    const char*     file,
    int             line,
    Level           level,
    bool            temp,
    size_t          ident,
    const char*     format,
    ...) {
  // print only if required
  if (level > this->level()) {
    return 0;
  }
  va_list ap;
  va_start(ap, format);
  // lock
  _d->lock();
  int rc = 0;
  if (_d->console_log && (level <= _console_level)) {
    FILE* fd = (level <= warning) ? stderr : stdout;
    rc = _d->consoleLog(fd, file, line, level, temp, ident, format, &ap);
  }
  if (_d->file_log && (level <= _file_level)) {
    rc = _d->fileLog(_d->fd, file, line, level, temp, ident, format, &ap);
  }
  // unlock
  _d->unlock();
  va_end(ap);
  return rc;
}
