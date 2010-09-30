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

// Compression to use when required: gzip -5 (best speed/ratio)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>

#include <string>

using namespace std;

#include "hreport.h"
#include "filereaderwriter.h"
#include "unzipreader.h"
#include "zipwriter.h"
#include "hasher.h"
#include "asyncwriter.h"
#include "multiwriter.h"
#include "hbackup.h"
#include "files.h"
#include "compdata.h"
#include "data.h"

using namespace hbackup;
using namespace htools;

struct Data::Private {
  string            path;
  string            data;
  string            data_gz;
  progress_f        progress;
  Private() : progress(NULL) {}
};

int Data::findExtension(
    char*           path,
    const char*     extensions[]) {
  size_t original_length = strlen(path);
  int i = 0;
  while (extensions[i] != NULL) {
    strcpy(&path[original_length], extensions[i]);
    if (isReadable(path)) {
      break;
    }
    ++i;
  }
  if (extensions[i] == NULL) {
    path[original_length] = '\0';
    return -1;
  }
  return i;
}

bool Data::isReadable(
    const char*     path) {
  FILE* fd = fopen(path, "r");
  if (fd != NULL) {
    fclose(fd);
    return true;
  }
  return false;
}

int Data::touch(
    const char*     path) {
  FILE* fd = fopen(path, "w");
  if (fd != NULL) {
    fclose(fd);
    return 0;
  }
  return -1;
}

ssize_t Data::compare(
    IReaderWriter&  left,
    IReaderWriter&  right) const {
  ssize_t rc;
  if (left.open() >= 0) {
    if (right.open() >= 0) {
      char buffer_left[102400];
      char buffer_right[102400];
      size_t data_size = 0;
      bool failed = false;
      bool differ = false;
      do {
        ssize_t length_left = left.read(buffer_left, sizeof(buffer_left));
        if (length_left < 0) {
          failed = true;
          break;
        }
        ssize_t length_right = right.read(buffer_right, sizeof(buffer_right));
        if (length_right < 0) {
          failed = true;
          break;
        }
        if (length_left != length_right) {
          differ = true;
          break;
        }
        if (length_left == 0) {
          break;
        }
        if (memcmp(buffer_left, buffer_right, length_left) != 0) {
          differ = true;
          break;
        }
        data_size += length_left;
        if (aborting(3)) {
          errno = ECANCELED;
          failed = true;
          break;
        }
      } while (true);
      if (failed) {
        rc = -1;
      } else {
        rc = differ ? 1 : 0;
      }
      if (right.close() < 0) {
        rc = -1;
      }
    } else {
      rc = -1;
    }
    if (left.close() < 0) {
      rc = -1;
    }
  } else {
    rc = -1;
  }
  return rc;
}

long long Data::copy(
    IReaderWriter*    source,
    FileReaderWriter* source_fr,
    IReaderWriter*    dest1,
    IReaderWriter*    dest2) const {
  // Make writers asynchronous
  dest1 = new AsyncWriter(dest1, false);
  MultiWriter w(dest1, false);
  if (dest2 != NULL) {
    dest2 = new AsyncWriter(dest2, false);
    w.add(dest2, false);
  }
  if (w.open() < 0) {
    delete dest1;
    if (dest2 != NULL) {
      delete dest2;
    }
    return -1;
  }
  // Copy loop
  enum { BUFFER_SIZE = 1 << 19 }; // Total buffered size = 1 MiB
  char buffer1[BUFFER_SIZE];      // odd buffer
  char buffer2[BUFFER_SIZE];      // even buffer
  char* buffer = buffer1;         // currently unused buffer
  ssize_t size;                   // Size actually read at begining of loop
  long long data_size = 0;        // Source file size
  // Progress
  long long progress_total = -1;
  long long progress_last;
  if ((_d->progress != NULL) && (source_fr != NULL)) {
    struct stat64 metadata;
    if (lstat64(source_fr->path(), &metadata) >= 0) {
      progress_total = metadata.st_size;
    }
    progress_last = 0;
  }

  bool failed = false;
  do {
    // size will be BUFFER_SIZE unless the end of file has been reached
    size = source->read(buffer, BUFFER_SIZE);
    if (size <= 0) {
      if (size < 0) {
        failed = true;
      }
      break;
    }
    if (w.write(buffer, size) < 0) {
      failed = true;
      break;
    }
    data_size += size;
    // Swap unused buffers
    if (buffer == buffer1) {
      buffer = buffer2;
    } else {
      buffer = buffer1;
    }
    // Update big brother
    if (progress_total >= 0) {
      long long progress_new = source_fr->offset();
      if (progress_new != progress_last) {
        (*_d->progress)(progress_last, progress_new, progress_total);
        progress_last = progress_new;
      }
    }
    // Check that we are not told to abort
    if (aborting(2)) {
      errno = ECANCELED;
      failed = true;
      break;
    }
  } while (size == BUFFER_SIZE);
  // close and update metadata and 'data size' (the unzipped size)
  if (w.close() < 0) {
    failed = true;
  }
  delete dest1;
  if (dest2 != NULL) {
    delete dest2;
  }
  return failed ? -1 : data_size;
}

int Data::getDir(
    const char*     checksum,
    char*           path,
    bool            create) const {
  size_t path_len = sprintf(path, "%s", _d->path.c_str());
  // Two cases: either there are files, or a .nofiles file and directories
  int level = 0;
  do {
    // If we can find a .nofiles file, then go down one more directory
    strcpy(&path[path_len++], "/.nofiles");
    if (isReadable(path)) {
      path[path_len++] = checksum[level++];
      path[path_len++] = checksum[level++];
      path[path_len] = '\0';
      if (create && (Directory(path).create() < 0)) {
        return -1;
      }
    } else {
      break;
    }
  } while (true);
  // Return path
  path[path_len] = '/';
  strcpy(&path[path_len], &checksum[level]);
  path = path;
  return Directory(path).isValid() ? 0 : 1;
}

int Data::getMetadata(
    const char*     path,
    long long*      size_p,
    CompressionCase* comp_status_p) const {
  char meta_path[PATH_MAX];
  sprintf(meta_path, "%s/%s", path, "meta");
  FILE* fd = fopen(meta_path, "r");
  if (fd == NULL) {
    hlog_error("%s opening metadata file '%s'", strerror(errno), meta_path);
    return -1;
  }
  char line[64] = "";
  ssize_t size = fread(line, 64, 1, fd);
  if (size < 0) {
    hlog_error("%s reading metadata file '%s'", strerror(errno), meta_path);
    return -1;
  }
  int rc = sscanf(line, "%lld\t%c", size_p,
    reinterpret_cast<char*>(comp_status_p));
  switch (rc) {
    case 0:
      *size_p = -1;
    case 1:
      *comp_status_p = unknown;
    case 2:
      break;
    default:
      return -1;
  }
  return fclose(fd);
}

int Data::setMetadata(
    const char*     path,
    long long       size,
    CompressionCase comp_status) const {
  char meta_path[PATH_MAX];
  sprintf(meta_path, "%s/%s", path, "meta");
  FILE* fd = fopen(meta_path, "w");
  if (fd == NULL) {
    hlog_error("%s creating metadata file '%s'", strerror(errno), meta_path);
    return -1;
  }
  int rc = fprintf(fd, "%lld\t%c", size, comp_status);
  if (rc < 0) {
    hlog_error("%s writing metadata file '%s'", strerror(errno), meta_path);
    return -1;
  }
  return fclose(fd);
}

int Data::removePath(const char* path) const {
  const char* extensions[] = { "", ".gz", NULL };
  char local_path[PATH_MAX];
  sprintf(local_path, "%s/%s", path, "data");
  int no = findExtension(local_path, extensions);
  int rc = 0;
  if (no >= 0) {
    rc = ::remove(local_path);
  }
  int errno_keep = errno;
  sprintf(local_path, "%s/%s", path, "corrupted");
  ::remove(local_path);
  sprintf(local_path, "%s/%s", path, "meta");
  ::remove(local_path);
  ::remove(path);
  errno = errno_keep;
  return rc;
}

int Data::organise(
    const char*     path,
    int             number) const {
  DIR           *directory;
  struct dirent *dir_entry;
  bool          failed = false;

  // Already organised?
  char nofiles_path[PATH_MAX];
  sprintf(nofiles_path, "%s/%s", path, ".nofiles");
  if (isReadable(nofiles_path)) {
    return 0;
  }
  // Find out how many entries
  if ((directory = opendir(path)) == NULL) {
    return 1;
  }
  // Skip . and ..
  number += 2;
  while (((dir_entry = readdir(directory)) != NULL) && (number > 0)) {
    number--;
  }
  // Decide what to do
  if (number == 0) {
    rewinddir(directory);
    while ((dir_entry = readdir(directory)) != NULL) {
      // Ignore anything in .*
      if (dir_entry->d_name[0] == '.') {
        continue;
      }
      Node source_path(Path(path, dir_entry->d_name));
      if (source_path.stat()) {
        hlog_error("%s stating source file '%s'", strerror(errno),
          source_path.path());
        failed = true;
      } else
      if ((source_path.type() == 'd')
       // If we crashed, we might have some two-letter dirs already
       && (strlen(source_path.name()) != 2)
       // If we've reached the point where the dir is ??-?, stop!
       && (source_path.name()[2] != '-')) {
        // Create two-letter directory
        char two_letters[3] = {
          source_path.name()[0], source_path.name()[1], '\0'
        };
        Directory dir(Path(path, two_letters));
        if (dir.create() < 0) {
          failed = true;
        } else {
          // Move directory accross, changing its name
          if (rename(source_path.path(),
              Path(dir.path(), &source_path.name()[2]).c_str())) {
            failed = true;
          }
        }
      }
    }
    if (! failed) {
      touch(nofiles_path);
    }
  }
  closedir(directory);
  return failed ? -1 : 0;
}

int Data::crawl_recurse(
    Directory&      dir,
    const string&   checksum_part,
    list<CompData>* data,
    bool            thorough,
    bool            repair,
    size_t*         valid,
    size_t*         broken) const {
  bool failed = false;
  if (dir.isValid() && ! dir.createList()) {
    bool no_files   = false;
    bool temp_found = false;
    list<Node*>::iterator i = dir.nodesList().begin();
    while (i != dir.nodesList().end()) {
      if (failed) {
        // Skip any processing, but remove node from list
      } else
      // Will come first in Path order
      if ((*i)->type() == 'f') {
        // '.' files are before, so .nofiles will be found first
        if (strcmp((*i)->name(), ".nofiles") == 0) {
          no_files = true;
        }
      } else
      if ((*i)->type() == 'd') {
        // '.' files are before, so .temp will be found first
        if (! temp_found && (strcmp((*i)->name(), ".temp") == 0)) {
          temp_found = true;
        } else {
          string checksum = checksum_part + (*i)->name();
          if (no_files) {
            Directory d(**i);
            if (crawl_recurse(d, checksum, data, thorough, repair, valid,
                broken) < 0){
              failed = true;
            }
          } else {
            long long size;
            bool      comp;
            if (! check(checksum.c_str(), thorough, repair, &size, &comp)) {
              // Add to data, the list of collected data
              if (data != NULL) {
                data->push_back(CompData(checksum.c_str(), size, comp));
              }
              if (valid != NULL) {
                (*valid)++;
              }
            } else {
              if (broken != NULL) {
                (*broken)++;
              }
            }
          }
        }
      }
      if (aborting()) {
        failed = true;
      }
      delete *i;
      i = dir.nodesList().erase(i);
    }
  } else {
    failed = true;
  }
  return failed ? -1 : 0;
}

Data::Data(const char* path) : _d(new Private) {
  _d->path = path;
  _d->data = _d->path + "/.data";
  _d->data_gz = _d->data + ".gz";
}

Data::~Data() {
  delete _d;
}

int Data::open(bool create) {
  // Base directory
  Directory dir(_d->path.c_str());
  // Check for existence
  if (dir.isValid()) {
    return 0;
  }
  if (create && (dir.create() >= 0)) {
    // Signal creation
    return 1;
  }
  return -1;
}

void Data::setProgressCallback(progress_f progress) {
  _d->progress = progress;
}

int Data::name(
    const char*     checksum,
    string&         path,
    string&         extension) const {
  char temp_path[PATH_MAX];
  if (getDir(checksum, temp_path)) {
    return -1;
  }
  path = temp_path;
  const char* extensions[] = { "", ".gz", NULL };
  char local_path[PATH_MAX];
  sprintf(local_path, "%s/%s", path.c_str(), "data");
  int no = findExtension(local_path, extensions);
  if (no < 0) {
    errno = ENOENT;
    hlog_error("%s looking for file '%s'*", strerror(errno), local_path);
    return -1;
  }
  path      = local_path;
  extension = extensions[no];
  return 0;
}

int Data::read(
    const char*     path,
    const char*     checksum) const {
  bool failed = false;

  char source[PATH_MAX];
  if (getDir(checksum, source)) {
    hlog_error("Cannot access DB data for %s", checksum);
    return -1;
  }

  // Open source
  const char* extensions[] = { "", ".gz", NULL };
  char local_path[PATH_MAX];
  sprintf(local_path, "%s/%s", source, "data");
  int no = findExtension(local_path, extensions);
  if (no < 0) {
    errno = ENOENT;
    hlog_error("%s looking for source file '%s'*", strerror(errno),
      local_path);
    return -1;
  }
  FileReaderWriter* data_fr = new FileReaderWriter(local_path, false);
  IReaderWriter* hh = data_fr;
  if (no > 0) {
    hh = new UnzipReader(hh, true);
  }
  char hash[129];
  Hasher data(hh, true, Hasher::md5, hash);
  if (data.open()) {
    hlog_error("%s opening source file '%s'", strerror(errno), local_path);
    return -1;
  }
  string temp_path = path;
  temp_path += ".hbackup-part";
  FileReaderWriter temp(temp_path.c_str(), true);
  // Copy file to temporary name (size not checked: checksum suffices)
  if (copy(&data, data_fr, &temp) < 0) {
    hlog_error("%s reading source file '%s'", strerror(errno), local_path);
    failed = true;
  }
  data.close();
  if (! failed) {
    // Verify that checksums match before overwriting final destination
    if (strncmp(checksum, hash, strlen(hash)) != 0) {
      hlog_error("Read checksums don't match");
      failed = true;
    } else

    // All done
    if (rename(temp_path.c_str(), path)) {
      hlog_error("%s renaming read file '%s'", strerror(errno), path);
      failed = true;
    }
  }
  if (failed) {
    ::remove(temp_path.c_str());
  }
  return failed ? -1 : 0;
}

Data::WriteStatus Data::write(
    const char*     path,
    char            dchecksum[64],
    int*            comp_level,
    bool            comp_auto) const {
  bool failed = false;

  // Open source file
  FileReaderWriter source_fr(path, false);
  char source_hash[129];
  Hasher source(&source_fr, false, Hasher::md5, source_hash);
  if (source.open() < 0) {
    return error;
  }

  // Temporary file(s) to write to
  FileReaderWriter* temp2 = NULL;
  CompressionCase comp_case;
  // compress < 0 => required to not compress by filter or configuration
  if (*comp_level < 0) {
    comp_case = forced_no;
    comp_auto = false;
    *comp_level = 0;
  } else
  // compress = 0 => do not compress
  if (*comp_level == 0) {
    comp_case = later;
    comp_auto = false;
  } else
  // compress > 0 && comp_auto => decide whether to compress or not
  if (comp_auto) {
    comp_case = later;
    // Automatic compression: copy twice, compressed and not, and choose best
    temp2 = new FileReaderWriter(_d->data_gz.c_str(), true);
  } else {
  // compress > 0 && ! comp_auto => compress
    comp_case = forced_yes;
  }
  FileReaderWriter* temp1_fw = new FileReaderWriter(_d->data.c_str(), true);
  IReaderWriter* temp1 = temp1_fw;
  if (*comp_level > 0) {
    temp1 = new ZipWriter(temp1_fw, true, *comp_level);
  } else {
    temp1 = temp1_fw;
  }

  long long source_data_size;
  if (! failed) {
    // Copy file locally
    source_data_size = copy(&source, &source_fr, temp1, temp2);
    if (source_data_size < 0) {
      failed = true;
    }
  }
  source.close();

  if (failed) {
    ::remove(_d->data.c_str());
    delete temp1;
    if (temp2 != NULL) {
      ::remove(_d->data_gz.c_str());
      delete temp2;
    }
    return error;
  }

  // Size for comparison with existing data
  long long size_cmp = temp1_fw->offset();
  // Name of file
  const char* dest_name = _d->data.c_str();
  // If temp2 is not NULL then comp_auto is true
  if (temp2 != NULL) {
    // Add ~1.6% to gzip'd size
    long long size_gz = temp1_fw->offset() + (temp1_fw->offset() >> 6);
    hlog_debug("Checking data, sizes: f=%lld z=%lld (%lld)",
      temp2->offset(), temp1_fw->offset(), size_gz);
    // Keep non-compressed file (temp2)?
    if (temp2->offset() <= size_gz) {
      ::remove(_d->data.c_str());
      dest_name = _d->data_gz.c_str();
      size_cmp = size_gz;
      comp_case = size_no;
      *comp_level = 0;
    } else {
      ::remove(_d->data_gz.c_str());
      comp_case = size_yes;
    }
    delete temp2;
  }
  delete temp1;

  // File to add to DB
  FileReaderWriter* dfr = new FileReaderWriter(dest_name, false);
  IReaderWriter* hh = dfr;
  if (*comp_level > 0) {
    hh = new UnzipReader(hh, true);
  }
  char hash[129];
  Hasher dest(hh, true, Hasher::md5, hash);

  // Get file final location
  char dest_path[PATH_MAX];
  if (getDir(source_hash, dest_path, true) < 0) {
    hlog_error("Cannot create DB data '%s'", source_hash);
    return error;
  }

  // Make sure our checksum is unique: compare first bytes of files
  int index = 0;
  char final_path[PATH_MAX];
  char data_path[PATH_MAX];
  WriteStatus status = error;
  do {
    sprintf(final_path, "%s-%u", dest_path, index);
    // Directory does not exist
    if (! Directory(final_path).isValid()) {
      status = add;
    } else {
      const char* extensions[] = { "", ".gz", NULL };
      sprintf(data_path, "%s/%s", final_path, "data");
      int no = findExtension(data_path, extensions);
      // File does not exist
      if (no < 0) {
        status = add;
      } else
      // Compare files
      {
        FileReaderWriter* fr = new FileReaderWriter(data_path, false);
        IReaderWriter* hh = fr;
        if (no > 0) {
          hh = new UnzipReader(hh, true);
        }
        char hash[129];
        Hasher data(hh, true, Hasher::md5, hash);
        int cmp = compare(dest, data);
        switch (cmp) {
          // Same data
          case 0:
            // Empty file is compressed
            if ((fr->offset() == 0) && (no > 0)) {
              status = replace;
            } else
            // Replacing data will save space
            if (size_cmp < fr->offset()) {
              status = replace;
            } else
            // Nothing to do
            {
              status = leave;
            }
            break;
          // Different data
          case 1:
            index++;
            break;
          // Error
          default:
            hlog_error("%s comparing data '%s'", strerror(errno), source_hash);
            status = error;
            failed = true;
        }
      }
    }
  } while (! failed && (status == error));

  // Now move the file in its place
  if ((status == add) || (status == replace)) {
    hlog_debug("%s %scompressed data for %s-%d",
      (status == add) ? "Adding" : "Replacing with",
      (*comp_level != 0) ? "" : "un", source_hash, index);
    if (Directory(final_path).create() < 0) {
      hlog_error("%s creating directory '%s'", strerror(errno), final_path);
    } else
    if ((status == replace) && (::remove(data_path) < 0)) {
      hlog_error("%s removing previous data '%s'", strerror(errno), data_path);
    } else {
      char name[PATH_MAX];
      sprintf(name, "%s/data%s", final_path, (*comp_level != 0) ? ".gz" : "");
      if (rename(dest_name, name)) {
        hlog_error("%s moving file '%s'", strerror(errno), name);
        status = error;
      } else {
        // Always add metadata (size) file (no error status on failure)
        setMetadata(final_path, source_data_size, comp_case);
      }
    }
  } else
  /* Make sure we have metadata information */
  if (comp_case == forced_no) {
    long long size;
    CompressionCase comp_status;
    if (! getMetadata(final_path, &size, &comp_status) &&
        (comp_status != forced_no)) {
      setMetadata(final_path, size, forced_no);
    }
  }

  // If anything failed, or nothing happened, delete temporary file
  if ((status == error) || (status == leave)) {
    ::remove(dest_name);
  }

  // Report checksum
  if (status != error) {
    sprintf(dchecksum, "%s-%d", source_hash, index);
    // Make sure we won't exceed the file number limit
    if (status != leave) {
      // dest_path is /path/to/checksum
      char* pos = strrchr(dest_path, '/');

      if (pos != NULL) {
        *pos = '\0';
        // Now dest_path is /path/to
        organise(dest_path, 256);
      }
    }
  }
  return status;
}

int Data::check(
    const char*     checksum,
    bool            thorough,
    bool            repair,
    long long*      size_p,
    bool*           compressed_p) const {
  char path[PATH_MAX];
  // Get dir from checksum
  if (getDir(checksum, path)) {
    return -1;
  }
  // Look for data in dir
  bool failed  = false;
  bool display = true;  // To not print twice when calling check


  const char* extensions[] = { "", ".gz", NULL };
  char local_path[PATH_MAX];
  sprintf(local_path, "%s/%s", path, "data");
  int no = findExtension(local_path, extensions);
  // Missing data
  if (no < 0) {
    hlog_error("Data missing for %s", checksum);
    if (repair) {
      removePath(path);
    }
    return -1;
  }
  // Get original file size
  long long data_size = -1;
  CompressionCase comp_status = unknown;
  getMetadata(path, &data_size, &comp_status);
  // Get file size
  long long real_size = -1;
  {
    struct stat64 metadata;
    if (lstat64(local_path, &metadata) >= 0) {
      real_size = metadata.st_size;
    }
  }

  char corrupted_path[PATH_MAX];
  sprintf(corrupted_path, "%s/%s", path, "corrupted");
  // Check data for corruption
  if (thorough) {
    // Already marked corrupted?
    if (isReadable(corrupted_path)) {
      hlog_warning("Data corruption reported for %s", checksum);
      data_size = -1;
      failed = true;
    } else {
      FileReaderWriter* fr = new FileReaderWriter(local_path, false);
      IReaderWriter* hh = fr;
      if (no > 0) {
        hh = new UnzipReader(hh, true);
      }
      char hash[129];
      Hasher data(hh, true, Hasher::md5, hash);
      // Open file
      if (data.open()) {
        hlog_error("%s opening file '%s'", strerror(errno), local_path);
        failed = true;
      } else {
        long long progress_total;
        long long progress_last;
        if (_d->progress != NULL) {
          progress_total = real_size;
          progress_last = 0;
        }
        // Compute file size and checksum
        long long size = 0;
        ssize_t length;
        do {
          char buffer[102400];
          length = data.read(buffer, sizeof(buffer));
          if (length < 0) {
            failed = true;
          }
          if (length <= 0) {
            break;
          }
          size += length;
          if ((_d->progress != NULL) && (progress_total >= 0)) {
            long long progress_new = fr->offset();
            if (progress_new != progress_last) {
              (*_d->progress)(progress_last, progress_new, progress_total);
              progress_last = progress_new;
            }
          }
          if (aborting(1)) {
            errno = ECANCELED;
            failed = true;
          }
        } while (length > 0);
        // Close file
        if (data.close()) {
          hlog_error("%s closing file '%s'", strerror(errno), local_path);
          failed = true;
        }
        if (failed) {
          hlog_error("%s reading file '%s'", strerror(errno), local_path);
        } else
        // Compare with given checksum
        if (strncmp(hash, checksum, strlen(hash))) {
          hlog_error("Data corrupted for %s%s", checksum, repair ? ", remove" : "");
          hlog_debug("Checksum: %s", hash);
          failed = true;
          if (repair) {
            removePath(path);
          } else {
            // Mark corrupted
            touch(corrupted_path);
          }
          data_size = -1;
        } else
        // Compare data size and stored size for compress files
        if (size != data_size) {
          if (data_size >= 0) {
            hlog_error("Correcting wrong metadata for %s", checksum);
          } else {
            hlog_warning("Adding missing metadata for %s", checksum);
          }
          data_size = size;
          setMetadata(path, data_size, later);
        }
      }
    }
  } else
  if (repair) {
    // Remove data marked corrupted
    if (isReadable(corrupted_path)) {
      if (removePath(path) == 0) {
        hlog_info("Removed corrupted data for %s", checksum);
      } else {
        hlog_error("%s removing data '%s'", strerror(errno), checksum);
      }
      failed = true;
    } else
    // Compute size if missing
    if (data_size < 0) {
      if (no == 0) {
        if (real_size >= 0) {
          hlog_warning("Setting missing metadata for %s", checksum);
          data_size = real_size;
          setMetadata(path, data_size, later);
        }
      } else
      // Compressed file, check it thoroughly, which shall add the metadata
      {
        if (check(checksum, true, true, size_p, compressed_p)) {
          failed = true;
        }
        getMetadata(path, &data_size, &comp_status);
        display = false;
      }
    }
  } else
  if ((data_size < 0) && (errno == ENOENT)) {
    hlog_error("Metadata missing for %s", checksum);
  }

  if (display) {
    if (no == 0) {
      hlog_verbose_temp("%s %lld", checksum, data_size);
    } else {
      hlog_verbose_temp("%s %lld %lld", checksum, data_size, real_size);
    }
  }

  // Return file information if required
  if (size_p != NULL) {
    *size_p = data_size;
  }
  if (compressed_p != NULL) {
    *compressed_p = no > 0;
  }
  return failed ? -1 : 0;
}

int Data::remove(
    const char*     checksum) const {
  char path[PATH_MAX];
  if (getDir(checksum, path)) {
    // Warn about directory missing
    return 1;
  }
  return removePath(path);
}

int Data::crawl(
    bool            thorough,
    bool            repair,
    list<CompData>* data) const {
  Directory d(_d->path.c_str());
  size_t valid  = 0;
  size_t broken = 0;
  int rc = crawl_recurse(d, "", data, thorough, repair, &valid, &broken);
  hlog_verbose("Found %zu valid and %zu broken data file(s)", valid, broken);
  return rc;
}
