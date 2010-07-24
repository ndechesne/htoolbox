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

struct List::Private {
  Path              s_path;
  FILE*             stream;
  const char*       header;
  const char*       old_header;
  const char*       footer;
  bool              read_only;
  bool              old_version;
  Status            status;
  Line              data;
  Path              path;
  Private(const char* path_in) : s_path(path_in), stream(NULL),
    header("# version 5"), old_header("# version 4"), footer("# end") {}
};

ssize_t List::getLine(Line& line, bool version) {
  LineBuffer& buffer = line;
  ssize_t rc;
  char delim;
  if (! version && ! _d->old_version) {
    delim = '\0';
  } else {
    delim = '\n';
  }
  rc = getdelim(buffer.bufferPtr(), buffer.capacityPtr(), delim, _d->stream);
  if (rc <= 0) {
    return feof(_d->stream) ? 0 : -1;
  }
  if (line[rc - 1] != delim) {
    return 0;
  }
  if (! version && ! _d->old_version) {
    char lf[1];
    if ((fread(lf, 1, 1, _d->stream) < 1) || (*lf != '\n')) {
      return 0;
    }
  }
  *buffer.sizePtr() = rc;
  --rc;
  buffer.erase(rc);
  return rc;
}

List::List(const Path& path) : _d(new Private(path)) {}

List::~List() {
  delete _d;
}

int List::create() {
  _d->stream = fopen(_d->s_path, "w");
  if (_d->stream == NULL) {
    hlog_error("%s creating '%s'", strerror(errno), _d->s_path.c_str());
    return -1;
  }
  if (putLine(_d->header) < 0) {
    hlog_error("%s writing header in '%s'", strerror(errno), _d->s_path.c_str());
    fclose(_d->stream);
    _d->stream = NULL;
    return -1;
  }
  _d->read_only = false;
  return 0;
}

int List::open() {
  _d->read_only = true;
  _d->stream = fopen(_d->s_path, "r");
  if (_d->stream == NULL) {
    return -1;
  }
  // Check rights
  Line data;
  if (getLine(data, true) < 0) {
    close();
    return -1;
  }
  int rc = 0;
  // Expected header?
  if (strcmp(data, _d->header) == 0) {
    _d->old_version = false;
  } else
  // Old header?
  if (strcmp(data, _d->old_header) == 0) {
    _d->old_version = true;
  } else
  // Unknown header
  {
    errno = EPROTO;
    rc = -1;
  }
  // Get first line to check for empty list
  if ((rc == 0) && (fetchLine(true) == List::failed)) {
    rc = -1;
  }
  return rc;
}

int List::close() {
  int rc = 0;
  if (! _d->read_only) {
    if (putLine(_d->footer) < 0) {
      hlog_error("%s writing footer in '%s'", strerror(errno), _d->s_path.c_str());
      rc = -1;
    }
  }
  if (fclose(_d->stream)) {
    hlog_error("%s closing '%s'", strerror(errno), _d->s_path.c_str());
    rc = -1;
  }
  _d->stream = NULL;
  return rc;
}

int List::flush() {
  return fflush(_d->stream);
}

const Path& List::path() const {
  return _d->s_path;
}

const Path& List::getPath() const {
  return _d->path;
}

const Line& List::getData() const {
  return _d->data;
}

List::Status List::fetchLine(bool init) {
  // Check current status
  if (init || (_d->status == List::got_nothing)) {
    // Line contains no re-usable data
    ssize_t length = getLine(_d->data);
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
          out(warning, msg_standard, "Repeated path in list", -1, _d->path);
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

ssize_t List::putLine(const Line& line) {
  if (fwrite(line, line.size(), 1, _d->stream) < 1) return -1;
  if (fwrite("\0\n", 2, 1, _d->stream) < 1) return -1;
  return line.size();
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
    out(error, msg_standard, "Wrong number of arguments for line", -1, NULL);
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

int List::add(
    const Path&     path,
    const Node*     node,
    List*           new_list,
    List*           journal) {
  int rc = 0;
  if (new_list != NULL) {
    char* line = NULL;
    ssize_t size = encodeLine(&line, time(NULL), node);

    if (new_list->putLine(line) != size) {
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
  }
  return rc;
}

void List::setProgressCallback(progress_f progress) {
  (void) progress;
//   _d->stream.setProgressCallback(progress);
}

bool List::isEmpty(
    const List*     list) {
  // If EOF is reached immediately, list is empty
  return list->end();
}

int List::getEntry(
    List*           list,
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
    rc = list->fetchLine();

    if (rc < 0) {
      if (rc == List::eof) {
        out(error, msg_standard, "Unexpected end of list", -1, list->path());
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
        *path = strdup(list->getPath());
      }
      get_path = false;
    } else

    // Data
    if (! get_path) {
      time_t ts;
      if ((node != NULL) || (timestamp != NULL) || (date > 0)) {
        // Get all arguments from line
        List::decodeLine(list->getData(), &ts, list->getPath(), node);
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
      list->resetStatus();
    }
  }
  return 1;
}

int List::search(
    List*           list,
    const char*     path_l,
    time_t          expire,
    time_t          remove,
    List*           new_list,
    List*           journal,
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
      out(error, msg_standard, "Unexpected end of list", -1, list->path());
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
                out(info, msg_standard, list->getPath(), -2, "D      ");
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

int List::merge(
    List*           list,
    List*           new_list,
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
        out(warning, msg_standard, "Unexpected end of journal", -1,
          journal->path());
        rc_journal = 0;
      } else {
        hlog_error("reading journal, at line %d", j_line_no);
        return -1;
      }
    }

    // End of file
    if (rc_journal == 0) {
      if (rc_list > 0) {
        rc_list = search(list, "", -1, 0, new_list);
        if (rc_list < 0) {
          // Error copying list
          out(error, msg_standard, "End of list copy failed", -1, NULL);
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
          hlog_error("journal, line %d: path out of order: '%s' > '%s'",
            j_line_no, path.c_str(), journal->getPath().c_str());
          return -1;
        }
      }
      // Copy new path
      path = journal->getPath();
      // Search/copy list and append path if needed
      if (rc_list >= 0) {
        rc_list = search(list, path, -1, 0, new_list);
        if (rc_list < 0) {
          // Error copying list
          out(error, msg_standard, "Path search failed", -1, NULL);
          return -1;
        }
      }
    } else

    // Got data
    {
      // Must have a path before then
      if (path.length() == 0) {
        // Did not get anything before data
        hlog_error("data out of order in journal, at line %d", j_line_no);
        return -1;
      }

      // Write
      if (new_list->putLine(journal->getData()) < 0) {
        // Could not write
        out(error, msg_standard, "Journal copy failed", -1, NULL);
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
      while ((rc = List::getEntry(this, &ts, &path, &node, date)) > 0) {
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
        out(error, msg_standard, "Failed to read list", -1, NULL);
      }
    }
    close();
  } else {
    hlog_error("%s opening '%s'", strerror(errno), _d->s_path.c_str());
  }
}
