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

#include "report.h"
#include "filereaderwriter.h"
#include "nullwriter.h"
#include "unzipreader.h"
#include "zipwriter.h"
#include "hasher.h"
#include "stackhelper.h"
#include "asyncwriter.h"
#include "multiwriter.h"
#include "files.h"

using namespace htools;

#include "hbackup.h"
#include "backup.h"
#include "data.h"

using namespace hbackup;

struct Data::Private {
  string            path;
  Backup&           backup;
  string            data;
  string            data_gz;
  progress_f        progress;
  Private(Backup& b) : backup(b), progress(NULL) {}
};

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
    IFileReaderWriter* source,
    IReaderWriter*     dest1,
    IReaderWriter*     dest2) const {
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
  enum { BUFFER_SIZE = 102400 };  // Too big and we end up wasting time
  char buffer1[BUFFER_SIZE];      // odd buffer
  char buffer2[BUFFER_SIZE];      // even buffer
  char* buffer = buffer1;         // currently unused buffer
  ssize_t size;                   // Size actually read at begining of loop
  long long data_size = 0;        // Source file size
  // Progress
  long long progress_total = -1;
  long long progress_last;
  if (_d->progress != NULL) {
    struct stat64 metadata;
    if (lstat64(source->path(), &metadata) >= 0) {
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
      long long progress_new = source->offset();
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
  size_t path_len = sprintf(path, "%s/", _d->path.c_str());
  const char* reader = checksum;
  char* writer = &path[path_len];
  size_t n = 0;
  do {
    if ((n == 2) || (n == 4) || (n == 6)) {
      if (create) {
        *writer = '\0';
        if (Node(path).mkdir() < 0) {
          hlog_alert("%s creating directory '%s'", strerror(errno), path);
          return -1;
        }
      }
      *writer++ = '/';
    }
    ++n;
    *writer++ = *reader;
  } while (*reader++ != '\0');
  return Node(path).isDir() ? 0 : 1;
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
  } else {
    bool failed = false;
    char line[64] = "";
    ssize_t size = fread(line, 64, 1, fd);
    if (size < 0) {
      hlog_error("%s reading metadata file '%s'", strerror(errno), meta_path);
      failed = true;
    } else {
      char bin[64];
      int rc = sscanf(line, "%lld\t%c%s", size_p,
        reinterpret_cast<char*>(comp_status_p), bin);
      switch (rc) {
        case 1:
          *comp_status_p = unknown;
        case 2:
          break;
        default:
          hlog_error("metadata broken for '%s'", meta_path);
          *size_p = -1;
          failed = true;
      }
    }
    if (fclose(fd) < 0) {
      hlog_error("%s closing metadata file '%s'", strerror(errno), meta_path);
      failed = true;
    }
    if (failed) {
      ::remove(meta_path);
    }
    return failed ? 1 : 0;
  }
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

int Data::removePath(const char* path, const char* hash) const {
  const char* extensions[] = { "", ".gz", NULL };
  char local_path[PATH_MAX];
  sprintf(local_path, "%s/%s", path, "data");
  int no = Node::findExtension(local_path, extensions);
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
  _d->backup.removeHash(hash);
  errno = errno_keep;
  return rc;
}

int Data::upgrade(
    size_t          level,
    htools::Node&   dir) const {
  if (level == 0) {
    if (Node(Path(dir.path(), ".upgraded")).isReg()) {
      return 0;
    }
    hlog_info("Upgrading database structure in %s, please wait...", dir.path());
  }
  bool failed = false;
  if (level < 3) {
    if (! dir.createList()) {
      list<Node*>::iterator i = dir.nodesList().begin();
      while (i != dir.nodesList().end()) {
        Path file(dir.path(), (*i)->name());
        if (failed) {
          // Skip any processing, but remove node from list
        } else
        if ((*i)->type() == 'f') {
          if (strcmp((*i)->name(), ".nofiles") == 0) {
            ::remove(file);
          }
        } else
        if (((*i)->type() == 'd') && ((*i)->name()[0] != '.')) {
          if (strlen((*i)->name()) > 2) {
            char in[3];
            in[0] = (*i)->name()[0];
            in[1] = (*i)->name()[1];
            in[2] = '\0';
            Path dir_in(dir.path(), in);
            if ((mkdir(dir_in, 0777)) && (errno != EEXIST)) {
              hlog_error("failed to create dir %s", dir_in.c_str());
              failed = true;
            } else {
              Path new_file(dir_in, &(*i)->name()[2]);
              if (rename(file, new_file) != 0) {
                hlog_error("failed to rename %s into %s",
                  file.c_str(), new_file.c_str());
                failed = true;
              }
              Node subdir(dir_in);
              if (upgrade(level + 1, subdir) < 0) {
                failed = true;
              }
            }
          } else {
            Node subdir(file);
            if (upgrade(level + 1, subdir) < 0) {
              failed = true;
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
      hlog_error("failed to create list for %s", dir.path());
      failed = true;
    }
  }
  if ((level == 0) && ! failed) {
    Node::touch(Path(dir.path(), ".upgraded"));
  }
  return failed ? -1 : 0;
}

int Data::crawl_recurse(
    size_t          level,
    Node&           dir,
    char*           hash,
    Collector*      collector,
    bool            thorough,
    bool            repair,
    size_t*         valid,
    size_t*         broken) const {
  bool failed = false;
  if (level < 4) {
    if (! dir.createList()) {
      list<Node*>::iterator i = dir.nodesList().begin();
      while (i != dir.nodesList().end()) {
        if (failed) {
          // Skip any processing, but remove node from list
        } else
        if (((*i)->type() == 'd') && ((*i)->name()[0] != '.')) {
          Node subdir(Path(dir.path(), (*i)->name()));
          strcpy(&hash[level * 2], (*i)->name());
          if (crawl_recurse(level + 1, subdir, hash, collector, thorough,
              repair, valid, broken) < 0) {
            failed = true;
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
  } else {
    long long data_size;
    long long file_size;
    if (check(hash, thorough, repair, &data_size, &file_size) == 0) {
      // Add to data, the list of collected data
      if (collector != NULL) {
        collector->add(hash, data_size, file_size);
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
  return failed ? -1 : 0;
}

Data::Data(const char* path, Backup& backup) : _d(new Private(backup)) {
  _d->path = path;
  _d->data = _d->path + "/.data";
  _d->data_gz = _d->data + ".gz";
}

Data::~Data() {
  delete _d;
}

int Data::open(bool create) {
  // Base directory
  Node dir(_d->path.c_str());
  // Check for existence
  if (dir.isDir()) {
    return upgrade(0, dir);
  }
  if (create && (dir.mkdir() >= 0)) {
    Node::touch(Path(dir.path(), ".upgraded"));
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
    char*           path,
    string*         extension) const {
  if (getDir(checksum, path)) {
    return -1;
  }
  strcpy(&path[strlen(path)], "/data");
  if (extension != NULL) {
    const char* extensions[] = { "", ".gz", NULL };
    int no = Node::findExtension(path, extensions);
    if (no < 0) {
      errno = ENOENT;
      hlog_error("%s looking for file '%s'*", strerror(errno), path);
      return -1;
    } else
    *extension = extensions[no];
  }
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
  int no = Node::findExtension(local_path, extensions);
  if (no < 0) {
    errno = ENOENT;
    hlog_error("%s looking for source file '%s'*", strerror(errno),
      local_path);
    return -1;
  }
  FileReaderWriter* data_fr = new FileReaderWriter(local_path, false);
  IReaderWriter* data_fd = data_fr;
  // The reader might be unzipping...
  if (no > 0) {
    data_fd = new UnzipReader(data_fd, true);
  }
  StackHelper data(data_fd, true, data_fr);
  if (data.open()) {
    hlog_error("%s opening source file '%s'", strerror(errno), local_path);
    return -1;
  }
  string temp_path = path;
  temp_path += ".hbackup-part";
  IReaderWriter* temp_fd = new FileReaderWriter(temp_path.c_str(), true);
  // ...so we compute the checksum from the writer (in its own thread)
  char hash[129];
  Hasher temp(temp_fd, true, Hasher::md5, hash);
  // Copy file to temporary name (size not checked: checksum suffices)
  if (copy(&data, &temp) < 0) {
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
    CompressionCase comp_case,
    char*           store_path) const {
  bool failed = false;

  // Open source file
  FileReaderWriter source_fr(path, false);
  char source_hash[129];
  StackHelper source(new Hasher(&source_fr, false, Hasher::md5, source_hash),
                     true, &source_fr);
  if (source.open() < 0) {
    return error;
  }

  // Temporary file(s) to write to
  FileReaderWriter* temp1_fw = NULL;
  IReaderWriter* temp1 = NULL;
  FileReaderWriter* temp2 = NULL;
  // Check compression case
  switch (comp_case) {
    case auto_now:
    case forced_yes:
      if (*comp_level == 0) {
        hlog_error("cannot compress with level = %d", *comp_level);
        errno = EINVAL;
        return error;
      }
      temp1_fw = new FileReaderWriter(_d->data_gz.c_str(), true);
      temp1 = new ZipWriter(temp1_fw, true, *comp_level);
      // Automatic compression: copy twice, compressed and not, and choose best
      if (comp_case == auto_now) {
        temp2 = new FileReaderWriter(_d->data.c_str(), true);
      }
      break;
    case forced_no:
    case auto_later:
      *comp_level = 0;
      temp1 = temp1_fw = new FileReaderWriter(_d->data.c_str(), true);
      break;
    default:
      hlog_error("wrong compression case = %c", comp_case);
      errno = EINVAL;
      return error;
  };

  long long source_data_size;
  if (! failed) {
    // Copy file locally
    source_data_size = copy(&source, temp1, temp2);
    if (source_data_size < 0) {
      failed = true;
    }
  }
  source.close();

  if (failed) {
    ::remove(temp1_fw->path());
    delete temp1;
    if (temp2 != NULL) {
      ::remove(temp2->path());
      delete temp2;
    }
    return error;
  }

  // Size for comparison with existing data
  long long size_cmp = temp1_fw->offset();
  // Name of file
  const char* dest_name = NULL;
  // If temp2 is not NULL then comp_auto is true
  if (temp2 != NULL) {
    // Add ~1.6% to gzip'd size
    long long size_gz = temp1_fw->offset() + (temp1_fw->offset() >> 6);
    hlog_debug("Checking data, sizes: f=%lld z=%lld (%lld)",
      temp2->offset(), temp1_fw->offset(), size_gz);
    // Keep non-compressed file (temp2)?
    if (temp2->offset() <= size_gz) {
      ::remove(_d->data_gz.c_str());
      dest_name = _d->data.c_str();
      size_cmp = size_gz;
      comp_case = size_no;
      *comp_level = 0;
    } else {
      ::remove(_d->data.c_str());
      dest_name = _d->data_gz.c_str();
      comp_case = size_yes;
    }
    delete temp2;
  } else {
    if (*comp_level == 0) {
      dest_name = _d->data.c_str();
    } else {
      dest_name = _d->data_gz.c_str();
    }
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
    // Node does not exist
    if (! Node(final_path).isDir()) {
      status = add;
    } else {
      const char* extensions[] = { "", ".gz", NULL };
      sprintf(data_path, "%s/%s", final_path, "data");
      int no = Node::findExtension(data_path, extensions);
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
  char name[PATH_MAX] = "";
  if ((status == add) || (status == replace)) {
    hlog_debug("%s %scompressed data for %s-%d",
      (status == add) ? "Adding" : "Replacing with",
      (*comp_level != 0) ? "" : "un", source_hash, index);
    if (Node(final_path).mkdir() < 0) {
      hlog_error("%s creating directory '%s'", strerror(errno), final_path);
      status = error;
    } else
    if ((status == replace) && (::remove(data_path) < 0)) {
      hlog_error("%s removing previous data '%s'", strerror(errno), data_path);
      status = error;
    } else {
      sprintf(name, "%s/data%s", final_path, (*comp_level != 0) ? ".gz" : "");
      if (rename(dest_name, name)) {
        hlog_error("%s moving file '%s'", strerror(errno), name);
        status = error;
      } else {
        // Always add metadata (size) file (no error status on failure)
        setMetadata(final_path, source_data_size, comp_case);
      }
      _d->backup.addHash(source_hash);
    }
  } else
  /* Make sure we have metadata information */
  if (comp_case == forced_no) {
    long long size;
    CompressionCase comp_status;
    if (! getMetadata(final_path, &size, &comp_status) &&
        (comp_status != forced_no)) {
      setMetadata(final_path, size, comp_case);
    }
  }

  // If anything failed, or nothing happened, delete temporary file
  if ((status == error) || (status == leave)) {
    ::remove(dest_name);
  }

  // Report checksum
  if (status != error) {
    sprintf(dchecksum, "%s-%d", source_hash, index);
    if (store_path != NULL) {
      if (status == leave) {
        sprintf(name, "%s/data%s", final_path, (*comp_level != 0) ? ".gz" : "");
      }
      strcpy(store_path, name);
    }
  }
  return status;
}

int Data::check(
    const char*     checksum,
    bool            thorough,
    bool            repair,
    long long*      data_size_p,
    long long*      file_size_p) const {
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
  int no = Node::findExtension(local_path, extensions);
  // Missing data
  if (no < 0) {
    hlog_error("Data missing for %s", checksum);
    if (repair) {
      removePath(path, checksum);
    }
    return -1;
  }
  // Get original file size
  long long data_size = -1;
  CompressionCase comp_status = unknown;
  getMetadata(path, &data_size, &comp_status);
  // Get file size
  long long file_size = -1;

  char corrupted_path[PATH_MAX];
  sprintf(corrupted_path, "%s/%s", path, "corrupted");
  // Check data for corruption
  if (thorough) {
    // Already marked corrupted?
    if (Node::isReadable(corrupted_path)) {
      hlog_warning("Data corruption reported for %s", checksum);
      data_size = -1;
      failed = true;
    } else {
      FileReaderWriter* data_fr = new FileReaderWriter(local_path, false);
      IReaderWriter* data_fd = data_fr;
      if (no > 0) {
        data_fd = new UnzipReader(data_fd, true);
      }
      StackHelper data(data_fd, true, data_fr);
      // Open file
      if (data.open()) {
        hlog_error("%s opening file '%s'", strerror(errno), local_path);
        failed = true;
      } else {
        // Copy to null writer and compute the hash from it (different thread)
        char hash[129];
        Hasher nh(new NullWriter, true, Hasher::md5, hash);
        long long copy_rc = copy(&data, &nh);
        file_size = data.offset();
        if (copy_rc < 0) {
          hlog_error("%s reading file '%s'", strerror(errno), local_path);
          failed = true;
          if (errno == EUCLEAN) {
            if (repair) {
              removePath(path, checksum);
              hlog_info("Removed corrupted data for %s", checksum);
            } else {
              // Mark corrupted
              Node::touch(corrupted_path);
              hlog_info("Reported corruption for %s", checksum);
            }
          }
        }
        // Close file
        if (data.close()) {
          hlog_error("%s closing file '%s'", strerror(errno), local_path);
          failed = true;
        }
        if (! failed) {
          // Compare with given checksum
          if (strncmp(hash, checksum, strlen(hash))) {
            hlog_error("Data corrupted for %s%s", checksum, repair ? ", remove" : "");
            hlog_debug("Checksum: %s", hash);
            failed = true;
            if (repair) {
              removePath(path, checksum);
              hlog_info("Removed corrupted data for %s", checksum);
            } else {
              // Mark corrupted
              Node::touch(corrupted_path);
              hlog_info("Reported corruption for %s", checksum);
            }
            data_size = -1;
          } else
          // Compare data size and stored size for compress files
          if (copy_rc != data_size) {
            if (data_size >= 0) {
              hlog_error("Correcting wrong metadata for %s", checksum);
            } else {
              hlog_warning("Adding missing metadata for %s", checksum);
            }
            data_size = copy_rc;
            setMetadata(path, data_size, unknown);
          }
        }
      }
    }
  } else
  if (repair) {
    // Remove data marked corrupted
    if (Node::isReadable(corrupted_path)) {
      if (removePath(path, checksum) == 0) {
        hlog_info("Removed corrupted data for %s", checksum);
      } else {
        hlog_error("%s removing data '%s'", strerror(errno), checksum);
      }
      failed = true;
    } else
    // Compute size if missing
    if (data_size < 0) {
      if (no == 0) {
        struct stat64 metadata;
        if (lstat64(local_path, &metadata) >= 0) {
          file_size = metadata.st_size;
          hlog_warning("Setting missing metadata for %s", checksum);
          data_size = file_size;
          setMetadata(path, data_size, unknown);
        }
      } else
      // Compressed file, check it thoroughly, which shall add the metadata
      {
        if (check(checksum, true, true, data_size_p, file_size_p) < 0) {
          failed = true;
        } else {
          getMetadata(path, &data_size, &comp_status);
        }
        display = false;
      }
    } else
    if (hlog_is_worth(verbose)) {
      struct stat64 metadata;
      if (lstat64(local_path, &metadata) >= 0) {
        file_size = metadata.st_size;
      }
    }
  } else {
    if ((data_size < 0) && (errno == ENOENT)) {
      hlog_error("Metadata missing for %s", checksum);
    }
  }

  if (! failed && display) {
    if (no == 0) {
      hlog_verbose_temp("%s %lld", checksum, data_size);
    } else {
      hlog_verbose_temp("%s %lld %lld", checksum, data_size, file_size);
    }
  }

  // Return file information if required
  if (data_size_p != NULL) {
    *data_size_p = data_size;
  }
  if (file_size_p != NULL) {
    *file_size_p = file_size;
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
  return removePath(path, checksum);
}

int Data::crawl(
    bool            thorough,
    bool            repair,
    Collector*      collector) const {
  Node d(_d->path.c_str());
  size_t valid  = 0;
  size_t broken = 0;
  char hash[64] = "";
  int rc = crawl_recurse(0, d, hash, collector, thorough, repair, &valid,
    &broken);
  hlog_verbose("Found %zu valid and %zu broken data file(s)", valid, broken);
  return rc;
}