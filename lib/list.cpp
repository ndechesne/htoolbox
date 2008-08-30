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

#include <sstream>
#include <list>

#include <string.h>
#include <errno.h>

using namespace std;

#include "hbackup.h"
#include "line.h"
#include "files.h"
#include "report.h"
#include "list.h"

using namespace hbackup;

namespace hbackup {

enum Status {
  // Order matters as we may check for (status < ok) or (status >= eof)
  _error = -1,              // an error occured
  ok,                       // nothing to report
  eof,                      // end of file reached
  empty                     // list is empty
};

enum LineStatus {
  no_data,                  // line contains no valid data
  cached_data,              // line contains searched-for data
  new_data                  // line contains new data
};

}

struct List::Private {
  Status            status;
  Line              line;
  LineStatus        line_status;
  // Set when a previous version is detected
  bool              old_version;
};

List::List(Path path) : Stream(path), _d(new Private) {}

List::~List() {
  delete _d;
}

int List::open(
    const char*     req_mode,
    int             compression,
    bool            checksum) {
  // Put future as old for now (testing stage)
  const char old_header[] = "# version 3";
  const char header[]     = "# version 4";
  int        rc           = 0;

  if (Stream::open(req_mode, (compression > 0) ? compression : 0,
      (compression < 0) ? 0 : -1, checksum)) {
    rc = -1;
  } else
  // Open for write
  if (isWriteable()) {
    if (Stream::putLine(header) < 0) {
      Stream::close();
      rc = -1;
    }
    _d->status      = empty;
    _d->line_status = no_data;
  } else
  // Open for read
  {
    // Check rights
    if (Stream::getLine(_d->line.instance()) < 0) {
      Stream::close();
      rc = -1;
    } else
    // Expected header?
    if (strcmp(_d->line, header) == 0) {
      _d->old_version = false;
    } else
    // Old header?
    if (strcmp(_d->line, old_header) == 0) {
      _d->old_version = true;
    } else
    // Unknown header
    {
      rc = -1;
    }
    _d->status      = ok;
    _d->line_status = no_data;
    // Get first line to check for empty list
    fetchLine();
    if (_d->status == _error) {
      rc = -1;
    } else
    if (_d->status == eof) {
      _d->status = empty;
    } else
    {
      // Re-use data
      _d->line_status = new_data;
    }
  }
  return rc;
}

int List::close() {
  const char footer[] = "# end\n";
  int rc = 0;

  if (isWriteable()) {
    if (Stream::write(footer, strlen(footer)) < 0) {
      out(error, msg_errno, "writing list footer", errno, path());
      rc = -1;
    }
  }
  if (Stream::close()) {
    out(error, msg_errno, "closing list", errno, path());
    rc = -1;
  }
  return rc;
}

bool List::isOldVersion() const {
  return _d->old_version;
}

bool List::isEmpty() const {
  return _d->status == empty;
}

ssize_t List::fetchLine(bool use_found) {
  // Check current status
  switch (_d->status) {
    case ok:
      break;
    case _error:
      return -1;
    case empty:
    case eof:
      return 0;
  }
  ssize_t rc = -1;
  if ((_d->line_status == new_data)
  || ((_d->line_status == cached_data) && use_found)) {
    // Line contains data to be re-used
    rc = _d->line.size();
  } else {
    // Line contains no re-usable data
    bool    eol;
    ssize_t length = Stream::getLine(_d->line.instance(), &eol);
    // Read error
    if (length < 0) {
      _d->status = _error;
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
    if (_d->line[0] == '#') {
      _d->status = eof;
      rc = 0;
    } else {
      rc = length;
    }
  }
  // Preventively mark data as used
  _d->line_status = no_data;
  return rc;
}

// Insert line into buffer
ssize_t List::putLine(const char* line) {
  if ((_d->line_status == no_data) || (_d->line_status == cached_data)) {
    _d->line        = line;
    _d->line_status = new_data;
    // Remove empty status
    _d->status      = ok;
    return 0;
  }
  return -1;
}

void List::keepLine() {
  _d->line_status = new_data;
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
    out(error, msg_standard, "Wrong number of arguments for line");
    return -1;
  }

  // Return extracted data
  if (node != NULL) {
    switch (type) {
    case 'f':
      *node = new File(path, type, mtime, size, uid, gid, mode, extra ? extra : "");
      break;
    case 'l':
      *node = new Link(path, type, mtime, size, uid, gid, mode, extra ? extra : "");
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

char List::getLineType() {
  fetchLine();
  // Status is one of -2: eof!, -1: error, 0: eof, 1: ok
  switch (_d->status) {
    case ok:
      break;
    case eof:
    case empty:
      // Eof
      return 'E';
    case _error:
      // Failure
      return 'F';
  }
  // Line should be re-used
  _d->line_status = new_data;
  if (_d->line[0] != '\t') {
    // Path
    return 'P';
  } else
  {
    const char* pos = strchr(&_d->line[2], '\t');
    if (pos == NULL) {
      // Type unaccessible
      return 'T';
    } else
    {
      pos++;
      return *pos;
    }
  }
}

int List::getEntry(
    time_t*       timestamp,
    char**        path,
    Node**        node,
    time_t        date) {
  // Initialise
  if (node != NULL) {
    delete *node;
    *node = NULL;
  }

  bool    got_path;
  if (date < 0) {
    got_path = true;
  } else {
    got_path = false;
  }

  ssize_t length;
  while (true) {
    // Get line
    if (date == -2) {
      length = _d->line.size();
    } else {
      length = fetchLine(true);
    }

    if (length < 0) {
      if (_d->status == eof) {
        out(error, msg_standard, "Unexpected end of list", -1, this->path());
      }
      return -1;
    }

    // End of file
    if (length == 0) {
      return 0;
    }

    // Path
    if (_d->line[0] != '\t') {
      if (path != NULL) {
        free(*path);
        *path = strdup(_d->line);
      }
      got_path = true;
    } else

    // Data
    if (got_path) {
      time_t ts;
      if ((node != NULL) || (timestamp != NULL) || (date > 0)) {
        // Get all arguments from line
        decodeLine(_d->line, &ts, path == NULL ? "" : *path, node);
        if (timestamp != NULL) {
          *timestamp = ts;
        }
      }
      if ((date <= 0) || (ts <= date)) {
        break;
      }
    }
    if (date == -2) {
      break;
    }
  }
  return 1;
}

int List::add(
    const char*     path,
    time_t          timestamp,
    const Node*     node,
    bool            bufferize) {
  if (path != NULL) {
    if (Stream::putLine(path) != static_cast<ssize_t>(strlen(path) + 1)) {
      return -1;
    }
    // Remove empty status
    _d->status = ok;
  }
  if (timestamp < 0) {
    return 0;
  }
  char* line = NULL;
  int   size;
  int   rc   = 0;

  if (node == NULL) {
    size = asprintf(&line, "\t%ld\t-", timestamp);
  } else {
    char* temp = NULL;
    size = asprintf(&temp, "\t%ld\t%c\t%lld\t%ld\t%u\t%u\t%o",
      timestamp, node->type(), node->size(), node->mtime(), node->uid(),
      node->gid(), node->mode());
    switch (node->type()) {
      case 'f': {
        const char* extra  = (static_cast<const File*>(node))->checksum();
        size = asprintf(&line, "%s\t%s", temp, extra);
      } break;
      case 'l': {
        const char* extra  = (static_cast<const Link*>(node))->link();
        size = asprintf(&line, "%s\t%s", temp, extra);
      } break;
      default:
        size = asprintf(&line, "%s", temp);
    }
    free(temp);
  }
  if ((timestamp == 0) && bufferize) {
    // Send line to search method, so it can deal with exception
    putLine(line);
  } else
  if (Stream::putLine(line) != (size + 1)) {
    rc = -1;
  } else {
    // Remove empty status
    _d->status = ok;
  }
  free(line);
  return rc;
}

int List::search(
    const char*     path_l,
    List*           list,
    time_t          expire) {
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
  bool active_data_line = true;

  while (true) {
    // Read list or get last data
    rc = fetchLine();

    // Failed
    if (rc < 0) {
      // Unexpected end of list
      out(error, msg_standard, "Unexpected end of list", -1, path());
      return -1;
    }

    // Not end of file
    if (rc != 0) {
      if (_d->line[0] != '\t') {
        // Got a path
        if ((path_l != NULL) && (path_l[0] != '\0')) {
          // Compare paths
          path_cmp = Path::compare(path_l, _d->line);
        }
        if (path_cmp <= 0) {
          if (path_cmp < 0) {
            // Path exceeded
            _d->line_status = new_data;
          } else {
            // Looking for path, found
            _d->line_status = cached_data;
          }
        }
        // Next line of data is active
        active_data_line = true;
      } else {
        // Got data
        if (list != NULL) {
          // Deal with exception
          if (list->_d->line_status == new_data) {
            list->_d->line_status = no_data;
            // Find tab just after the timestamp
            int pos = _d->line.find('\t', 1);
            if (pos < 0) {
              return -1;
            }
            // Re-create line using previous data
            _d->line.append(&list->_d->line[2], pos);
          } else
          // Check for exception
          if (strncmp(_d->line, "\t0\t", 3) == 0) {
            list->putLine(_d->line);

            // Do not copy: merge with following line
            continue;
          }

          // Check for expiry
          if (! active_data_line && (expire >= 0)) {
            if (expire != 0) {
              // Check timestamp for expiration
              time_t ts = 0;
              if (! decodeLine(_d->line, &ts) && (ts < expire)) {
                continue;
              }
            } else {
              // Always obsolete
              continue;
            }
          } // ! active_data_line && (expire >= 0)
        } // list != NULL
        // Next line of data is not active
        active_data_line = false;
      }
    }

    // New data, add to merge
    if (list != NULL) {
      // Searched path found or status is eof/empty
      if ((_d->line_status != no_data) || (_d->status >= eof)) {
        if ((path_l != NULL) && (path_l[0] != '\0')) {
          // Write path
          if (list->Stream::putLine(path_l) < 0) {
            // Could not write
            return -1;
          }
        }
      } else
      // Our data is here or after, so let's copy if required
      if (list->Stream::putLine(_d->line) < 0) {
        // Could not write
        return -1;
      }
      // Remove empty status
      list->_d->status = ok;
    }
    // If line status is not 'no_data', return search status
    if (_d->status >= eof) {
      return 0;
    } else
    switch (_d->line_status) {
      case cached_data:
        // Found
        return 2;
      case new_data:
        // Exceeded
        return 1;
      case no_data:
        // Do nothing
        ;
    }
  }
  return -1;
}

int List::merge(
    List&         list,
    List&         journal) {
  // Check that all files are open
  if (! isOpen() || ! list.isOpen() || ! journal.isOpen()) {
    out(error, msg_standard, "At least one list is not open");
    return -1;
  }

  // Check open mode
  if (! isWriteable() || list.isWriteable() || journal.isWriteable()) {
    out(error, msg_standard, "At least one list is not open correctly");
    return -1;
  }

  int rc_list = 1;

  // Line read from journal
  int j_line_no = 0;

  // Current path and data (from journal)
  Path path;

  // Last search status
  _d->line_status = no_data;

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
        rc_list = list.search("", this);
        if (rc_list < 0) {
          // Error copying list
          out(error, msg_standard, "End of list copy failed");
          return -1;
        }
      }
      return 0;
    }

    // Got a path
    if (journal._d->line[0] != '\t') {
      // Check path order
      if (path.length() != 0) {
        if (path.compare(journal._d->line) > 0) {
          // Cannot go back
          out(error, msg_number, "Path out of order", j_line_no, "journal");
          return -1;
        }
      }
      // Copy new path
      path = journal._d->line;
      // Search/copy list and append path if needed
      if (rc_list >= 0) {
        rc_list = list.search(path, this);
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

      // If exception, try to put in list buffer (will succeed) otherwise write
      if (((strncmp(journal._d->line, "\t0\t", 3) != 0)
        || (list.putLine(journal._d->line) < 0))
       && (Stream::putLine(journal._d->line) < 0)) {
        // Could not write
        out(error, msg_standard, "Journal copy failed");
        return -1;
      }
    }
  }
  return 0;
}

void List::show(
    time_t          date,
    time_t          time_start,
    time_t          time_base) {
  if (! open("r")) {
    if (isEmpty()) {
      printf("List is empty\n");
      return;
    }
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
        printf(" %c %6llu %03o", node->type(),
        (node->type() != 'd') ? node->size() : 0, node->mode());
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
    close();
    if (rc < 0) {
      out(error, msg_standard, "Failed to read list");
    }
  } else {
    out(error, msg_standard, "Failed to open list");
  }
}
