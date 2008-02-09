/*
     Copyright (C) 2006-2007  Herve Fache

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

#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>

using namespace std;

#include "files.h"
#include "data.h"
#include "hbackup.h"

using namespace hbackup;

static bool cancel() {
  return terminating("write") != 0;
}

struct Data::Private {
  char* path;
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
      if (create && Directory(path.c_str()).create()) {
        return 1;
      }
    } else {
      break;
    }
  } while (true);
  // Return path
  path += "/" + checksum.substr(level);
  return Directory(path.c_str()).isValid() ? 0 : -1;
}

int Data::organise(
    const string&   path,
    int             number) const {
  DIR           *directory;
  struct dirent *dir_entry;
  File          nofiles(Path(path.c_str(), ".nofiles"));
  int           failed   = 0;

  // Already organised?
  if (nofiles.isValid()) {
    return 0;
  }
  // Find out how many entries
  if ((directory = opendir(path.c_str())) == NULL) {
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
      Node node(Path(path.c_str(), dir_entry->d_name));
      string source_path = path + "/" + dir_entry->d_name;
      if (node.stat()) {
        cerr << "Error: organise: cannot get metadata: " << source_path
          << endl;
        failed = 2;
      } else
      if ((node.type() == 'd')
       // If we crashed, we might have some two-letter dirs already
       && (strlen(node.name()) != 2)
       // If we've reached the point where the dir is ??-?, stop!
       && (node.name()[2] != '-')) {
        // Create two-letter directory
        string dir_path = path + "/" + node.name()[0] + node.name()[1];
        if (Directory(dir_path.c_str()).create()) {
          failed = 2;
        } else {
          // Create destination path
          string dest_path = dir_path + "/" + &node.name()[2];
          // Move directory accross, changing its name
          if (rename(source_path.c_str(), dest_path.c_str())) {
            failed = 1;
          }
        }
      }
    }
    if (! failed) {
      nofiles.create();
    }
  }
  closedir(directory);
  return failed;
}

int Data::write(
    const string&   path,
    char**          dchecksum,
    int             compress) {
  int failed = 0;

  if (path.empty()) {
    cerr << "Empty path!?!" << endl;
    return -1;
  }

  Stream source(path.c_str());
  if (source.open("r")) {
    cerr << strerror(errno) << ": " << path << endl;
    return -1;
  }

  // Temporary file to write to
  string temp_path = _d->path;
  temp_path += "/temp";
  Stream temp(temp_path.c_str());
  if (temp.open("w", compress)) {
    cerr << strerror(errno) << ": " << temp_path << endl;
    failed = -1;
  } else

  // Copy file locally
  temp.setCancelCallback(cancel);
  if (temp.copy(source)) {
    cerr << strerror(errno) << ": " << path << endl;
    failed = -1;
  }

  source.close();
  temp.close();

  if (failed) {
    return failed;
  }

  // Get file final location
  string dest_path;
  if (getDir(source.checksum(), dest_path, true) == 2) {
    cerr << "Error: write: failed to get dir for: " << source.checksum()
      << endl;
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
      Stream* try_file = new Stream(Path(final_path, "data"));

      if (try_file->isValid()) {
        try_file->open("r");
      } else {
        delete try_file;
        try_file = new Stream(Path(final_path, "data.gz"));
        if (! try_file->isValid()) {
          delete try_file;
          // Need to copy file accross: leave index to current value and exit
          break;
        }
        try_file->open("r", 1);
      }
      temp.open("r", compress);
      switch (temp.compare(*try_file, 1024)) {
        case 0:
          present = true;
          break;
        case 1:
          index++;
          break;
        default:
          failed = -1;
      }
      try_file->close();
      delete try_file;
      temp.close();
    } else {
      Directory(final_path).create();
      dest_path = final_path;
      missing = true;
    }
    free(final_path);
  } while ((failed == 0) && (! missing) && (! present));

  // Now move the file in its place
  if (! present && rename(temp_path.c_str(),
        (dest_path + "/data" + ((compress != 0) ? ".gz" : "")).c_str())) {
    cerr << "Failed to move file " << temp_path << " to "
      << dest_path << ": " << strerror(errno) << endl;
    failed = -1;
  } else
  if (! present && (verbosity() > 1)) {
    cout << "Adding " << ((compress != 0) ? "compressed " : "")
      << "file data to DB in: " << dest_path << endl;
  }

  // If anything failed, delete temporary file
  if (failed || present) {
    std::remove(temp_path.c_str());
  }

  // Report checksum
  *dchecksum = NULL;
  if (! failed) {
    asprintf(dchecksum, "%s-%d", source.checksum(), index);
  }

  // Make sure we won't exceed the file number limit
  if (! failed) {
    // dest_path is /path/to/checksum
    unsigned int pos = dest_path.rfind('/');

    if (pos != string::npos) {
      dest_path.erase(pos);
      // Now dest_path is /path/to
      organise(dest_path, 256);
    }
  }

  return failed;
}

int Data::crawl(
    Directory&      dir,
    const string&   checksumPart,
    bool            thorough,
    list<string>&   checksums) const {
  if (dir.isValid() && ! dir.createList()) {
    bool no_files = false;
    list<Node*>::iterator i = dir.nodesList().begin();
    while (i != dir.nodesList().end()) {
      if (((*i)->type() == 'f') && (strcmp((*i)->name(), ".nofiles") == 0)) {
        no_files = true;
      }
      if ((*i)->type() == 'd') {
        string checksum = checksumPart + (*i)->name();
        if (no_files) {
          Directory *d = new Directory(**i);
          crawl(*d, checksum, thorough, checksums);
          delete d;
        } else {
          if (verbosity() > 1) {
            cout << " -> checking: " << checksum << endl;
          }
          if (! check(checksum, thorough)) {
            checksums.push_back(checksum);
          }
        }
      }
      delete *i;
      i = dir.nodesList().erase(i);
    }
  } else {
    return -1;
  }
  return 0;
}

int Data::parseChecksums(
    list<string>&   checksums) {
  return -1;
}

Data::Data() {
  _d       = new Private;
  _d->path = NULL;
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
    Directory dir(path);
    if (dir.isValid()) {
      return 0;
    }
    if (! create) {
      goto failed;
    }
    if (dir.create()) {
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
  free(_d->path);
  _d->path = NULL;
}

int Data::read(const string& path, const string& checksum) {
  int failed = 0;

  string source_path;
  if (getDir(checksum, source_path)) {
    cerr << "Error: read: failed to get dir for: " << checksum << endl;
    return -1;
  }

  // Open source
  bool decompress = false;
  source_path += "/data";
  if (! File(source_path.c_str()).isValid()) {
    source_path += ".gz";
    decompress = true;
  }
  Stream source(source_path.c_str());
  if (source.open("r", decompress ? 1 : 0)) {
    cerr << "Error: failed to open read source file: " << source_path << endl;
    return -1;
  }

  // Open temporary file to write to
  string temp_path = path + ".part";
  Stream temp(temp_path.c_str());
  if (temp.open("w")) {
    cerr << "Error: failed to open read dest file: " << temp_path << endl;
    failed = 2;
  } else

  // Copy file to temporary name (size not checked: checksum suffices)
  temp.setCancelCallback(cancel);
  if (temp.copy(source)) {
    cerr << "Error: failed to read file: " << source_path << endl;
    failed = 2;
  }

  source.close();
  temp.close();

  if (! failed) {
    // Verify that checksums match before overwriting final destination
    if (strncmp(checksum.c_str(), temp.checksum(), strlen(temp.checksum()))) {
      cerr << "Error: read checksums don't match: " << checksum << " "
        << temp.checksum() << endl;
      failed = 2;
    } else

    // All done
    if (rename(temp_path.c_str(), path.c_str())) {
      cerr << "Error: failed to rename read file to " << strerror(errno)
        << ": " << path << endl;
      failed = 2;
    }
  }

  if (failed) {
    std::remove(temp_path.c_str());
  }

  return failed;
}

int Data::check(
    const string&   checksum,
    bool            thorough) const {
  string path;
  if (getDir(checksum, path)) {
    errno = EUCLEAN;
    return -1;
  }
  bool failed = false;
  Stream *data = NULL;
  // Uncompressed data
  if (File(Path(path.c_str(), "data")).isValid()) {
    if (thorough) {
      data = new Stream(Path(path.c_str(), "data"));
      if (data->open("r")) {
        errno = EINVAL;
        failed = true;
      }
    } else {
      return 0;
    }
  } else
  // Compressed data
  if (File(Path(path.c_str(), "data.gz")).isValid()) {
    if (thorough) {
      data = new Stream(Path(path.c_str(), "data.gz"));
      if (data->open("r", 1)) {
        errno = EINVAL;
        failed = true;
      }
    } else {
      return 0;
    }
  } else
  // Missing data
  {
    cerr << "Error: data missing for " << checksum << endl;
    errno = ENOENT;
    return -1;
  }
  if (data->computeChecksum() || data->close()) {
    errno = EINVAL;
    failed = true;
  } else
  if (strncmp(data->checksum(), checksum.c_str(), strlen(data->checksum()))){
    cerr << "Error: data corrupted for " << checksum << endl;
    errno = EUCLEAN;
    failed = true;
  }
  delete data;
  return failed ? -1 : 0;
}

int Data::remove(
    const string&   checksum) {
  int rc;
  string path;
  if (getDir(checksum, path)) {
    return 1;
  }
  File* f;
  f = new File(Path(path.c_str(), "data"));
  if (! f->isValid()) {
    delete f;
    f = new File(Path(path.c_str(), "data.gz"));
    if (! f->isValid()) {
      delete f;
      f = NULL;
    }
  }
  if (f != NULL) {
    rc = f->remove();
  } else {
    rc = 2;
  }
  delete f;
  remove(path.c_str());
  return rc;
}
