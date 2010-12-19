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
#include "files.h"
#include "report.h"
#include "list.h"

using namespace hbackup;
using namespace htools;

int List::open(bool quiet_if_not_exists) {
  // Choose mode
  const char* mode;
  if (_type == list_read) {
    mode = "r";
  } else {
    mode = "w";
  }
  // Open
  _fd = fopen(_path, mode);
  if (_fd == NULL) {
    if (! quiet_if_not_exists) {
      hlog_error("%s opening '%s' for %sing",
        strerror(errno), _path, _type == list_read ? "read" : "writ");
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
      hlog_error("%s getting version for '%s'", error_str, _path);
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
        hlog_error("Unknown version for '%s': '%s'", _path, buffer);
        went_wrong = true;
      }
    }
    free(buffer);
    if (went_wrong) goto failed;
  } else {
    if (putLine(_header()) < 0) {
      hlog_error("%s writing header in '%s'", strerror(errno), _path);
      goto failed;
    }
  }
  hlog_regression("opened '%s' for %sing",
    _path, _type == list_read ? "read" : "writ");
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
        hlog_error("%s writing footer in '%s'", strerror(errno), _path);
        /* the error is reported by fclose */
      }
    }
    rc = fclose(_fd);
    _fd = NULL;
    hlog_regression("closed '%s'", _path);
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
    if (stat64(_path, &stat) < 0) {
      return;
    }
    _size = stat.st_size;
  }
  _progress = progress;
}

ssize_t List::putLine(const char* line) {
  if (_fd == NULL) {
    hlog_error("'%s' not open", _path);
    return -1;
  }
  if (_type == list_read) {
    hlog_error("'%s' read only", _path);
    return -1;
  }
  ssize_t line_size = strlen(line);
  if (line_size > 0) {
    if (fwrite(line, line_size, 1, _fd) < 1) {
      hlog_error("%s writing line to '%s'", strerror(errno), _path);
      return -1;
    }
  }
  if (fwrite("\0\n", 2, 1, _fd) < 1) {
    hlog_error("%s writing line ending to '%s'", strerror(errno), _path);
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
      hlog_error("%s flushing file path to '%s'", strerror(errno), _path);
      return -1;
    }
    hlog_regression("DBG %s flush _file_path = %s into %s", __FUNCTION__,
      _file_path, _path);
    _file_path[0] = '\0';
  }
  // Write removed mark if relevant
  if (_removed > 0) {
    char line[32];
    sprintf(line, "\t%ld\t-", _removed);
    ssize_t ts_size = putLine(line);
    if (ts_size < 1) {
      hlog_error("%s flushing line to '%s'", strerror(errno), _path);
      return -1;
    }
    hlog_regression("DBG %s flush _removed = %ld into %s", __FUNCTION__,
      _removed, _path);
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
      _file_path, _path);
  } else {
    return putLine(path);
  }
  return 0;
}

ssize_t List::putData(const char* ts_metadata_extra) {
  ssize_t line_size = 0;
  time_t ts = 0;
  if (_type != journal_write) {
    char type;
    if ((decodeType(ts_metadata_extra, &type) < 0) || (type != '-')) {
      ts = 0;
    } else {
      decodeTimeStamp(ts_metadata_extra, &ts);
    }
    // So it _is_ a removed mark
    if (ts != 0) {
      _removed = ts;
      hlog_regression("DBG %s1 store _removed = %ld into %s", __FUNCTION__,
        _removed, _path);
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
      _path);
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
        _removed, _path);
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
    hlog_error("%s write ts_str to '%s'", strerror(errno), _path);
    return -1;
  }
  line_size += metadata_size;
  if (extra != NULL) {
    size_t extra_size = fprintf(_fd, "\t%s", extra);
    if (extra_size < 1) {
      hlog_error("%s write separator to '%s'", strerror(errno), _path);
      return -1;
    }
    line_size += extra_size;
  }
  if (fwrite("\0\n", 2, 1, _fd) < 1) {
    hlog_error("%s write end of line to '%s'", strerror(errno), _path);
    return -1;
  }
  return line_size;
}

ssize_t List::getLine(char** buffer_p, size_t* cap_p) {
  int delim = _old_version ? '\n' : '\0';
  size_t cap = *cap_p;
  ssize_t rc = getdelim(buffer_p, cap_p, delim, _fd);
  if (*cap_p != cap) {
    hlog_error("unexpected buffer capacity change from %zd to %zd",
      cap, *cap_p);
    return -3;
  }
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
    // Max size for path line:
    // * path             4096
    // * ending              2
    // TOTAL              4098
    // Max size for data line:
    // * time stamp         10
    // * type                1
    // * size               20
    // * mtime              10
    // * uid                10
    // * gid                10
    // * mode                4
    // * path/checksum    4096
    // * field separators    8
    // * ending              2
    // TOTAL              4171
    data_cap = path_cap = 4352;
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

ListReader::ListReader(const char* path) : _d(new Private(path)) {}

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

size_t List::encode(
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

int List::decodeLine(
    const char*     line,
    time_t*         ts,
    Node**          node_p,
    const char*     path) {
  // Fields
  char        type;             // file type
  long long   size;             // on-disk size, in bytes
  time_t      mtime;            // time of last modification
  uid_t       uid;              // user ID of owner
  gid_t       gid;              // group ID of owner
  mode_t      mode;             // permissions
  char        extra[PATH_MAX];  // linked file or checksum

  int num = sscanf(line, "\t%ld\t%c\t%Ld\t%ld\t%d\t%d\t%o\t%[^\t]",
    ts, &type, &size, &mtime, &uid, &gid, &mode, extra);
  // Check number of params
  if (num < 0) {
    return -1;
  }
  if (num < 2) {
    hlog_error("Wrong number of arguments for line");
    return -1;
  }
  if (num < 8) {
    extra[0] = '\0';
  }

  // Return extracted data
  switch (type) {
  case '-':
    *node_p = NULL;
    break;
  default:
    *node_p = new Node(path, type, mtime, size, uid, gid, mode, extra);
  }
  return 0;
}

int List::decodeTimeStamp(
    const char*     line,
    time_t*         ts) {
  if (sscanf(line, "\t%ld\t", ts) != 1) {
    *ts = 0;
    return -1;
  }
  return 0;
}

int List::decodeType(
    const char*     line,
    char*           type) {
  const char* pos = strchr(&line[1], '\t');
  if (pos == NULL) {
    *type = '?';
    return -1;
  }
  *type = *++pos;
  return 0;
}

int List::decodeSizeChecksum(
    const char*     line,
    long long*      size_p,
    char            checksum[MAX_CHECKSUM_LENGTH + 1]) {
  checksum[0] = '\0';
  const char* field = &line[1];
  size_t field_no = 0;
  while ((field = strchr(field, '\t')) != NULL) {
    ++field_no;
    ++field;
    switch (field_no) {
      case 1: if (field[0] != 'f') return 1;
        break;
      case 2: if (sscanf(field, "%Ld\t", size_p) != 1) return -1;
        break;
      case 7:
        strncpy(checksum, field, MAX_CHECKSUM_LENGTH + 1);
        checksum[MAX_CHECKSUM_LENGTH] = '\0';
        return 0;
    }
  }
  return 1;
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

int ListReader::fetchSizeChecksum(
    long long*      size_p,
    char            checksum[List::MAX_CHECKSUM_LENGTH + 1]) {
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
      // Get all arguments from line
      if (List::decodeSizeChecksum(getData(), size_p, checksum) < 0) {
        hlog_error("failed to decode line");
        return -1;
      }
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
    char*           path,
    Node**          node_p,
    time_t          date) {
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
      strcpy(path, getPath());
      get_path = false;
    } else

    // Data
    if (! get_path) {
      time_t ts;
      // Get all arguments from line
      if (List::decodeLine(getData(), &ts, node_p, getPath()) < 0) {
        hlog_error("failed to decode line");
        return -1;
      }
      if (timestamp != NULL) {
        *timestamp = ts;
      }
      if ((date <= 0) || (ts <= date)) {
        break_it = true;
      } else {
        delete *node_p;
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
    ListWriter*     journal,
    bool*           modified) {
  int rc = 0;

  // Need to know whether line of data is active or not
  bool stop                = false; // Searched-for path found or exceeded
  bool path_found          = false; // Searched-for path found
  bool mark_path_gone      = false; // Current path has gone => mark removed
  bool lines_below_expired = false; // Found last not-expired line

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
        lines_below_expired = false;
        mark_path_gone = true;
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
      } else
      if (rc == ListReader::got_data) {
        // Got data
        if (lines_below_expired) {
          // Always obsolete
          list->resetStatus();
          continue;
        } else
        if (expire == 0) {
          lines_below_expired = true;
        } else
        if (expire > 0) {
          // Check for expiry
          time_t data_ts = 0;
          if (List::decodeTimeStamp(list->getData(), &data_ts) == 0) {
            // First line that expired => last that we keep
            if (data_ts <= expire) {
              lines_below_expired = true;
            }
          }
        }
        if (mark_path_gone) {
          // Mark removed if path gone
          if (remove > 0) {
            // Find type
            char data_type;
            if (decodeType(list->getData(), &data_type) < 0) {
              return -1;
            }
            // Not marked removed yet
            if (data_type != '-') {
              time_t ts = time(NULL);
              if (putData(ts, "-", NULL) < 0) {
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
          mark_path_gone = false;
        }
      }
    }

    // New data, add to merge
    if ((rc == ListReader::eor) || stop) {
      // Searched path found or status is eof/empty
      if (path_found) {
        list->resetStatus();
      }
      if ((path_l != NULL) && (path_l[0] != '\0')) {
        // Write path
        if (putPath(path_l) < 0) {
          // Could not write
          return -1;
        }
      }
    } else
    if (rc == ListReader::got_path) {
      // Our data is here or after, so let's copy if required
      if (putPath(list->getPath()) < 0) {
        // Could not write
        return -1;
      }
    } else
    if (rc == ListReader::got_data) {
      if (putData(list->getData()) < 0) {
        // Could not write
        return -1;
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
  char path[PATH_MAX] = "";

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
      if (path[0] != '\0') {
        if (Path::compare(path, journal.getPath()) > 0) {
          // Cannot go back
          hlog_error("Journal, line %d: path out of order: '%s' > '%s'",
            j_line_no, path, journal.getPath());
          return -1;
        }
      }
      // Copy new path
      strcpy(path, journal.getPath());
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
      if (path[0] == '\0') {
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
      char path[PATH_MAX];
      Node* node;
      int rc = 0;
      while ((rc = getEntry(&ts, path, &node, date)) > 0) {
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
            printf(" %s", node->hash());
          }
          if (node->type() == 'l') {
            printf(" %s", node->link());
          }
          delete node;
        } else {
          printf(" [rm]");
        }
        printf("\n");
      }
      if (rc < 0) {
        hlog_error("Failed to read list");
      }
    }
    close();
  } else {
    hlog_error("%s opening '%s'", strerror(errno), _d->file.path());
  }
}