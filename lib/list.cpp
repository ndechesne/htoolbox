/*
     Copyright (C) 2007  Herve Fache

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

#include <iostream>
#include <list>

#include <string.h>
#include <errno.h>

using namespace std;

#include "strings.h"
#include "files.h"
#include "list.h"
#include "hbackup.h"

using namespace hbackup;

int List::decodeLine(
    const char*   path,
    Node**        node,
    time_t*       timestamp) {
  char*     start  = &_line[2];
  int       fields = 7;
  // Fields
  char      type;             // file type
  time_t    mtime;            // time of last modification
  long long size;             // on-disk size, in bytes
  uid_t     uid;              // user ID of owner
  gid_t     gid;              // group ID of owner
  mode_t    mode;             // permissions
  char*     checksum = NULL;  // file checksum
  char*     link     = NULL;  // what the link points to

  for (int field = 1; field <= fields; field++) {
    // Get tabulation position
    char* delim = strchr(start, '\t');
    if (delim == NULL) {
      errno = EUCLEAN;
    } else {
      *delim = '\0';
      // Extract data
      switch (field) {
        case 1:   // DB timestamp
          if ((timestamp != NULL)
            && sscanf(start, "%ld", timestamp) != 1) {
            errno = EUCLEAN;
          }
          break;
        case 2:   // Type
          if (sscanf(start, "%c", &type) != 1) {
            errno = EUCLEAN;;
          } else if (type == '-') {
            fields = 2;
          } else if ((type == 'f') || (type == 'l')) {
            fields++;
          }
          break;
        case 3:   // Size
          if (sscanf(start, "%lld", &size) != 1) {
            errno = EUCLEAN;;
          }
          break;
        case 4:   // Modification time
          if (sscanf(start, "%ld", &mtime) != 1) {
            errno = EUCLEAN;;
          }
          break;
        case 5:   // User
          if (sscanf(start, "%u", &uid) != 1) {
            errno = EUCLEAN;;
          }
          break;
        case 6:   // Group
          if (sscanf(start, "%u", &gid) != 1) {
            errno = EUCLEAN;;
          }
          break;
        case 7:   // Permissions
          if (sscanf(start, "%o", &mode) != 1) {
            errno = EUCLEAN;;
          }
          break;
        case 8:  // Checksum or Link
            if (type == 'f') {
              checksum = start;
            } else if (type == 'l') {
              link = start;
            }
          break;
        default:
          errno = EUCLEAN;;
      }
      start = delim + 1;
    }
    if (errno != 0) {
      cerr << "dblist: file corrupted line " << _line.c_str() << endl;
      errno = EUCLEAN;
      break;
    }
  }
  switch (type) {
    case '-':
      *node = NULL;
      break;
    case 'f':
      *node = new File(path, type, mtime, size, uid, gid, mode, checksum);
      break;
    case 'l':
      *node = new Link(path, type, mtime, size, uid, gid, mode, link);
      break;
    default:
      *node = new Node(path, type, mtime, size, uid, gid, mode);
  }
  return (errno != 0) ? -1 : 0;
}

int List::open(
    const char*   req_mode,
    int           compression) {
  const char header[] = "# version 2\n";
  int rc = 0;

  if (Stream::open(req_mode, compression)) {
    rc = -1;
  } else
  // Open for write
  if (isWriteable()) {
    if (write(header, strlen(header)) < 0) {
      Stream::close();
      rc = -1;
    }
  } else
  // Open for read
  {
    // Check rights
    if (Stream::getLine(_line) < 0) {
      Stream::close();
      rc = -1;
    } else
    // Check header
    if (_line != header) {
      errno = EUCLEAN;
      rc = -1;
    } else {
      ssize_t size = Stream::getLine(_line);
      if (size < 0) {
        // Signal error
        _line_status = -1;
        rc = -1;
      } else
      if (size == 0) {
        // Signal unexpected end of file
        _line_status = -2;
      } else
      // Check emptiness
      if (_line[0] == '#') {
        // Signal emptiness
        _line_status = 0;
      } else
      // Tell reader that there is a line cached
      {
        _line_status = 3;
      }
    }
    _prefix_cmp = 0;
  }
  return rc;
}

int List::close() {
  const char footer[] = "# end\n";
  int rc = 0;

  if (isWriteable()) {
    if (Stream::write(footer, strlen(footer)) < 0) {
      rc = -1;
    }
  }
  if (Stream::close()) {
    rc = -1;
  }
  return rc;
}

ssize_t List::getLine(bool use_found) {
  int status = _line_status;
  if (_line_status == 2) {
    if (use_found) {
      status = 3;
    } else {
      status = 1;
    }
  }
  switch (status) {
    case -2:
      // Unexpected end of file
      return 0;
    case 0:
      // Empty list
      return _line.length();
    case 1: {
      // Line contains no re-usable data,
      ssize_t length = Stream::getLine(_line);
      if (length < 0) {
        _line_status = -1;
      } else
      if (length == 0) {
        _line_status = -2;
      } else
      {
        _line_status = 1;
      }
      return length;
    }
    case 3:
      // Line contains data to be re-used
      _line_status = 1;
      return _line.length();
    default:
      // Error
      _line_status = 1;
      return -1;
  }
}

ssize_t List::putLine(const char* line) {
  if ((_line_status == 1) || (_line_status == 2)) {
    _line        = line;
    _line_status = 3;
    return _line.length();
  }
  return -1;
}

void List::keepLine() {
  _line_status = 3;
}

char List::getLineType() {
  getLine();
  // Status is one of -2: eof!, -1: error, 0: eof, 1: ok
  switch (_line_status) {
    case -2:
      // Unexpected eof
      return 'U';
    case 0:
      // Eof
      return 'E';
    case 1:
      break;
    default:
      // Failure
      return 'F';
  }
  // Line should be re-used
  _line_status = 3;
  if (_line[0] != '\t') {
    // Prefix (client)
    return 'C';
  } else
  if (_line[1] != '\t') {
    // Path
    return 'P';
  } else
  {
    const char* pos = strchr(&_line[2], '\t');
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
    char**        prefix,
    char**        path,
    Node**        node,
    time_t        date) {
  // Initialise
  errno = 0;
  if (node != NULL) {
    free(*node);
    *node = NULL;
  }

  bool    got_path;
  ssize_t length;

  if (date <= -1) {
    got_path = true;
  } else {
    got_path = false;
  }

  while (true) {
    // Get line
    if (date != -2) {
      length = getLine(true);
    } else {
      length = _line.length();
    }

    if (length == 0) {
      errno = EUCLEAN;
      cerr << "unexpected end of file" << endl;
      return -1;
    }

    // Check line
    length--;
    if ((length < 2) || (_line[length] != '\n')) {
      errno = EUCLEAN;
      return -1;
    }

    // End of file
    if (_line[0] == '#') {
      // Make sure we return end of file also if called again
      _line_status = 0;
      return 0;
    }

    // Prefix
    if (_line[0] != '\t') {
      if (prefix != NULL) {
        free(*prefix);
        *prefix = NULL;
        asprintf(prefix, "%s", &_line[0]);
        // Change end of line
        (*prefix)[length] = '\0';
      }
    } else

    // Path
    if (_line[1] != '\t') {
      if (path != NULL) {
        free(*path);
        *path = NULL;
        asprintf(path, "%s", &_line[1]);
        // Change end of line
        (*path)[length - 1] = '\0';
      }
      got_path = true;
    } else

    // Data
    if (got_path) {
      time_t ts;
      if (node != NULL) {
        _line[length] = '\t';
        // Will set errno if an error is found
        decodeLine(path == NULL ? "" : *path, node, &ts);
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

int List::added(
    const char*   prefix,
    const char*   path,
    const Node*   node,
    time_t        timestamp) {
  if (prefix != NULL) {
    Stream::write(prefix, strlen(prefix));
    Stream::write("\n", 1);
  }
  Stream::write("\t", 1);
  Stream::write(path, strlen(path));
  Stream::write("\n", 1);
  char* line = NULL;
  int size = asprintf(&line, "\t\t%ld\t%c\t%lld\t%ld\t%u\t%u\t%o",
    (timestamp >= 0) ? timestamp : time(NULL), node->type(), node->size(),
    node->mtime(), node->uid(), node->gid(), node->mode());
  Stream::write(line, size);
  free(line);
  switch (node->type()) {
    case 'f':
      Stream::write("\t", 1);
      {
        const char* checksum = ((File*) node)->checksum();
        Stream::write(checksum, strlen(checksum));
      }
      break;
    case 'l':
      Stream::write("\t", 1);
      {
        const char* link = ((Link*) node)->link();
        Stream::write(link, strlen(link));
      }
  }
  Stream::write("\n", 1);
  return 0;
}

int List::removed(
    const char*   prefix,
    const char*   path,
    time_t        timestamp) {
  if (prefix != NULL) {
    Stream::write(prefix, strlen(prefix));
    Stream::write("\n", 1);
  }
  Stream::write("\t", 1);
  Stream::write(path, strlen(path));
  Stream::write("\n", 1);
  char* line = NULL;
  int size = asprintf(&line, "\t\t%ld\t-\n",
    (timestamp >= 0) ? timestamp : time(NULL));
  Stream::write(line, size);
  free(line);
  return 0;
}

int List::search(
    const char*     prefix_l,
    const char*     path_l,
    List*           list,
    time_t          expire,
    list<string>*   active,
    list<string>*   expired) {
  string  exception_line;
  int     path_cmp;
  int     rc         = 0;

  size_t  prefix_len = 0;   // Not set
  size_t  path_len   = 0;   // Not set

  // Pre-set prefix comparison result
  if (prefix_l == NULL) {
    // Any prefix will match
    _prefix_cmp = 0;
  } else
  if (prefix_l[0] == '\0') {
    // No need to compare prefixes
    _prefix_cmp = 1;
  } else {
    // string or line?
    int len = strlen(prefix_l);
    if ((len > 0) && (prefix_l[len - 1] != '\n')) {
      // string: signal it by setting its lengh
      prefix_len = len;
    }
  }

  // Pre-set path comparison result
  if (prefix_l == NULL) {
    // No path will match
    path_cmp = 1;
  } else
  if (path_l == NULL) {
    // Any path will match
    path_cmp = 0;
  } else
  if ((_prefix_cmp > 0) || (path_l[0] == '\0')) {
    // No need to compare paths
    path_cmp = 1;
  } else
  {
    // Any value will do
    path_cmp = -1;
    // string or line?
    int len = strlen(path_l);
    if ((len > 1) && (path_l[0] != '\t') && (path_l[len - 1] != '\n')) {
      // string: signal it by setting its lengh
      path_len = len;
    }
  }

  while (true) {
    // Read list or get last data
    rc = getLine();

    // Failed
    if (rc <= 0) {
      // Unexpected end of file
      cerr << "Unexpected end of list" << endl;
      errno = EUCLEAN;
      return -1;
    }

    // Check line
    if ((rc < 2) || (_line[rc - 1] != '\n')) {
      // Corrupted line
      cerr << "Corrupted line in list" << endl;
      errno = EUCLEAN;
      return -1;
    }

    // End of file
    if (_line[0] == '#') {
      // Future searches will return eof too
      _line_status = 0;
    } else

    // Got a prefix
    if (_line[0] != '\t') {
      // Compare prefixes
      if ((prefix_l != NULL) && (prefix_l[0] != '\0')) {
        if ((prefix_len > 0) && (_line.length() == (prefix_len + 1))) {
          _prefix_cmp = pathCompare(prefix_l, _line.c_str(), prefix_len);
        } else {
          _prefix_cmp = pathCompare(prefix_l, _line.c_str());
        }
      }
      if (_prefix_cmp <= 0)  {
        if (_prefix_cmp < 0) {
          // Prefix exceeded
          _line_status = 3;
        } else
        if ((prefix_l == NULL)
         || ((path_l != NULL) && (path_l[0] == '\0'))) {
          // Looking for prefix, found
          _line_status = 2;
        }
      }
    } else

    // Got a path
    if (_line[1] != '\t') {
      // Compare paths
      if ((_prefix_cmp <= 0) && (path_l != NULL) && (path_l[0] != '\0')) {
        if (path_len > 0) {
          if (_line.length() == (path_len + 2)) {
            path_cmp = pathCompare(path_l, &_line[1], path_len);
          } else {
            path_cmp = pathCompare(path_l, &_line[1]);
          }
        } else {
          path_cmp = pathCompare(path_l, _line.c_str());
        }
      }
      if (path_cmp <= 0) {
        if (path_cmp < 0) {
          // Path exceeded
          _line_status = 3;
        } else {
          // Looking for path, found
          _line_status = 2;
        }
      }
    } else

    // Got data
    {
      // Deal with exception
      if (exception_line.size() != 0) {
        // Copy start of line (timestamp)
        size_t pos = _line.substr(2).find('\t');
        if (pos == string::npos) {
          errno = EUCLEAN;
          return -1;
        }
        pos += 3;
        // Re-create line using previous data
        _line.resize(pos);
        _line.append(exception_line);
        exception_line = "";
      } else
      // Check for exception
      if (strncmp(_line.c_str(), "\t\t0\t", 4) == 0) {
        // Get only end of line
        exception_line = _line.substr(4);
        // Do not copy: merge with following line
        continue;
      }

      // Check for expiry
      if (expire > 0) {
        // Check time
        time_t ts = 0;
        string reader = &_line[2];
        reader[reader.size() - 1] = '\0';
        size_t pos = reader.find('\t');
        if ((pos != string::npos)
          && (sscanf(reader.substr(pos - 1).c_str(), "%lu", &ts) == 1)) {
          reader = reader.substr(pos + 1);
          const char* checksum = NULL;
          if ((reader[0] == 'f')
            && ((expired != NULL) || (active != NULL))) {
            pos = reader.rfind('\t');
            if (pos != string::npos) {
              checksum = &reader[pos + 1];
            }
          }
          if (time(NULL) - ts > expire) {
            if ((checksum != NULL) && (expired != NULL)) {
              expired->push_back(checksum);
            }
            // Do not copy: expired
            continue;
          } else {
            if ((checksum != NULL) && (active != NULL)) {
              active->push_back(checksum);
            }
          }
        }
      }
    }

    // New data, add
    if (list != NULL) {
      if ((_line_status == 0) || (_line_status == 3)) {
        if ((path_l != NULL) && (path_l[0] != '\0')) {
          // Write path
          if ((path_len != 0) && (list->write("\t", 1) < 0)) {
            // Could not write
            return -1;
          }
          if (list->write(path_l, strlen(path_l)) < 0) {
            // Could not write
            return -1;
          }
          if ((path_len != 0) && (list->write("\n", 1) < 0)) {
            // Could not write
            return -1;
          }
        } else
        if ((prefix_l != NULL) && (prefix_l[0] != '\0')) {
          // Write prefix
          if (list->write(prefix_l, strlen(prefix_l)) < 0) {
            // Could not write
            return -1;
          }
          if ((prefix_len != 0) && (list->write("\n", 1) < 0)) {
            // Could not write
            return -1;
          }
        }
      } else
      // Our data is here or after, so let's copy if required
      if (list->write(_line.c_str(), _line.length()) < 0) {
        // Could not write
        return -1;
      }
    }
    // If line status is not 1, return search status
    switch (_line_status) {
      case 0:
        // EOF
        return 0;
      case 2:
        // Found
        return 2;
      case 3:
        // Exceeded
        return 1;
    }
  }
  return -2;
}

int List::merge(
    List&         list,
    List&         journal) {
  // Check that all files are open
  if (! isOpen() || ! list.isOpen() || ! journal.isOpen()) {
    errno = EBADF;
    return -1;
  }

  // Check open mode
  if (! isWriteable() || list.isWriteable() || journal.isWriteable()) {
    errno = EINVAL;
    return -1;
  }

  int rc      = 1;
  int rc_list = 1;

  // Line read from journal
  int j_line_no = 0;

  // Current prefix, path and data (from journal)
  StrPath prefix;
  StrPath path;

  // Last search status
  _line_status = 1;

  // Parse journal
  while (rc > 0) {
    int  rc_journal = journal.getLine();
    j_line_no++;

    // Failed
    if (rc_journal < 0) {
      cerr << "Error reading journal, line " << j_line_no << endl;
      rc = -1;
      break;
    }

    // Check line
    if ((rc_journal != 0)
     && ((rc_journal < 2) || (journal._line[rc_journal - 1] != '\n'))) {
      // Corrupted line
      cerr << "Corruption in journal, line " << j_line_no << endl;
      errno = EUCLEAN;
      rc = -1;
      break;
    }

    // End of file
    if ((journal._line[0] == '#') || (rc_journal == 0)) {
      if (rc_list > 0) {
        rc_list = list.search("", "", this);
        if (rc_list < 0) {
          // Error copying list
          cerr << "End of list copy failed" << endl;
          rc = -1;
          break;
        }
      }
      if (rc_journal == 0) {
        cerr << "Unexpected end of journal, line " << j_line_no << endl;
        errno = EBUSY;
        rc = -1;
      }
      rc = 0;
      break;
    }

    // Got a prefix
    if (journal._line[0] != '\t') {
      if (prefix.length() != 0) {
        int cmp = prefix.compare(journal._line);
        // If same prefix, ignore it
        if (cmp == 0) {
          cerr << "Prefix duplicated in journal, line " << j_line_no << endl;
          continue;
        }
        // Check path order
        if (cmp > 0) {
          // Cannot go back
          cerr << "Prefix out of order in journal, line " << j_line_no << endl;
          errno = EUCLEAN;
          rc = -1;
          break;
        }
      }
      // Copy new prefix
      prefix = journal._line;
      // No path for this entry yet
      path = "";
      // Search/copy list
      if (rc_list >= 0) {
        rc_list = list.search(prefix.c_str(), path.c_str(), this);
        if (rc_list < 0) {
          // Error copying list
          cerr << "Prefix search failed" << endl;
          rc = -1;
          break;
        }
      }
    } else

    // Got a path
    if (journal._line[1] != '\t') {
      // Must have a prefix by now
      if (prefix.length() == 0) {
        // Did not get a prefix first thing
        cerr << "Prefix missing in journal, line " << j_line_no << endl;
        errno = EUCLEAN;
        rc = -1;
        break;
      }
      // Check path order
      if (path.length() != 0) {
        if (path.compare(journal._line) > 0) {
          // Cannot go back
          cerr << "Path out of order in journal, line " << j_line_no << endl;
          errno = EUCLEAN;
          rc = -1;
          break;
        }
      }
      // Copy new path
      path = journal._line;
      // Search/copy list
      if (rc_list >= 0) {
        rc_list = list.search(prefix.c_str(), path.c_str(), this);
        if (rc_list < 0) {
          // Error copying list
          cerr << "Path search failed" << endl;
          rc = -1;
          break;
        }
      }
    } else

    // Got data
    {
      // Must have a path before then
      if ((prefix.length() == 0) || (path.length() == 0)) {
        // Did not get anything before data
        cerr << "Data out of order in journal, line " << j_line_no << endl;
        errno = EUCLEAN;
        rc = -1;
        break;
      }

      // If exception, try to put in list buffer (will succeed) otherwise write
      if (((strncmp(journal._line.c_str(), "\t\t0\t", 4) != 0)
        || (list.putLine(journal._line.c_str()) < 0))
       && (write(journal._line.c_str(), journal._line.length()) < 0)) {
        // Could not write
        cerr << "Journal copy failed" << endl;
        rc = -1;
        break;
      }
    }
  }

  return rc;
}
