/*
     Copyright (C) 2007-2008  Herve Fache

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

#include <errno.h>

#include "hbackup.h"
#include "line.h"
#include "files.h"
#include "report.h"
#include "list.h"

using namespace hbackup;

namespace hbackup {

enum Status {
  // Order matters as we may check for (status < ok) or (status >= eof)
  failed = -1,              // an error occured
  got_nothing,              // No relevant data
  got_path,                 // Got a path
  got_data,                 // Got data
  eof,                      // end of file reached
  empty                     // list is empty
};

}

struct Register::Private {
  Stream            stream;
  Status            status;
  Line              data;
  Path              path;
  // Set when a previous version is detected
  bool              old_version;
  bool              modified;
  List*             new_list;
  List*             journal;
  Private(const char* path) : stream(path) {}
};

ssize_t Register::fetchLine() {
  // Check current status
  switch (_d->status) {
    case failed:
      return -1;
    case got_nothing:
      break;
    case got_path:
      return _d->path.size();
    case got_data:
      return _d->data.size();
    case empty:
    case eof:
      return 0;
  }
  // Line contains no re-usable data
  bool    eol;
  ssize_t length = _d->stream.getLine(_d->data, &eol);
  // Read error
  if (length < 0) {
    _d->status = failed;
  } else
  // Unexpected end of list
  if (length == 0) {
    _d->status = eof;
  } else
  // Incorrect end of line
  if (! eol) {
    // All lines MUST end in end-of-line character
    _d->status = eof;
  } else
  // End of list
  if (_d->data[0] == '#') {
    _d->status = eof;
    return 0;
  } else
  // Usable information
  {
    if (_d->data[0] != '\t') {
      if (strcmp(_d->path, _d->data)) {
        _d->path.swap(_d->data);
      } else {
        out(warning, msg_standard, "Repeated path in list", -1, _d->path);
      }
      _d->status = got_path;
    } else {
      _d->status = got_data;
    }
    return length;
  }
  return -1;
}

Register::Register(Path path) : _d(new Private(path)) {}

Register::~Register() {
  delete _d;
}

int Register::open(List* new_list, List* journal) {
  // Put same header in both until a new version becomes necessary
  const char header[]     = "# version 4";
  const char old_header[] = "# version 4";

  _d->modified = false;
  if (_d->stream.open(O_RDONLY, 0, false)) {
    return -1;
  }
  // Check rights
  if (_d->stream.getLine(_d->data) < 0) {
    _d->stream.close();
    return -1;
  }
  int rc = 0;
  // Expected header?
  if (strcmp(_d->data, header) == 0) {
    _d->old_version = false;
  } else
  // Old header?
  if (strcmp(_d->data, old_header) == 0) {
    _d->old_version = true;
  } else
  // Unknown header
  {
    rc = -1;
  }
  // Initialise status and path
  _d->status   = got_nothing;
  _d->path     = "";
  _d->new_list = new_list;
  _d->journal  = journal;
  // Get first line to check for empty list
  fetchLine();
  if (_d->status == failed) {
    rc = -1;
  } else
  if (_d->status == eof) {
      // EOF reached immediately, list is empty
    _d->status = empty;
  }
  return rc;
}

int Register::close() {
  return _d->stream.close();
}

const char* Register::path() const {
  return _d->stream.path();
}

void Register::setProgressCallback(progress_f progress) {
  _d->stream.setProgressCallback(progress);
}

bool Register::isEmpty() const {
  return _d->status == empty;
}

bool Register::isModified() const {
  return _d->modified;
}

int Register::getEntry(
    time_t*       timestamp,
    char**        path,
    Node**        node,
    time_t        date) {
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

  ssize_t length;
  bool    break_it = false;
  while (! break_it) {
    // Get line
    length = fetchLine();

    if (length < 0) {
      if (_d->status == eof) {
        out(error, msg_standard, "Unexpected end of list", -1,
          _d->stream.path());
      }
      return -1;
    }

    // End of file
    if (length == 0) {
      return 0;
    }

    // Path
    if (_d->status == got_path) {
      if (path != NULL) {
        free(*path);
        *path = strdup(_d->path);
      }
      get_path = false;
    } else

    // Data
    if (! get_path) {
      time_t ts;
      if ((node != NULL) || (timestamp != NULL) || (date > 0)) {
        // Get all arguments from line
        decodeLine(_d->data, &ts, _d->path, node);
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
      _d->status = got_nothing;
    }
  }
  return 1;
}

int Register::search(
    const char*     path_l,
    time_t          expire,
    time_t          remove) {
  int     path_cmp;
  int     rc         = 0;

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
    rc = fetchLine();

    // Failed
    if (rc < 0) {
      // Unexpected end of list
      out(error, msg_standard, "Unexpected end of list", -1,
        _d->stream.path());
      return -1;
    }

    // Not end of file
    if (rc != 0) {
      if (_d->status == got_path) {
        path_new = true;
        // Got a path
        if ((path_l != NULL) && (path_l[0] != '\0')) {
          // Compare paths
          path_cmp = Path::compare(path_l, _d->path);
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
      if (_d->status == got_data) {
        // Got data
        if (_d->new_list != NULL) {
          if (active_data_line) {
            // Mark removed
            if ((remove > 0) && path_new) {
              // Find type
              const char* pos = strchr(&_d->data[2], '\t');
              if (pos == NULL) {
                return -1;
              }
              ++pos;
              // Not marked removed yet
              if (*pos != '-') {
                char* line = NULL;
                encodeLine(&line, remove, NULL);
                if (putLine(_d->new_list->_stream, line) < 0) {
                  // Could not write
                  free(line);
                  return -1;
                }
                if (_d->journal != NULL) {
                  putLine(_d->journal->_stream, _d->path);
                  putLine(_d->journal->_stream, line);
                }
                free(line);
                _d->modified = true;
                out(info, msg_standard, _d->path, -2, "D      ");
              }
            }
          } else {
            // Check for expiry
            if (expire >= 0) {
              if (expire != 0) {
                // Check timestamp for expiration
                time_t ts = 0;
                if (! decodeLine(_d->data, &ts) && (ts < expire)) {
                  _d->status = got_nothing;
                  continue;
                }
              } else {
                // Always obsolete
                _d->status = got_nothing;
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
    if (_d->new_list != NULL) {
      // Searched path found or status is eof/empty
      if ((_d->status == eof) || (_d->status == empty) || stop) {
        if (path_found) {
          _d->status = got_nothing;
        }
        if ((path_l != NULL) && (path_l[0] != '\0')) {
          // Write path
          if (putLine(_d->new_list->_stream, path_l) < 0) {
            // Could not write
            return -1;
          }
        }
      } else
      // Our data is here or after, so let's copy if required
      if (_d->status == got_path) {
        if (putLine(_d->new_list->_stream, _d->path) < 0) {
          // Could not write
          return -1;
        }
      } else
      if (_d->status == got_data) {
        if (putLine(_d->new_list->_stream, _d->data) < 0) {
          // Could not write
          return -1;
        }
      }
    }

    // Return search status
    if (_d->status >= eof) {
      return 0;
    }
    if (path_found && (path_l == NULL)) {
      return 2;
    }
    if (stop && !path_found) {
      return 1;
    }
    // Reset status
    if ((_d->status == got_data) || (_d->status == got_path)) {
      _d->status = got_nothing;
    }
    // Get out, discarding path
    if (path_found) {
      return 2;
    }
  }
  return -1;
}

int Register::add(
    const Path&     path,
    const Node*     node) {
  int rc = 0;
  if (_d->new_list != NULL) {
    char* line = NULL;
    int   size = encodeLine(&line, time(NULL), node);

    if (putLine(_d->new_list->_stream, line) != size) {
      rc = -1;
    }
    if (_d->journal != NULL) {
      // Add to journal
      if ((putLine(_d->journal->_stream, path) !=
            static_cast<ssize_t>(path.size())) ||
          (putLine(_d->journal->_stream, line) != size)) {
        rc = -1;
      }
    }
    free(line);
    _d->modified = true;
  }
  return rc;
}

void Register::show(
    time_t          date,
    time_t          time_start,
    time_t          time_base) {
  if (! open()) {
    if (isEmpty()) {
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
      free(node);
      if (rc < 0) {
        out(error, msg_standard, "Failed to read list");
      }
    }
    _d->stream.close();
  } else {
    out(error, msg_errno, "opening list", errno, path());
  }
}

int Register::encodeLine(
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

int Register::decodeLine(
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
    out(error, msg_standard, "Wrong number of arguments for line");
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

ssize_t Register::putLine(int fd, const Line& line) {
  size_t length = line.size();
  while (length > 0) {
    ssize_t size = write(fd, &line[line.size() - length], length);
    if (size < 0) return -1;
    length -= size;
  }
  if (write(fd, "\n", 1) < 1) {
    return -1;
  }
  return line.size();
}

int List::create() {
  const char header[] = "# version 4";

  _stream = creat(_path, 0644);
  if (_stream < 0) {
    return -1;
  }
  if (Register::putLine(_stream, header) < 0) {
    close(_stream);
    return -1;
  }
  return 0;
}

int List::finalize() {
  const char footer[] = "# end";
  int rc = 0;

  if (Register::putLine(_stream, footer) < 0) {
    out(error, msg_errno, "writing list footer", errno, _path);
    rc = -1;
  }
  if (close(_stream)) {
    out(error, msg_errno, "closing list", errno, _path);
    rc = -1;
  }
  return rc;
}

int List::merge(
    Register&     list,
    Register&     journal) {
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
      if (journal._d->status == eof) {
        out(warning, msg_standard, "Unexpected end of journal", -1,
          journal.path());
        rc_journal = 0;
      } else {
        out(error, msg_number, "Reading line", j_line_no, "journal");
        return -1;
      }
    }

    // End of file
    if (rc_journal == 0) {
      if (rc_list > 0) {
        rc_list = list.search("");
        if (rc_list < 0) {
          // Error copying list
          out(error, msg_standard, "End of list copy failed");
          return -1;
        }
      }
      return 0;
    }

    // Got a path
    if (journal._d->status == got_path) {
      // Check path order
      if (path.length() != 0) {
        if (path.compare(journal._d->path) > 0) {
          // Cannot go back
          out(error, msg_number, "Path out of order", j_line_no, "journal");
          return -1;
        }
      }
      // Copy new path
      path = journal._d->path;
      // Search/copy list and append path if needed
      if (rc_list >= 0) {
        rc_list = list.search(path);
        if (rc_list < 0) {
          // Error copying list
          out(error, msg_standard, "Path search failed");
          return -1;
        }
      }
    } else

    // Got data
    {
      // Must have a path before then
      if (path.length() == 0) {
        // Did not get anything before data
        out(error, msg_number, "Data out of order", j_line_no, "journal");
        return -1;
      }

      // Write
      if (Register::putLine(_stream, journal._d->data) < 0) {
        // Could not write
        out(error, msg_standard, "Journal copy failed");
        return -1;
      }
    }
    // Reset status to get next data
    journal._d->status = got_nothing;
  }
  return 0;
}

int List::addLine(
    const char      line[]) {
  if (Register::putLine(_stream, line) != static_cast<ssize_t>(strlen(line) + 1)) {
    return -1;
  }
  return 0;
}
