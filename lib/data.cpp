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
      // Ignore . and ..
      if (! strcmp(dir_entry->d_name, ".")
       || ! strcmp(dir_entry->d_name, "..")) {
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
    const string&   checksumPart,
    list<string>*   checksums,
    bool            thorough,
    bool            remove,
    unsigned int*   valid,
    unsigned int*   broken) const {
  bool failed = false;
  if (dir.isValid() && ! dir.createList()) {
    bool no_files = false;
    list<Node*>::iterator i = dir.nodesList().begin();
    while (i != dir.nodesList().end()) {
      if (failed) {
        // Skip any processing, but remove node from list
      } else
      if ((*i)->type() == 'f') {
        if (strcmp((*i)->name(), ".nofiles") == 0) {
          no_files = true;
        }
      } else
      if ((*i)->type() == 'd') {
        string checksum = checksumPart + (*i)->name();
        if (no_files) {
          Directory d(**i);
          if (crawl_recurse(d, checksum, checksums, thorough, remove, valid,
              broken) < 0){
            failed = true;
          }
        } else {
          out(verbose, msg_standard, checksum.c_str(), -3);
          if (! check(checksum.c_str(), thorough, remove)) {
            if (checksums != NULL) {
              checksums->push_back(checksum);
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

Data::Data() {
  _d       = new Private;
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
  asprintf(&_d->temp, "%s/%s", _d->path, "temp");
  if ((_d->path == NULL) || (_d->temp == NULL)) {
    goto failed;
  }
  {
    Directory dir(path);
    if (dir.isValid()) {
      return 0;
    }
    if (! create) {
      goto failed;
    }
    if (dir.create() < 0) {
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

int Data::read(
    const char*     path,
    const char*     checksum) {
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
  } else

  // Copy file to temporary name (size not checked: checksum suffices)
  temp.setCancelCallback(aborting);
  data->setProgressCallback(_d->progress);
  if (temp.copy(*data)) {
    out(error, msg_errno, "Copying read file", errno, data->path());
    failed = true;
  }

  data->close();
  temp.close();

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

int Data::setTemp(const char* path) {
  free(_d->temp);
  _d->temp = strdup(path);
  return (_d->temp == NULL) ? -1 : 0;
}

int Data::write(
    const char*     path,
    char**          dchecksum,
    int             compress) {
  bool failed = false;

  Stream source(path);
  if (source.open("r")) {
    out(error, msg_errno, "Opening write source file", errno, path);
    return -1;
  }

  // Temporary file to write to
  Stream temp(_d->temp);
  if (temp.open("w", compress)) {
    out(error, msg_errno, "Opening write temp file", errno, temp.path());
    failed = true;
  } else

  // Copy file locally
  temp.setCancelCallback(aborting);
  source.setProgressCallback(_d->progress);
  if (temp.copy(source)) {
    out(error, msg_errno, "Copying write file", errno, temp.path());
    failed = true;
  }

  source.close();
  temp.close();

  if (failed) {
    return -1;
  }

  // Get file final location
  string dest_path;
  if (getDir(source.checksum(), dest_path, true) < 0) {
    out(error, msg_standard, "Cannot create DB data", -1, source.checksum());
    return -1;
  }

  // Make sure our checksum is unique: compare first bytes of files
  int  index = 0;
  bool missing = false;
  bool present = false;
  do {
    char* final_path = NULL;
    asprintf(&final_path, "%s-%u", dest_path.c_str(), index);

    // Check whether directory already exists
    if (Directory(final_path).isValid()) {
      vector<string> extensions;
      extensions.push_back("");
      extensions.push_back(".gz");
      unsigned int no;
      Stream *data = Stream::select(Path(final_path, "data"), extensions, &no);
      if (data == NULL) {
        // Need to copy file accross: leave index to current value and exit
        dest_path = final_path;
        break;
      }
      data->open("r", (no > 0) ? 1 : 0);
      temp.open("r", compress);
      switch (temp.compare(*data, 1024)) {
        case 0:
          present = true;
          break;
        case 1:
          index++;
          break;
        default:
          failed = true;
      }
      data->close();
      delete data;
      temp.close();
    } else {
      Directory(final_path).create();
      dest_path = final_path;
      missing = true;
    }
    free(final_path);
  } while (! failed && ! missing && ! present);

  // Now move the file in its place
  if (! present && rename(temp.path(),
      (dest_path + "/data" + ((compress != 0) ? ".gz" : "")).c_str())) {
    stringstream s;
    s << "Failed to move file " << temp.path() << " to " << dest_path << ": "
      << strerror(errno);
    out(error, msg_standard, s.str().c_str());
    failed = true;
  } else
  if (! present) {
    stringstream s;
    s << "Adding " << ((compress != 0) ? "compressed " : "")
      << "file data to DB for " << source.checksum() << "-" << index;
    out(verbose, msg_standard, s.str().c_str());
  }

  // If anything failed, delete temporary file
  if (failed || present) {
    temp.remove();
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

  return failed ? -1 : 0;
}

int Data::check(
    const char*     checksum,
    bool            thorough,
    bool            remove) const {
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
  } else
  // Check data for corruption
  if (thorough) {
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
      failed = true;
      if (remove) {
        data->remove();
        std::remove(path.c_str());
      } else {
        // Mark corrupted
        File(Path(path.c_str(), "corrupted")).create();
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
  std::remove(path.c_str());
  return rc;
}

int Data::crawl(
    list<string>*   checksums,
    bool            thorough,
    bool            remove) const {
  Directory d(_d->path);
  unsigned int valid  = 0;
  unsigned int broken = 0;
  int rc = crawl_recurse(d, "", checksums, thorough, remove, &valid, &broken);
  stringstream s;
  s << "Found " << valid << " valid and " << broken << " broken data files";
  out(verbose, msg_standard, s.str().c_str());
  return rc;
}
