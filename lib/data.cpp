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
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>

#include <sstream>
#include <string>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "hreport.h"
#include "compdata.h"
#include "data.h"

using namespace hbackup;
using namespace hreport;

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
  Stream data(local_path, false, false, no > 0 ? 1 : 0);
  if (data.open()) {
    hlog_error("%s opening source file '%s'", strerror(errno),
      data.path());
    return -1;
  }
  string temp_path = path;
  temp_path += ".hbackup-part";
  Stream temp(temp_path.c_str(), true);
  // Copy file to temporary name (size not checked: checksum suffices)
  data.setCancelCallback(aborting);
  data.setProgressCallback(_d->progress);
  if (data.copy(&temp)) {
    failed = true;
  }
  data.close();

  if (! failed) {
    // Verify that checksums match before overwriting final destination
    if (strncmp(checksum, temp.checksum(), strlen(temp.checksum()))) {
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
  Stream source(path, false, true);
  if (source.open() < 0) {
    return error;
  }

  // Temporary file(s) to write to
  Stream* temp2 = NULL;
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
    temp2 = new Stream(_d->data_gz.c_str(), true, false, 0);
  } else {
  // compress > 0 && ! comp_auto => compress
    comp_case = forced_yes;
  }
  Stream* temp1 = new Stream(_d->data.c_str(), true, false, *comp_level);

  if (! failed) {
    // Copy file locally
    source.setCancelCallback(aborting);
    source.setProgressCallback(_d->progress);
    if (source.copy(temp1, temp2) != 0) {
      failed = true;
    }
  }
  source.close();

  if (failed) {
    temp1->remove();
    delete temp1;
    if (temp2 != NULL) {
      temp2->remove();
      delete temp2;
    }
    return error;
  }

  // Size for comparison with existing DB data
  long long size_cmp = temp1->size();
  // If temp2 is not NULL then comp_auto is true
  if (temp2 != NULL) {
    // Add ~1.6% to gzip'd size
    long long size_gz = temp1->size() + (temp1->size() >> 6);
    hlog_debug("Checking data, sizes: f=%lld z=%lld (%lld)",
      temp2->size(), temp1->size(), size_gz);
    // Keep non-compressed file (temp2)?
    if (temp2->size() <= size_gz) {
      temp1->remove();
      delete temp1;
      temp1 = temp2;
      size_cmp = size_gz;
      comp_case = size_no;
      *comp_level = 0;
    } else {
      temp2->remove();
      delete temp2;
      comp_case = size_yes;
    }
  }

  // File to add to DB
  Stream dest(temp1->path(), false, false, *comp_level);

  // Get file final location
  char dest_path[PATH_MAX];
  if (getDir(source.checksum(), dest_path, true) < 0) {
    hlog_error("Cannot create DB data '%s'", source.checksum());
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
        Stream data(data_path, false, false, (no > 0) ? 1 : 0);
        data.open();
        dest.open();
        int cmp = dest.compare(data);
        dest.close();
        data.close();
        switch (cmp) {
          // Same data
          case 0:
            // Empty file is compressed
            if ((data.size() == 0) && (no > 0)) {
              status = replace;
            } else
            // Replacing data will save space
            if (size_cmp < data.size()) {
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
            hlog_error("%s comparing data '%s'", strerror(errno),
              source.checksum());
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
      (*comp_level != 0) ? "" : "un", source.checksum(), index);
    if (Directory(final_path).create() < 0) {
      hlog_error("%s creating directory '%s'", strerror(errno), final_path);
    } else
    if ((status == replace) && (::remove(data_path) < 0)) {
      hlog_error("%s removing previous data '%s'", strerror(errno), data_path);
    } else {
      char name[PATH_MAX];
      sprintf(name, "%s/data%s", final_path, (*comp_level != 0) ? ".gz" : "");
      if (rename(dest.path(), name)) {
        hlog_error("%s moving file '%s'", strerror(errno), name);
        status = error;
      } else {
        // Always add metadata (size) file (no error status on failure)
        setMetadata(final_path, source.dataSize(), comp_case);
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
    dest.remove();
  }

  // Report checksum
  if (status != error) {
    sprintf(dchecksum, "%s-%d", source.checksum(), index);
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
    long long*      size,
    bool*           compressed) const {
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
  Stream data(local_path, false, true, (no > 0) ? 1 : 0);
  // Get original file size
  long long original_size = -1;
  CompressionCase comp_status = unknown;
  getMetadata(path, &original_size, &comp_status);

  char corrupted_path[PATH_MAX];
  sprintf(corrupted_path, "%s/%s", path, "corrupted");
  // Check data for corruption
  if (thorough) {
    // Already marked corrupted?
    if (isReadable(corrupted_path)) {
      hlog_warning("Data corruption reported for %s", checksum);
      original_size = -1;
      failed = true;
    } else {
      data.setCancelCallback(aborting);
      data.setProgressCallback(_d->progress);
      // Open file
      if (data.open()) {
        hlog_error("%s opening file '%s'", strerror(errno), data.path());
        failed = true;
      } else
      // Compute file checksum
      if (data.computeChecksum()) {
        hlog_error("%s reading file '%s'", strerror(errno), data.path());
        failed = true;
      } else
      // Close file
      if (data.close()) {
        hlog_error("%s closing file '%s'", strerror(errno), data.path());
        failed = true;
      } else
      // Compare with given checksum
      if (strncmp(data.checksum(), checksum, strlen(data.checksum()))) {
        stringstream s;
        hlog_error("Data corrupted for %s%s", checksum, repair ? ", remove" : "");
        hlog_debug("Checksum: %s", data.checksum());
        failed = true;
        if (repair) {
          removePath(path);
        } else {
          // Mark corrupted
          touch(corrupted_path);
        }
        original_size = -1;
      } else
      // Compare data size and stored size for compress files
      if (data.dataSize() != original_size) {
        if (original_size >= 0) {
          hlog_error("Correcting wrong metadata for %s", checksum);
        } else {
          hlog_warning("Adding missing metadata for %s", checksum);
        }
        original_size = data.dataSize();
        setMetadata(path, original_size, later);
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
    if (original_size < 0) {
      if (no == 0) {
        hlog_warning("Setting missing metadata for %s", checksum);
        original_size = data.size();
        setMetadata(path, original_size, later);
      } else
      // Compressed file, check it thoroughly, which shall add the metadata
      {
        if (check(checksum, true, true, size, compressed)) {
          failed = true;
        }
        getMetadata(path, &original_size, &comp_status);
        display = false;
      }
    }
  } else
  if ((original_size < 0) && (errno == ENOENT)) {
    hlog_error("Metadata missing for %s", checksum);
  }

  if (display) {
    if (no == 0) {
      hlog_verbose_temp("%s %lld", checksum, original_size);
    } else {
      hlog_verbose_temp("%s %lld %lld", checksum, original_size, data.size());
    }
  }

  // Return file information if required
  if (size != NULL) {
    *size = original_size;
  }
  if (compressed != NULL) {
    *compressed = no > 0;
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
