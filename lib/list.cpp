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

#include "hbackup.h"
#include "line.h"
#include "files.h"
#include "hreport.h"
#include "list.h"

using namespace hbackup;
using namespace hreport;

class ListFile {
  string _name;
  bool _read_only;
  FILE* _fd;
  bool _old_version;
  const char* _header() { return "# version 5"; }
  const char* _old_header() { return "# version 4\n"; }
  const char* _footer() { return "# end"; }
public:
  ListFile(const char* name, bool ro) : _name(name), _read_only(ro), _fd(NULL) {}
  ~ListFile() { if (_fd != NULL) close(); }
  const char* name() const { return _name.c_str(); }
  int open(bool quiet_if_not_exists = false);
  int close();
  int flush() const {
    return fflush(_fd);
  }
  ssize_t putLine(const char* line);
  // Called only once, leave inline
  ssize_t getLine(Line& line) {
    if (_fd == NULL) return -1;
    if (!_read_only) return -1;
    LineBuffer& buffer = line;
    ssize_t rc;
    char delim;
    if (! _old_version) {
      delim = '\0';
    } else {
      delim = '\n';
    }
    rc = getdelim(buffer.bufferPtr(), buffer.capacityPtr(), delim, _fd);
    if (rc <= 0) {
      return feof(_fd) ? 0 : -1;
    }
    if (line[rc - 1] != delim) {
      return 0;
    }
    if (! _old_version) {
      char lf[1];
      if ((fread(lf, 1, 1, _fd) < 1) || (*lf != '\n')) {
        return 0;
      }
    }
    *buffer.sizePtr() = rc;
    --rc;
    buffer.erase(rc);
    return rc;
  }
};

int ListFile::open(bool quiet_if_not_exists) {
  // Choose mode
  const char* mode;
  if (_read_only) {
    mode = "r";
  } else {
    mode = "w";
  }
  // Open
  _fd = fopen(_name.c_str(), mode);
  if (_fd == NULL) {
    if (! quiet_if_not_exists) {
      hlog_error("%s opening '%s' for %sing",
        strerror(errno), _name.c_str(), _read_only ? "read" : "writ");
    }
    goto failed;
  }
  if (_read_only) {
    bool went_wrong = false;
    char* buffer = NULL;
    size_t capacity;
    if (getline(&buffer, &capacity, _fd) <= 0) {
      const char* error_str;
      if (feof(_fd)) {
        error_str = "EOF reached";
      } else {
        error_str = strerror(errno);
      }
      hlog_error("%s getting version for '%s'", error_str, _name.c_str());
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
        hlog_error("Unknown version for '%s': '%s'", _name.c_str(), buffer);
        went_wrong = true;
      }
    }
    free(buffer);
    if (went_wrong) goto failed;
  } else {
    if (putLine(_header()) < 0) {
      hlog_error("%s writing header in '%s'", strerror(errno), _name.c_str());
      goto failed;
    }
  }
  hlog_regression("opened '%s' for %sing",
    _name.c_str(), _read_only ? "read" : "writ");
  return 0;
failed:
  close();
  return -1;
}

int ListFile::close() {
  int rc;
  if (_fd != NULL) {
    if (! _read_only) {
      if (putLine(_footer()) < 0) {
        hlog_error("%s writing footer in '%s'", strerror(errno), _name.c_str());
        /* the error is reported by fclose */
      }
    }
    rc = fclose(_fd);
    _fd = NULL;
    hlog_regression("closed '%s'", _name.c_str());
  } else {
    errno = EBADF;
    rc = -1;
  }
  return rc;
}

ssize_t ListFile::putLine(const char* line) {
  if (_fd == NULL) {
    hlog_error("'%s' not open", _name.c_str());
    return -1;
  }
  if (_read_only) {
    hlog_error("'%s' read only", _name.c_str());
    return -1;
  }
  size_t line_size = strlen(line);
  if (fwrite(line, line_size, 1, _fd) < 1) {
    hlog_error("'%s' write line", _name.c_str());
    return -1;
  }
  if (fwrite("\0\n", 2, 1, _fd) < 1) {
    hlog_error("'%s' write end of line", _name.c_str());
    return -1;
  }
  return line_size;
}

struct List::Private {
  ListFile          file;
  Status            status;
  Line              data;
  Path              path;
  Private(const char* path) : file(path, true) {}
};

struct ListWriter::Private {
  ListFile          file;
  Private(const char* path) : file(path, false) {}
};

List::List(const Path& path) : _d(new Private(path)) {}

List::~List() {
  delete _d;
}

int List::open(bool quiet_if_not_exists) {
  if (_d->file.open(quiet_if_not_exists) < 0) {
    return -1;
  }
  // Get first line to check for empty list
  return (fetchLine(true) == List::failed) ? -1 : 0;
}

int List::close() {
  return _d->file.close();
}

const char* List::path() const {
  return _d->file.name();
}

const Path& List::getPath() const {
  return _d->path;
}

const Line& List::getData() const {
  return _d->data;
}

ListWriter::ListWriter(const Path& path) : _d(new Private(path)) {}

ListWriter::~ListWriter() {
  delete _d;
}

int ListWriter::open() {
  return _d->file.open();
}

int ListWriter::close() {
  return _d->file.close();
}

int ListWriter::flush() {
  return _d->file.flush();
}

const char* ListWriter::path() const {
  return _d->file.name();
}

List::Status List::fetchLine(bool init) {
  // Check current status
  if (init || (_d->status == List::got_nothing)) {
    // Line contains no re-usable data
    ssize_t length = _d->file.getLine(_d->data);
    // Read error
    if (length < 0) {
      _d->status = List::failed;
    } else
    // Unexpected end of list or incorrect end of line
    if (length == 0) {
      // All lines MUST end in end-of-line character
      _d->status = List::eof;
    } else
    // End of list
    if (_d->data[0] == '#') {
      _d->status = List::eor;
    } else
    // Usable information
    {
      if (init) {
        // Initialise path
        _d->path = "";
      }
      if (_d->data[0] != '\t') {
        if (strcmp(_d->path, _d->data)) {
          _d->path.swap(_d->data);
        } else {
          hlog_warning("Repeated path in list '%s'", _d->path.c_str());
        }
        _d->status = List::got_path;
      } else {
        _d->status = List::got_data;
      }
    }
  }
  return _d->status;
}

void List::resetStatus() {
  if ((_d->status == got_data) || (_d->status == got_path)) {
    _d->status = got_nothing;
  }
}

bool List::end() const {
  // If EOF is reached immediately, list is empty
  return (_d->status == eor) || (_d->status == eof);
}

ssize_t ListWriter::putLine(const char* line) {
  return _d->file.putLine(line);
}

ssize_t List::encodeLine(
    char**          linep,
    time_t          timestamp,
    const Node*     node) {
  int size;

  if (node == NULL) {
    size = asprintf(linep, "\t%ld\t-", timestamp);
  } else {
    const char* extra;
    switch (node->type()) {
      case 'f': {
        extra = (static_cast<const File*>(node))->checksum();
      } break;
      case 'l': {
        extra = (static_cast<const Link*>(node))->link();
      } break;
      default:
        extra = "";
    }
    size = asprintf(linep, "\t%ld\t%c\t%lld\t%ld\t%u\t%u\t%o\t%s",
      timestamp, node->type(), node->size(), node->mtime(), node->uid(),
      node->gid(), node->mode(), extra);
    // Remove trailing tab
    if (extra[0] == '\0') {
      (*linep)[--size] = '\0';
    }
  }
  return size;
}

int List::decodeLine(
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

int ListWriter::add(
    const Path&     path,
    const Node*     node,
    ListWriter*     journal) {
  int rc = 0;
  char* line = NULL;
  ssize_t size = List::encodeLine(&line, time(NULL), node);

  if (putLine(line) != size) {
    rc = -1;
  }
  if (journal != NULL) {
    // Add to journal
    if ((journal->putLine(path) != static_cast<ssize_t>(path.size())) ||
        (journal->putLine(line) != size)) {
      rc = -1;
    }
  }
  free(line);
  return rc;
}

void List::setProgressCallback(progress_f progress) {
  (void) progress;
//   _d->stream.setProgressCallback(progress);
}

int List::getEntry(
    time_t*         timestamp,
    char**          path,
    Node**          node,
    time_t          date) {
  // Initialise
  if (node != NULL) {
    delete *node;
    *node = NULL;
  }

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
      if (rc == List::eof) {
        hlog_error("Unexpected end of list in '%s'", _d->file.name());
      }
      return -1;
    }

    // End of file
    if (rc == 0) {
      return 0;
    }

    // Path
    if (rc == List::got_path) {
      if (path != NULL) {
        free(*path);
        *path = strdup(getPath());
      }
      get_path = false;
    } else

    // Data
    if (! get_path) {
      time_t ts;
      if ((node != NULL) || (timestamp != NULL) || (date > 0)) {
        // Get all arguments from line
        List::decodeLine(getData(), &ts, getPath(), node);
        if (timestamp != NULL) {
          *timestamp = ts;
        }
      }
      if ((date <= 0) || (ts <= date)) {
        break_it = true;
      }
    }
    // Reset status
    if (date != -2) {
      resetStatus();
    }
  }
  return 1;
}

int ListWriter::search(
    List*           list,
    const char*     path_l,
    time_t          expire,
    time_t          remove,
    ListWriter*     new_list,
    ListWriter*     journal,
    bool*           modified) {
  int rc = 0;

  int path_cmp;
  // Pre-set path comparison result
  if (path_l == NULL) {
    // Any path will match (search for next path)
    path_cmp = 0;
  } else
  if (path_l[0] == '\0') {
    // No path will match (search for end of list)
    path_cmp = 1;
  } else {
    // Only given path will match (search for given path)
    path_cmp = -1;
  }

  // Need to know whether line of data is active or not
  bool active_data_line = true;   // First line of data for path
  bool stop             = false;  // Searched-for path found or exceeded
  bool path_found       = false;  // Searched-for path found
  bool path_new         = false;  // Current path was found during this search

  while (true) {
    // Read list or get last data
    rc = list->fetchLine();

    // Failed
    if (rc < 0) {
      // Unexpected end of list
      hlog_error("Unexpected end of list '%s'", list->path());
      return -1;
    }

    // Not end of file
    if (rc != 0) {
      if (rc == List::got_path) {
        path_new = true;
        // Got a path
        if ((path_l != NULL) && (path_l[0] != '\0')) {
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
      if (rc == List::got_data) {
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
                char* line = NULL;
                List::encodeLine(&line, remove, NULL);
                if (new_list->putLine(line) < 0) {
                  // Could not write
                  free(line);
                  return -1;
                }
                if (journal != NULL) {
                  journal->putLine(list->getPath());
                  journal->putLine(line);
                }
                free(line);
                if (modified != NULL) {
                  *modified = true;
                }
                hlog_info("%-8c%s", 'D', list->getPath().c_str());
              }
            }
          } else {
            // Check for expiry
            if (expire >= 0) {
              if (expire != 0) {
                // Check timestamp for expiration
                time_t ts = 0;
                if (! List::decodeLine(list->getData(), &ts) && (ts < expire)) {
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
      if ((rc == List::eor) || stop) {
        if (path_found) {
          list->resetStatus();
        }
        if ((path_l != NULL) && (path_l[0] != '\0')) {
          // Write path
          if (new_list->putLine(path_l) < 0) {
            // Could not write
            return -1;
          }
        }
      } else
      // Our data is here or after, so let's copy if required
      if (rc == List::got_path) {
        if (new_list->putLine(list->getPath()) < 0) {
          // Could not write
          return -1;
        }
      } else
      if (rc == List::got_data) {
        if (new_list->putLine(list->getData()) < 0) {
          // Could not write
          return -1;
        }
      }
    }

    // Return search status
    if (rc == List::eor) {
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

int ListWriter::merge(
    List*           list,
    List*           journal) {
  int rc_list = 1;

  // Line read from journal
  int j_line_no = 0;

  // Current path and data (from journal)
  Path path;

  // Parse journal
  while (true) {
    int rc_journal = journal->fetchLine();
    j_line_no++;

    // Failed
    if (rc_journal < 0) {
      if (rc_journal == List::eof) {
        hlog_warning("Unexpected end of journal '%s'", journal->path());
        rc_journal = 0;
      } else {
        hlog_error("Reading journal, at line %d", j_line_no);
        return -1;
      }
    }

    // End of file
    if (rc_journal == 0) {
      if (rc_list > 0) {
        rc_list = search(list, "", -1, 0, this);
        if (rc_list < 0) {
          // Error copying list
          hlog_error("End of list copy failed");
          return -1;
        }
      }
      return 0;
    }

    // Got a path
    if (rc_journal == List::got_path) {
      // Check path order
      if (path.length() != 0) {
        if (path.compare(journal->getPath()) > 0) {
          // Cannot go back
          hlog_error("Journal, line %d: path out of order: '%s' > '%s'",
            j_line_no, path.c_str(), journal->getPath().c_str());
          return -1;
        }
      }
      // Copy new path
      path = journal->getPath();
      // Search/copy list and append path if needed
      if (rc_list >= 0) {
        rc_list = search(list, path, -1, 0, this);
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
      if (putLine(journal->getData()) < 0) {
        // Could not write
        hlog_error("Journal copy failed");
        return -1;
      }
    }
    // Reset status to get next data
    journal->resetStatus();
  }
  return 0;
}

void List::show(
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
      while ((rc = List::getEntry(&ts, &path, &node, date)) > 0) {
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
      free(node);
      if (rc < 0) {
        hlog_error("Failed to read list");
      }
    }
    close();
  } else {
    hlog_error("%s opening '%s'", strerror(errno), _d->file.name());
  }
}
