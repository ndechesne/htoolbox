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

// Compression to use when required: gzip -5 (best speed/ratio)

#include <sstream>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>

using namespace std;

#include "hbackup.h"
#include "files.h"
#include "report.h"
#include "compdata.h"
#include "data.h"

using namespace hbackup;

struct Data::Private {
  char*             path;
  char*             temp;
  progress_f        progress;
};

int Data::getDir(
    const string& checksum,
    string&       path,
    bool          create) const {
  path = _d->path;
  int level = 0;

  // Two cases: either there are files, or a .nofiles file and directories
  do {
    // If we can find a .nofiles file, then go down one more directory
    if (File(Path(path.c_str(), ".nofiles")).isValid()) {
      path += "/" + checksum.substr(level, 2);
      level += 2;
      if (create && (Directory(path.c_str()).create() < 0)) {
        return -1;
      }
    } else {
      break;
    }
  } while (true);
  // Return path
  path += "/" + checksum.substr(level);
  return Directory(path.c_str()).isValid() ? 0 : 1;
}

int Data::organise(
    const char*     path,
    int             number) const {
  DIR           *directory;
  struct dirent *dir_entry;
  File          nofiles(Path(path, ".nofiles"));
  bool          failed = false;

  // Already organised?
  if (nofiles.isValid()) {
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
        out(error, msg_errno, "Stating source file", errno,
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
              Path(dir.path(), &source_path.name()[2]))) {
            failed = true;
          }
        }
      }
    }
    if (! failed) {
      nofiles.create();
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
    bool            remove,
    unsigned int*   valid,
    unsigned int*   broken) const {
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
            if (crawl_recurse(d, checksum, data, thorough, remove, valid,
                broken) < 0){
              failed = true;
            }
          } else {
            out(verbose, msg_standard, checksum.c_str(), -3);
            bool compressed;
            string path;
            if (! check(checksum.c_str(), thorough, remove, &compressed,
                &path)) {
              Stream s(path.c_str());
              if (data != NULL) {
                if (compressed) {
                  CompData d(checksum.c_str(), s.getOriginalSize(), true);
                  data->push_back(d);
                } else {
                  CompData d(checksum.c_str(), s.size());
                  data->push_back(d);
                }
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

Data::Data() : _d(new Private) {
  _d->path = NULL;
  _d->temp = NULL;
  // Reset progress callback function
  _d->progress = NULL;
}

Data::~Data() {
  close();
  delete _d;
}

int Data::open(const char* path, bool create) {
  _d->path = strdup(path);
  if (_d->path == NULL) {
    goto failed;
  }
  {
    // Base directory
    Directory dir(_d->path);
    // Place for temporary files
    Directory temp_dir(Path(_d->path, ".temp"));
    _d->temp = strdup(temp_dir.path());
    // Check for existence
    if (dir.isValid() && temp_dir.isValid()) {
      return 0;
    }
    if (! dir.isValid() && ! create) {
      goto failed;
    }
    if ((dir.create() < 0) || (temp_dir.create() < 0)) {
      goto failed;
    }
  }
  // Signal creation
  return 1;
failed:
  close();
  return -1;
}

void Data::close() {
  free(_d->temp);
  _d->temp = NULL;
  free(_d->path);
  _d->path = NULL;
}

void Data::setProgressCallback(progress_f progress) {
  _d->progress = progress;
}

int Data::name(
    const char*     checksum,
    string&         path,
    string&         extension) const {
  if (getDir(checksum, path)) {
    return -1;
  }
  unsigned int    no;
  vector<string>  extensions;
  extensions.push_back("");
  extensions.push_back(".gz");
  Stream *data = Stream::select(Path(path.c_str(), "data"), extensions, &no);
  if (data == NULL) {
    return -1;
  }
  path      = data->path();
  extension = extensions[no];
  return 0;
}

int Data::read(
    const char*     path,
    const char*     checksum) const {
  bool failed = false;

  string source;
  if (getDir(checksum, source)) {
    out(error, msg_standard, "Cannot access DB data for", -1, checksum);
    return -1;
  }

  // Open source
  vector<string> extensions;
  extensions.push_back("");
  extensions.push_back(".gz");
  unsigned int no;
  Stream *data = Stream::select(Path(source.c_str(), "data"), extensions, &no);
  if (data == NULL) {
    return -1;
  }
  if (data->open("r", (no > 0) ? 1 : 0)) {
    out(error, msg_errno, "Opening read source file", errno, data->path());
    return -1;
  }

  // Open temporary file to write to
  string temp_path = path;
  temp_path += ".hbackup-part";
  Stream temp(temp_path.c_str());
  if (temp.open("w")) {
    out(error, msg_errno, "Opening read temp file", errno, data->path());
    failed = true;
  } else {
    // Copy file to temporary name (size not checked: checksum suffices)
    data->setCancelCallback(aborting);
    data->setProgressCallback(_d->progress);
    if (data->copy(&temp)) {
      failed = true;
    }
    temp.close();
  }

  data->close();

  if (! failed) {
    // Verify that checksums match before overwriting final destination
    if (strncmp(checksum, temp.checksum(), strlen(temp.checksum()))) {
      out(error, msg_standard, "Read checksums don't match");
      failed = true;
    } else

    // All done
    if (rename(temp_path.c_str(), path)) {
      out(error, msg_errno, "Renaming read file", errno, path);
      failed = true;
    }
  }

  if (failed) {
    std::remove(temp_path.c_str());
  }
  delete data;

  return failed ? -1 : 0;
}

int Data::write(
    const char*     path,
    const char*     temp_name,
    char**          dchecksum,
    int             compress,
    int*            acompress) {
  bool failed  = false;

  // Open source file
  Stream source(path);
  if (source.open("r")) {
    return -1;
  }

  // Temporary file(s) to write to
  Stream* temp1 = new Stream(Path(_d->temp, temp_name));
  Stream* temp2 = NULL;
  if (compress < 0) {
    char* temp_path_gz;
    asprintf(&temp_path_gz, "%s.gz", temp1->path());
    temp2 = new Stream(temp_path_gz);
    free(temp_path_gz);
    if (temp2->open("w", -compress, -1, false, source.size())) {
      out(error, msg_errno, "Opening write temp file", errno, temp2->path());
      failed = true;
    }
  }
  if (temp1->open("w", (compress > 0) ? compress:0, -1, false, source.size())){
    out(error, msg_errno, "Opening write temp file", errno, temp1->path());
    failed = true;
  }

  if (! failed) {
    // Copy file locally
    source.setCancelCallback(aborting);
    source.setProgressCallback(_d->progress);
    if (source.copy(temp1, temp2) != 0) {
      failed = true;
    }

    temp1->close();
    if (temp2 != NULL) {
      temp2->close();
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
    return -1;
  }

  // File to add to DB
  Stream*   dest = temp1;
  // Size for comparison with existing DB data
  long long size_cmp = temp1->size();
  if (temp2 != NULL) {
    // Add ~1.6% to gzip'd size
    long long size_gz = temp2->size() + (temp2->size() >> 6);
    stringstream s;
    s << "Checking data, sizes: f=" << temp1->size() << " z=" << temp2->size();
    s << " (" << size_gz << ")";
    out(debug, msg_standard, s.str().c_str());
    // Keep compressed file?
    if (temp1->size() > size_gz) {
      temp1->remove();
      delete temp1;
      dest     = temp2;
      size_cmp = size_gz;
      compress = -compress;
    } else {
      temp2->remove();
      delete temp2;
      compress = 0;
    }
  }

  if (acompress != NULL) {
    *acompress = compress;
  }

  // Get file final location
  string dest_path;
  if (getDir(source.checksum(), dest_path, true) < 0) {
    out(error, msg_standard, "Cannot create DB data", -1, source.checksum());
    return -1;
  }

  // Make sure our checksum is unique: compare first bytes of files
  int  index   = 0;
  enum {
    none    = -1,
    leave   = 0,
    add     = 1,
    replace = 2
  } action = none;
  Stream* data       = NULL;
  char*   final_path = NULL;
  do {
    asprintf(&final_path, "%s-%u", dest_path.c_str(), index);

    // Directory does not exist
    if (! Directory(final_path).isValid()) {
      action = add;
    } else {
      vector<string> extensions;
      extensions.push_back("");
      extensions.push_back(".gz");
      unsigned int no;
      delete data;
      data = Stream::select(Path(final_path, "data"), extensions, &no);
      // File does not exist
      if (data == NULL) {
        action = add;
      } else
      // Compare files
      {
        data->open("r", (no > 0) ? 1 : 0);
        dest->open("r", compress);
        int cmp = dest->compare(*data, 10*1024*1024);
        dest->close();
        data->close();
        switch (cmp) {
          // Same data
          case 0:
            // Gzip file format incorrect
            if ((no > 0) && (data->getOriginalSize() < 0)) {
              action = replace;
            } else
            // Replacing data will save space
            if (size_cmp < data->size()) {
              action = replace;
            } else
            // Nothing to do
            {
              action = leave;
            }
            break;
          // Different data
          case 1:
            index++;
            break;
          // Error
          default:
            out(error, msg_errno, "Failed to compare data", errno,
              source.checksum());
            failed = true;
        }
      }
    }
  } while (! failed && (action == none));

  // Now move the file in its place
  if ((action == add) || (action == replace)) {
    stringstream s;
    s << ((action == add) ? "Adding " : "Replacing with ")
      << ((compress != 0) ? "" : "un") << "compressed data for "
      << source.checksum() << "-" << index;
    out(debug, msg_standard, s.str().c_str());
    if (Directory(final_path).create() < 0) {
      out(error, msg_errno, "Creating directory", errno, final_path);
    } else
    if ((action == replace) && (data->remove() < 0)) {
      out(error, msg_errno, "Removing previous data", errno);
    } else {
      char* name;
      asprintf(&name, "%s/data%s", final_path, ((compress != 0) ? ".gz" : ""));
      if (rename(dest->path(), name)) {
        out(error, msg_errno, "Failed to move file", errno, name);
        failed = true;
      }
      free(name);
    }
  }

  // If anything failed, delete temporary file
  if (failed || (action == leave)) {
    dest->remove();
  }

  // Report checksum
  *dchecksum = NULL;
  if (! failed) {
    asprintf(dchecksum, "%s-%d", source.checksum(), index);

    // Make sure we won't exceed the file number limit
    // dest_path is /path/to/checksum
    unsigned int pos = dest_path.rfind('/');

    if (pos != string::npos) {
      dest_path.erase(pos);
      // Now dest_path is /path/to
      organise(dest_path.c_str(), 256);
    }
  }

  free(final_path);
  delete dest;
  delete data;
  return failed ? -1 : action;
}

int Data::check(
    const char*     checksum,
    bool            thorough,
    bool            remove,
    bool*           compressed,
    string*         data_path) const {
  string path;
  if (getDir(checksum, path)) {
    return -1;
  }
  bool failed = false;
  unsigned int no;
  vector<string> extensions;
  extensions.push_back("");
  extensions.push_back(".gz");
  Stream *data = Stream::select(Path(path.c_str(), "data"), extensions, &no);
  // Missing data
  if (data == NULL) {
    out(error, msg_standard, "Data missing", -1, checksum);
    if (remove) {
      File(Path(path.c_str(), "corrupted")).remove();
      std::remove(path.c_str());
    }
    return -1;
  }
  // Return file information if required
  if (compressed != NULL) {
    *compressed = no > 0;
  }
  if (data_path != NULL) {
    *data_path = data->path();
  }
  // Check data for corruption
  if (thorough) {
    // Already marked corrupted?
    if (File(Path(path.c_str(), "corrupted")).isValid()) {
      out(error, msg_standard, "Data corruption reported", -1, checksum);
      failed = true;
    } else {
      data->setCancelCallback(aborting);
      if (data->open("r", (no > 0) ? 1 : 0)) {
        out(error, msg_errno, "Opening file", errno, data->path());
        failed = true;
      } else
      if (data->computeChecksum() || data->close()) {
        out(error, msg_errno, "Reading file", errno, data->path());
        failed = true;
      } else
      if (strncmp(data->checksum(), checksum, strlen(data->checksum()))) {
        stringstream s;
        s << "Data corrupted" << (remove ? ", remove" : "");
        out(error, msg_standard, s.str().c_str(), -1, checksum);
        out(debug, msg_standard, data->checksum(), -1, "Checksum");
        failed = true;
        if (remove) {
          data->remove();
          std::remove(path.c_str());
        } else {
          // Mark corrupted
          File(Path(path.c_str(), "corrupted")).create();
        }
      }
    }
  } else
  // Remove data marked corrupted
  if (remove) {
    File corrupted(Path(path.c_str(), "corrupted"));
    if (corrupted.isValid()) {
      if (data->remove() == 0) {
        corrupted.remove();
        std::remove(path.c_str());
        out(info, msg_standard, "Remove corrupted data", -1, checksum);
      } else {
        out(error, msg_errno, "Removing data", errno, checksum);
      }
      failed = true;
    }
  }
  delete data;
  return failed ? -1 : 0;
}

int Data::remove(
    const char*     checksum) {
  int rc;
  string path;
  if (getDir(checksum, path)) {
    // Warn about directory missing
    return 1;
  }
  vector<string> extensions;
  extensions.push_back("");
  extensions.push_back(".gz");
  Stream *data = Stream::select(Path(path.c_str(), "data"), extensions);
  if (data != NULL) {
    rc = data->remove();
  } else {
    // Warn about data missing
    rc = 2;
  }
  delete data;
  int errno_keep = errno;
  std::remove(path.c_str());
  errno = errno_keep;
  return rc;
}

int Data::crawl(
    bool            thorough,
    bool            remove,
    list<CompData>* data) const {
  Directory d(_d->path);
  unsigned int valid  = 0;
  unsigned int broken = 0;
  int rc = crawl_recurse(d, "", data, thorough, remove, &valid, &broken);
  stringstream s;
  s << "Found " << valid << " valid and " << broken << " broken data files";
  out(verbose, msg_standard, s.str().c_str());
  return rc;
}
