/*
     Copyright (C) 2007-2010  Herve Fache

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

#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>

#include "hbackup.h"
#include "line.h"
#include "files.h"
#include "hreport.h"
#include "list.h"

using namespace hbackup;
using namespace hreport;

int List::open(bool quiet_if_not_exists) {
  // Choose mode
  const char* mode;
  if (_type == list_read) {
    mode = "r";
  } else {
    mode = "w";
  }
  // Open
  _fd = fopen(_path.c_str(), mode);
  if (_fd == NULL) {
    if (! quiet_if_not_exists) {
      hlog_error("%s opening '%s' for %sing",
        strerror(errno), _path.c_str(), _type == list_read ? "read" : "writ");
    }
    goto failed;
  }
  if (_type == list_read) {
    _previous_offset = 0;
    bool went_wrong = false;
    char* buffer = NULL;
    size_t capacity;
    _offset = getline(&buffer, &capacity, _fd);
    if (_offset <= 0) {
      const char* error_str;
      if (feof(_fd)) {
        error_str = "EOF reached";
      } else {
        error_str = strerror(errno);
      }
      hlog_error("%s getting version for '%s'", error_str, _path.c_str());
      went_wrong = true;
    } else {
      if (strcmp(buffer, _header()) == 0) {
        _old_version = false;
      } else
      if (strcmp(buffer, _old_header()) == 0) {
        _old_version = true;
      } else
      {
        buffer[sizeof(buffer) - 1] = '\0';
        hlog_error("Unknown version for '%s': '%s'", _path.c_str(), buffer);
        went_wrong = true;
      }
    }
    free(buffer);
    if (went_wrong) goto failed;
  } else {
    if (putLine(_header()) < 0) {
      hlog_error("%s writing header in '%s'", strerror(errno), _path.c_str());
      goto failed;
    }
  }
  hlog_regression("opened '%s' for %sing",
    _path.c_str(), _type == list_read ? "read" : "writ");
  return 0;
failed:
  close();
  return -1;
}

int List::close() {
  int rc;
  if (_fd != NULL) {
    if (_type != list_read) {
      if (putLine(_footer()) < 0) {
        hlog_error("%s writing footer in '%s'", strerror(errno), _path.c_str());
        /* the error is reported by fclose */
      }
    }
    rc = fclose(_fd);
    _fd = NULL;
    hlog_regression("closed '%s'", _path.c_str());
  } else {
    errno = EBADF;
    rc = -1;
  }
  return rc;
}

void List::setProgressCallback(progress_f progress) {
  if (_type != list_read) {
    return;
  }
  // Get list file size
  if (_size < 0) {
    struct stat64 stat;
    if (stat64(_path.c_str(), &stat) < 0) {
      return;
    }
    _size = stat.st_size;
  }
  _progress = progress;
}

ssize_t List::putLine(const char* line) {
  if (_fd == NULL) {
    hlog_error("'%s' not open", _path.c_str());
    return -1;
  }
  if (_type == list_read) {
    hlog_error("'%s' read only", _path.c_str());
    return -1;
  }
  ssize_t line_size = strlen(line);
  if (line_size > 0) {
    if (fwrite(line, line_size, 1, _fd) < 1) {
      hlog_error("%s writing line to '%s'", strerror(errno), _path.c_str());
      return -1;
    }
  }
  if (fwrite("\0\n", 2, 1, _fd) < 1) {
    hlog_error("%s writing line ending to '%s'", strerror(errno), _path.c_str());
    return -1;
  }
  return line_size;
}

inline ssize_t List::flushStored() {
  size_t line_size = 0;
  if (_file_path[0] != '\0') {
    line_size = putLine(_file_path);
    // Write path, and check that its length is greater than 1
    if (line_size < 1) {
      hlog_error("%s flushing file path to '%s'", strerror(errno), _path.c_str());
      return -1;
    }
    hlog_regression("DBG %s flush _file_path = %s into %s", __FUNCTION__,
      _file_path, _path.c_str());
    _file_path[0] = '\0';
  }
  // Write removed mark if relevant
  if (_removed > 0) {
    char line[32];
    sprintf(line, "\t%ld\t-", _removed);
    ssize_t ts_size = putLine(line);
    if (ts_size < 1) {
      hlog_error("%s flushing line to '%s'", strerror(errno), _path.c_str());
      return -1;
    }
    hlog_regression("DBG %s flush _removed = %ld into %s", __FUNCTION__,
      _removed, _path.c_str());
    line_size += ts_size;
    _removed = 0;
  }
  return line_size;
}

ssize_t List::putPath(const char* path) {
  if (_type != journal_write) {
    // Stored data is discarded
    _removed = 0;
    // Only store path for now. No, I don't care about size.
    strcpy(_file_path, path);
    hlog_regression("DBG %s store _file_path = %s into %s", __FUNCTION__,
      _file_path, _path.c_str());
  } else {
    return putLine(path);
  }
  return 0;
}

ssize_t List::putData(const char* ts_metadata_extra) {
  ssize_t line_size = 0;
  time_t ts = 0;
  if (_type != journal_write) {
    const char* second_tab = strchr(&ts_metadata_extra[1], '\t');
    if ((second_tab == NULL) ||
        (second_tab[1] != '-') ||
        (sscanf(ts_metadata_extra, "%ld", &ts) != 1)) {
      // Make sure sscanf did not store anything
      ts = 0;
    }
    // So it _is_ a removed mark
    if (ts != 0) {
      _removed = ts;
      hlog_regression("DBG %s1 store _removed = %ld into %s", __FUNCTION__,
        _removed, _path.c_str());
      return 0;
    } else {
      line_size = flushStored();
      if (line_size < 0) {
        return -1;
      }
    }
  }
  // Write line of metadata
  ssize_t ts_metadata_extra_size = putLine(ts_metadata_extra);
  if (ts_metadata_extra_size < 1) {
    hlog_error("%s writing ts_metadata_extra to '%s'", strerror(errno),
      _path.c_str());
    return -1;
  }
  line_size += ts_metadata_extra_size;
  return line_size;
}

ssize_t List::putData(time_t ts, const char* metadata, const char* extra) {
  ssize_t line_size = 0;
  if (_type != journal_write) {
    // So it _is_ a removed mark
    if (metadata[0] == '-') {
      _removed = ts;
      hlog_regression("DBG %s2 store _removed = %ld into %s", __FUNCTION__,
        _removed, _path.c_str());
      return 0;
    } else {
      line_size = flushStored();
      if (line_size < 0) {
        return -1;
      }
    }
  }
  // Write line of metadata
  ssize_t metadata_size = fprintf(_fd, "\t%ld\t%s", ts, metadata);
  if (metadata_size < 1) {
    hlog_error("%s write ts_str to '%s'", strerror(errno), _path.c_str());
    return -1;
  }
  line_size += metadata_size;
  if (extra != NULL) {
    size_t extra_size = fprintf(_fd, "\t%s", extra);
    if (extra_size < 1) {
      hlog_error("%s write separator to '%s'", strerror(errno), _path.c_str());
      return -1;
    }
    line_size += extra_size;
  }
  if (fwrite("\0\n", 2, 1, _fd) < 1) {
    hlog_error("%s write end of line to '%s'", strerror(errno), _path.c_str());
    return -1;
  }
  return line_size;
}

ssize_t List::getLine(char** buffer_p, size_t* cap_p) {
  if (_fd == NULL) return -1;
  if (_type != list_read) return -1;
  char delim;
  if (! _old_version) {
    delim = '\0';
  } else {
    delim = '\n';
  }
  ssize_t rc = getdelim(buffer_p, cap_p, delim, _fd);
  if (rc <= 0) {
    if (feof(_fd)) {
      return -2;
    }
    return -1;
  }
  _offset += rc;
  char* line = *buffer_p;
  if (line[rc - 1] != delim) {
    return -2;
  }
  if (! _old_version) {
    char lf[1];
    if ((fread(lf, 1, 1, _fd) < 1) || (*lf != '\n')) {
      return -2;
    }
    ++_offset;
  }
  if ((_progress != NULL) && (_size >= 0)) {
    if ((_offset == _size) ||
        ((_offset - _previous_offset) >= _offset_threadshold)) {
      _progress(_previous_offset, _offset, _size);
      _previous_offset = _offset;
    }
  }
  if (line[0] == '#') {
    return 0;
  }
  --rc;
  line[rc] = '\0';
  return rc;
}

struct ListReader::Private {
  List              file;
  Status            status;
  char*             path;
  size_t            path_cap;
  char*             data;
  size_t            data_cap;
  Private(const char* path_in) : file(path_in, List::list_read) {
    data_cap = path_cap = 256;
    path = static_cast<char*>(malloc(path_cap));
    path[0] = '\0'; // Let it crash if malloc failed
    data = static_cast<char*>(malloc(data_cap));
    data[0] = '\0'; // Let it crash if malloc failed
  }
  ~Private() {
    free(path);
    free(data);
  }
};

ListReader::ListReader(const Path& path) : _d(new Private(path)) {}

ListReader::~ListReader() {
  delete _d;
}

int ListReader::open(bool quiet_if_not_exists) {
  if (_d->file.open(quiet_if_not_exists) < 0) {
    return -1;
  }
  _d->status = got_nothing;
  _d->path[0] = '\0';
  // Get first line to check for empty list
  return (fetchLine() == ListReader::failed) ? -1 : 0;
}

int ListReader::close() {
  return _d->file.close();
}

const char* ListReader::path() const {
  return _d->file.path();
}

const char* ListReader::getPath() const {
  return _d->path;
}

const char* ListReader::getData() const {
  return _d->data;
}

// This is read buffer: unless the status is 'got_nothing', we do not fetch
// anything. The status is reset using resetStatus, and only if the status is
// got_data or got_path.
ListReader::Status ListReader::fetchLine() {
  // Check current status
  if (_d->status == ListReader::got_nothing) {
    // Line contains no re-usable data
    ssize_t length = _d->file.getLine(&_d->data, &_d->data_cap);
    switch (length) {
      case -2:
        // Unexpected end of list or incorrect end of line
        _d->status = ListReader::eof;
        break;
      case -1:
        // Read error
        _d->status = ListReader::failed;
        break;
      case 0:
        // End of list
        _d->status = ListReader::eor;
        break;
      default:
        // Usable information
        if (_d->data[0] != '\t') {
          if (strcmp(_d->path, _d->data) != 0) {
            // Swap buffers
            char* temp = _d->data;
            _d->data = _d->path;
            _d->path = temp;
          } else {
            hlog_error("Repeated path in list '%s'", _d->path);
          }
          _d->status = ListReader::got_path;
        } else {
          _d->status = ListReader::got_data;
        }
    }
  }
  return _d->status;
}

void ListReader::resetStatus() {
  if ((_d->status == got_data) || (_d->status == got_path)) {
    _d->status = got_nothing;
  }
}

bool ListReader::end() const {
  // If EOF is reached immediately, list is empty
  return (_d->status == eor) || (_d->status == eof);
}

size_t ListReader::encode(
    const Node&     node,
    char*           line,
    size_t*         sep_offset_p) {
  // Also get offset to the TAB before the UID = necessary length to compare
  *sep_offset_p = sprintf(line, "%c\t%lld\t%ld",
    node.type(), node.size(), node.mtime());
  size_t end_offset = *sep_offset_p;
  end_offset += sprintf(&line[*sep_offset_p], "\t%u\t%u\t%o",
    node.uid(), node.gid(), node.mode());
  return end_offset;
}

int ListReader::decodeLine(
    const char*     line,
    time_t*         ts,
    const char*     path,
    Node**          node) {
  // Fields
  char        type;             // file type
  long long   size;             // on-disk size, in bytes
  time_t      mtime;            // time of last modification
  uid_t       uid;              // user ID of owner
  gid_t       gid;              // group ID of owner
  mode_t      mode;             // permissions
  char*       extra = NULL;     // linked file or checksum or null

  int num;
  if (node != NULL) {
    num = sscanf(line, "\t%ld\t%c\t%Ld\t%ld\t%d\t%d\t%o\t%a[^\t]",
      ts, &type, &size, &mtime, &uid, &gid, &mode, &extra);
  } else {
    num = sscanf(line, "\t%ld\t%c\t",
      ts, &type);
  }

  // Check number of params
  if (num < 0) {
    return -1;
  }
  if (num < 2) {
    hlog_error("Wrong number of arguments for line");
    return -1;
  }

  // Return extracted data
  if (node != NULL) {
    switch (type) {
    case 'f':
      *node = new File(path, type, mtime, size, uid, gid, mode,
        extra ? extra : "");
      break;
    case 'l':
      *node = new Link(path, type, mtime, size, uid, gid, mode,
        extra ? extra : "");
      break;
    case '-':
      *node = NULL;
      break;
    default:
      *node = new Node(path, type, mtime, size, uid, gid, mode);
    }
  }
  free(extra);
  return 0;
}

void ListReader::setProgressCallback(progress_f progress) {
  _d->file.setProgressCallback(progress);
}

const char* ListReader::fetchData() {
  if (fetchLine() != ListReader::got_data) {
    hlog_error("Unexpectedly failed to find data in list in '%s'",
      _d->file.path());
    return NULL;
  }
  return getData();
}

int ListReader::fetchData(
    Node**          node) {
  // Initialise
  delete *node;
  *node = NULL;

  int rc;
  do {
    // Get line
    rc = fetchLine();
    if (rc < 0) {
      if (rc == ListReader::eof) {
        hlog_error("Unexpected end of list in '%s'", _d->file.path());
      }
      return -1;
    }
    // End of file
    if (rc == ListReader::eor) {
      return 0;
    }
    if (rc == ListReader::got_data) {
      time_t ts;
      // Get all arguments from line
      ListReader::decodeLine(getData(), &ts, getPath(), node);
    }
    resetStatus();
  } while (rc != ListReader::got_data);
  return 1;
}

int ListReader::searchPath() {
  while (true) {
    // Read list or get last data
    int rc = fetchLine();

    // Failed
    if (rc == ListReader::eof) {
      // Unexpected end of list
      hlog_error("Unexpected end of list '%s'", path());
      return -1;
    } else
    if (rc == ListReader::failed) {
      // Unexpected end of list
      hlog_error("Error reading list '%s'", path());
      return -1;
    } else
    if (rc == ListReader::eor) {
      // Normal end of list
      return 0;
    } else
    if (rc == ListReader::got_path) {
      // Found a path
      return 2;
    }
    // Reset status
    resetStatus();
  }
  return -1;
}

int ListReader::getEntry(
    time_t*         timestamp,
    char**          path,
    Node**          node,
    time_t          date) {
  // Initialise
  delete *node;
  *node = NULL;

  bool get_path;
  if (date < 0) {
    get_path = false;
  } else {
    get_path = true;
  }

  int   rc;
  bool  break_it = false;
  while (! break_it) {
    // Get line
    rc = fetchLine();

    if (rc < 0) {
      if (rc == ListReader::eof) {
        hlog_error("Unexpected end of list in '%s'", _d->file.path());
      }
      return -1;
    }

    // End of file
    if (rc == 0) {
      return 0;
    }

    // Path
    if (rc == ListReader::got_path) {
      free(*path);
      *path = strdup(getPath());
      get_path = false;
    } else

    // Data
    if (! get_path) {
      time_t ts;
      // Get all arguments from line
      ListReader::decodeLine(getData(), &ts, getPath(), node);
      if (timestamp != NULL) {
        *timestamp = ts;
      }
      if ((date <= 0) || (ts <= date)) {
        break_it = true;
      }
    }
    // Reset status
    resetStatus();
  }
  return 1;
}

int ListWriter::search(
    ListReader*     list,
    const char*     path_l,
    time_t          expire,
    time_t          remove,
    ListWriter*     new_list,
    ListWriter*     journal,
    bool*           modified) {
  int rc = 0;

  // Need to know whether line of data is active or not
  bool active_data_line = true;   // First line of data for path
  bool stop             = false;  // Searched-for path found or exceeded
  bool path_found       = false;  // Searched-for path found
  bool path_new         = false;  // Current path was found during this search

  while (true) {
    // Read list or get last data
    rc = list->fetchLine();

    // Failed
    if (rc == ListReader::eof) {
      // Unexpected end of list
      hlog_error("Unexpected end of list '%s'", list->path());
      return -1;
    } else
    if (rc == ListReader::failed) {
      // Unexpected end of list
      hlog_error("Error reading list '%s'", list->path());
      return -1;
    }

    // Not end of file
    if (rc != ListReader::eor) {
      if (rc == ListReader::got_path) {
        path_new = true;
        int path_cmp;
        // Got a path
        if (path_l == NULL) {
          // Any path will match (search for next path)
          path_cmp = 0;
        } else
        if (path_l[0] == '\0') {
          // No path will match (search for end of list)
          path_cmp = 1;
        } else
        {
          // Compare paths
          path_cmp = Path::compare(path_l, list->getPath());
        }
        if (path_cmp <= 0) {
          stop = true;
          if (path_cmp == 0) {
            // Looking for path, found
            path_found = true;
          }
        }
        // Next line of data is active
        active_data_line = true;
      } else
      if (rc == ListReader::got_data) {
        // Got data
        if (new_list != NULL) {
          if (active_data_line) {
            // Mark removed
            if ((remove > 0) && path_new) {
              // Find type
              const char* pos = strchr(&list->getData()[2], '\t');
              if (pos == NULL) {
                return -1;
              }
              ++pos;
              // Not marked removed yet
              if (*pos != '-') {
                time_t ts = time(NULL);
                if (new_list->putData(ts, "-", NULL) < 0) {
                  // Could not write
                  return -1;
                }
                if (journal != NULL) {
                  journal->putPath(list->getPath());
                  journal->putData(ts, "-", NULL);
                }
                if (modified != NULL) {
                  *modified = true;
                }
                hlog_info("%-8c%s", 'D', list->getPath());
              }
            }
          } else {
            // Check for expiry
            if (expire >= 0) {
              if (expire != 0) {
                // Check timestamp for expiration
                time_t ts = 0;
                if (! ListReader::decodeLine(list->getData(), &ts) && (ts < expire)) {
                  list->resetStatus();
                  continue;
                }
              } else {
                // Always obsolete
                list->resetStatus();
                continue;
              }
            }
          }
        } // list != NULL
        // Next line of data is not active
        active_data_line = false;
      }
    }

    // New data, add to merge
    if (new_list != NULL) {
      // Searched path found or status is eof/empty
      if ((rc == ListReader::eor) || stop) {
        if (path_found) {
          list->resetStatus();
        }
        if ((path_l != NULL) && (path_l[0] != '\0')) {
          // Write path
          if (new_list->putPath(path_l) < 0) {
            // Could not write
            return -1;
          }
        }
      } else
      // Our data is here or after, so let's copy if required
      if (rc == ListReader::got_path) {
        if (new_list->putPath(list->getPath()) < 0) {
          // Could not write
          return -1;
        }
      } else
      if (rc == ListReader::got_data) {
        if (new_list->putData(list->getData()) < 0) {
          // Could not write
          return -1;
        }
      }
    }

    // Return search status
    if (rc == ListReader::eor) {
      return 0;
    }
    if (path_found && (path_l == NULL)) {
      return 2;
    }
    if (stop && !path_found) {
      return 1;
    }
    // Reset status
    list->resetStatus();
    // Get out, discarding path
    if (path_found) {
      return 2;
    }
  }
  return -1;
}

int ListWriter::copy(
    ListWriter&     final,
    ListReader&     list,
    const char*     path_l) {
  bool copy_till_end = path_l[0] == '\0';
  bool stop = false;  // Searched-for path found or exceeded

  while (true) {
    // Read list or get last data
    int rc = list.fetchLine();

    // Failed
    if (rc == ListReader::eof) {
      // Unexpected end of list
      hlog_error("Unexpected end of list '%s'", list.path());
      return -1;
    } else
    if (rc == ListReader::failed) {
      // Unexpected end of list
      hlog_error("Error reading list '%s'", list.path());
      return -1;
    }

    // Got a path
    if ((rc == ListReader::got_path) && ! copy_till_end) {
      // Compare paths
      int path_cmp = Path::compare(path_l, list.getPath());
      if (path_cmp <= 0) {
        stop = true;
        if (path_cmp == 0) {
          // Looking for path, found
          list.resetStatus();
        }
      }
    }

    // Searched path found or status is eof/empty
    if ((rc == ListReader::eor) || stop) {
      if (! copy_till_end) {
        // Write path
        if (final.putPath(path_l) < 0) {
          // Could not write
          return -1;
        }
      }
      return (rc == ListReader::eor) ? 0 : 1;
    } else
    // Our data is here or after, so let's copy if required
    if (rc == ListReader::got_path) {
      if (final.putPath(list.getPath()) < 0) {
        // Could not write
        return -1;
      }
    } else
    if (rc == ListReader::got_data) {
      if (final.putData(list.getData()) < 0) {
        // Could not write
        return -1;
      }
    }
    // Reset status
    list.resetStatus();
  }
  return -1;
}

int ListWriter::merge(
    ListWriter&     final,
    ListReader&     list,
    ListReader&     journal) {
  int rc_list = 1;

  // Line read from journal
  int j_line_no = 0;

  // Current path and data (from journal)
  Path path;

  // Parse journal
  while (true) {
    int rc_journal = journal.fetchLine();
    j_line_no++;

    // Failed
    if (rc_journal < 0) {
      if (rc_journal == ListReader::eof) {
        hlog_warning("Unexpected end of journal '%s'", journal.path());
        rc_journal = 0;
      } else {
        hlog_error("Reading journal, at line %d", j_line_no);
        return -1;
      }
    }

    // End of file
    if (rc_journal == 0) {
      if (rc_list > 0) {
        rc_list = copy(final, list, "");
        if (rc_list < 0) {
          // Error copying list
          hlog_error("End of list copy failed");
          return -1;
        }
      }
      return 0;
    }

    // Got a path
    if (rc_journal == ListReader::got_path) {
      // Check path order
      if (path.length() != 0) {
        if (path.compare(journal.getPath()) > 0) {
          // Cannot go back
          hlog_error("Journal, line %d: path out of order: '%s' > '%s'",
            j_line_no, path.c_str(), journal.getPath());
          return -1;
        }
      }
      // Copy new path
      path = journal.getPath();
      // Search/copy list and append path if needed
      if (rc_list >= 0) {
        rc_list = copy(final, list, path);
        if (rc_list < 0) {
          // Error copying list
          hlog_error("Path search failed");
          return -1;
        }
      }
    } else

    // Got data
    {
      // Must have a path before then
      if (path.length() == 0) {
        // Did not get anything before data
        hlog_error("Data out of order in journal, at line %d", j_line_no);
        return -1;
      }

      // Write
      if (final.putData(journal.getData()) < 0) {
        // Could not write
        hlog_error("Journal copy failed");
        return -1;
      }
    }
    // Reset status to get next data
    journal.resetStatus();
  }
  return 0;
}

void ListReader::show(
    time_t          date,
    time_t          time_start,
    time_t          time_base) {
  if (! open()) {
    if (end()) {
      printf("Register is empty\n");
    } else {
      time_t ts;
      char*  path = NULL;
      Node*  node = NULL;
      int rc = 0;
      while ((rc = getEntry(&ts, &path, &node, date)) > 0) {
        time_t timestamp = 0;
        if (ts != 0) {
          timestamp = ts - time_start;
          if (time_base > 1) {
            timestamp /= time_base;
          }
        }
        printf("[%2ld] %-30s", timestamp, path);
        if (node != NULL) {
          printf(" %c %8lld %03o", node->type(), node->size(), node->mode());
          if (node->type() == 'f') {
            printf(" %s", static_cast<const File*>(node)->checksum());
          }
          if (node->type() == 'l') {
            printf(" %s", static_cast<const Link*>(node)->link());
          }
        } else {
          printf(" [rm]");
        }
        printf("\n");
      }
      free(path);
      delete node;
      if (rc < 0) {
        hlog_error("Failed to read list");
      }
    }
    close();
  } else {
    hlog_error("%s opening '%s'", strerror(errno), _d->file.path());
  }
}
