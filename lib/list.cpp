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
        _line_status = 2;
      } else
      // Tell reader that there is a line cached
      {
        _line_status = 1;
      }
    }
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

ssize_t List::getLine() {
  switch (_line_status) {
    case -2:
      // Unexpected end of file
      return 0;
    case 0: {
      // Need new line
      ssize_t length = Stream::getLine(_line);
      if (length < 0) {
        _line_status = -1;
      } else
      if (length == 0) {
        _line_status = -2;
      }
      return length;
    }
    case 1:
      // Re-use current line
      _line_status = 0;
      return _line.length();
    case 2:
      // Empty file (_line_status == 2)
      return _line.length();
    default:
      // Error
      return -1;
  }
}

bool List::findPrefix(const char* prefix_in) {
  StrPath prefix;
  if (prefix_in != NULL) {
    prefix  = prefix_in;
    prefix += "\n";
  }
  bool found = false;
  while ((getLine() > 0) && (_line[0] != '#')) {
    if ((_line[0] != '\t') && (_line >= prefix)) {
      if ((_line == prefix) || (prefix_in == NULL)) {
        found = true;
      }
      break;
    }
  }
  _line_status = 1;
  return found;
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

  bool    done = false;
  ssize_t length;

  while (! done) {
    // Get line
    length = getLine();

    if (length == 0) {
      errno = EUCLEAN;
      cerr << "unexpected end of file" << endl;
      break;
    }

    // Check line
    length--;
    if ((length < 2) || (_line[length] != '\n')) {
      errno = EUCLEAN;
      break;
    }

    // End of file
    if (_line[0] == '#') {
      // Make sure we return end of file also if called again
      _line_status = 1;
      return 0;
    }

    // Change end of line
    _line[length] = '\0';

    // Prefix
    if (_line[0] != '\t') {
      if (prefix != NULL) {
        free(*prefix);
        *prefix = NULL;
        asprintf(prefix, "%s", &_line[0]);
      }
    } else

    // Path
    if (_line[1] != '\t') {
      if (path != NULL) {
        free(*path);
        *path = NULL;
        asprintf(path, "%s", &_line[1]);
      }
      date = -1;
    } else

    // Data
    if (date != 0) {
      if (node != NULL) {
        _line[length] = '\t';
        // Will set errno if an error is found
        decodeLine(*path, node, timestamp);
      }
      done = true;
    }
  }

  if (errno != 0) {
    if (prefix != NULL) {
      free(*prefix);
      *prefix = NULL;
    }
    if (path != NULL) {
      free(*path);
      *path = NULL;
    }
    if (node != NULL) {
      free(*node);
      *node = NULL;
    }
    return -1;
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

int List::searchCopy(
    List&         list,
    StrPath&      prefix_l,
    StrPath&      path_l,
    time_t        expire,
    list<string>* active,
    list<string>* expired) {
  string exception_line;
  int    path_cmp;
  int    rc = 0;

  // No need to compare prefixes
  if (prefix_l.length() == 0) {
    list._line_status = 1;
  }
  // No need to compare paths
  if ((list._line_status > 0) || (path_l.length() == 0)) {
    path_cmp = 1;
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

    // End of file
    if (_line[0] == '#') {
      return 0;
    }

    // Check line
    if ((rc < 2) || (_line[rc - 1] != '\n')) {
      // Corrupted line
      cerr << "Corrupted line in list" << endl;
      errno = EUCLEAN;
      return -1;
    }

    // Got a prefix
    if (_line[0] != '\t') {
      // Compare prefixes
      if (prefix_l.length() != 0) {
        list._line_status = prefix_l.compare(_line);
      }
      if (list._line_status <= 0)  {
        if (list._line_status < 0) {
          // Prefix not found
          _line_status = 1;
          return 1;
        } else
        if (path_l.length() == 0) {
          // Looking for prefix, found
          return 1;
        }
      }
    } else

    // Got a path
    if (_line[1] != '\t') {
      // Compare paths
      if ((list._line_status <= 0) && (path_l.length() != 0)) {
        path_cmp = path_l.compare(_line);
      }
      if (path_cmp <= 0) {
        if (path_cmp < 0) {
          // Path not found
          _line_status = 1;
        }
        return 1;
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
    // Our data is here or after, so let's copy
    if (list.write(_line.c_str(), _line.length()) < 0) {
      // Could not write
      return -1;
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

  // Last searchCopy status
  _line_status = 0;

  // Parse journal
  while (rc > 0) {
    int rc_journal = journal.getLine();
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
      prefix = "";
      path   = "";
      if (rc_list > 0) {
        rc_list = list.searchCopy(*this, prefix, path);
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
      if (rc_list > 0) {
        rc_list = list.searchCopy(*this, prefix, path);
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
      if (rc_list > 0) {
        rc_list = list.searchCopy(*this, prefix, path);
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
    }

    // Copy journal line
    if (write(journal._line.c_str(), journal._line.length()) < 0) {
      // Could not write
      cerr << "Journal copy failed" << endl;
      rc = -1;
      break;
    }
  }

  return rc;
}
