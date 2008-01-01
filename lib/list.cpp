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

#include "files.h"
#include "list.h"
#include "hbackup.h"

using namespace hbackup;

int List::open(
    const char*   req_mode,
    int           compression) {
  const char old_header[] = "# version 2\n";
  const char header[]     = "# version 3\n";
  int        rc           = 0;

  if (Stream::open(req_mode, compression)) {
    rc = -1;
  } else
  // Open for write
  if (isWriteable()) {
    if (write(header, strlen(header)) < 0) {
      Stream::close();
      rc = -1;
    }
    _line_status = 0;
  } else
  // Open for read
  {
    // Check rights
    if (Stream::getLine(_line) < 0) {
      Stream::close();
      rc = -1;
    } else
    // Expected header?
    if (_line == header) {
      _old_version = false;
    } else
    // Old header?
    if (_line == old_header) {
      _old_version = true;
    } else
    // No.
    {
      errno = EUCLEAN;
      rc = -1;
    }
    _line_status = 1;
    getLine();
    if (_line_status < 0) {
      rc = -1;
    } else
    if (_line_status == 1) {
      _line_status = 3;
    }
    _client_cmp = 0;
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

const char* List::line(ssize_t* length) const {
  if (length != NULL) {
    *length = _line.length();
  }
  return _line.c_str();
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
      if (_line[0] == '#') {
        // Signal end of file
        _line_status = 0;
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

int List::decodeLine(
    const char*     line,
    const char*     path,
    Node**          node,
    time_t*         timestamp) {
  const char* start  = &line[2];
  int         fields = 7;
  // Fields
  char        type;             // file type
  time_t      mtime;            // time of last modification
  long long   size;             // on-disk size, in bytes
  uid_t       uid;              // user ID of owner
  gid_t       gid;              // group ID of owner
  mode_t      mode;             // permissions
  char*       extra = NULL;     // file checksum

  errno = 0;
  for (int field = 1; field <= fields; field++) {
    // Get tabulation position
    const char* delim;
    delim = strchr(start, '\t');
    if (delim == NULL) {
      delim = strchr(start, '\n');
    }
    if (delim == NULL) {
      errno = EUCLEAN;
    } else {
      // Extract data
      switch (field) {
        case 1:   // DB timestamp
          if ((timestamp != NULL) && sscanf(start, "%ld", timestamp) != 1) {
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
          asprintf(&extra, "%s", start);
          extra[strlen(extra) -1] = '\0';
          break;
        default:
          errno = EUCLEAN;;
      }
      start = delim + 1;
    }
    if (errno != 0) {
      cerr << "For path: " << path << endl;
      cerr << "Field " << field << " corrupted in line: " << line;
      type = '-';
      break;
    }
  }
  switch (type) {
    case '-':
      *node = NULL;
      break;
    case 'f':
      *node = new File(path, type, mtime, size, uid, gid, mode, extra);
      break;
    case 'l':
      *node = new Link(path, type, mtime, size, uid, gid, mode, extra);
      break;
    default:
      *node = new Node(path, type, mtime, size, uid, gid, mode);
  }
  free(extra);
  return (errno != 0) ? -1 : 0;
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
    // Client
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
    char**        client,
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

    // Client
    if (_line[0] != '\t') {
      if (client != NULL) {
        free(*client);
        *client = NULL;
        asprintf(client, "%s", &_line[0]);
        // Change end of line
        (*client)[length] = '\0';
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
        // Will set errno if an error is found
        decodeLine(_line.c_str(), path == NULL ? "" : *path, node, &ts);
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

int List::client(
    const char*     client) {
  ssize_t length = strlen(client);
  if ((Stream::write(client, length) != length)
   || (Stream::write("\n", 1) != 1)) {
    return -1;
  }
  _line_status = 1;
  return 0;
}

int List::path(
    const char*     path) {
  ssize_t length = strlen(path);
  if ((Stream::write("\t", 1) != 1)
   || (Stream::write(path, length) != length)
   || (Stream::write("\n", 1) != 1)) {
    return -1;
  }
  _line_status = 1;
  return 0;
}

int List::data(
    time_t          timestamp,
    const Node*     node,
    bool            bufferize) {
  char* line = NULL;
  int   size;
  int   rc   = 0;

  if (node == NULL) {
    size = asprintf(&line, "\t\t%ld\t-\n", timestamp);
  } else {
    char* temp = NULL;
    size = asprintf(&temp, "\t\t%ld\t%c\t%lld\t%ld\t%u\t%u\t%o",
      timestamp, node->type(), node->size(), node->mtime(), node->uid(),
      node->gid(), node->mode());
    switch (node->type()) {
      case 'f': {
        const char* extra  = ((File*) node)->checksum();
        size = asprintf(&line, "%s\t%s\n", temp, extra);
      } break;
      case 'l': {
        const char* extra  = ((Link*) node)->link();
        size = asprintf(&line, "%s\t%s\n", temp, extra);
      } break;
      default:
        size = asprintf(&line, "%s\n", temp);
    }
    free(temp);
  }
  if ((timestamp == 0) && bufferize) {
    // Send line to search method, so it can deal with exception
    _line        = line;
    _line_status = 3;
  } else
  if (Stream::write(line, size) != size) {
    rc = -1;
  } else {
    _line_status = 1;
  }
  free(line);
  return rc;
}

int List::search(
    const char*     client_l,
    const char*     path_l,
    List*           list,
    time_t          expire,
    list<string>*   active,
    list<string>*   expired) {
  int     path_cmp;
  int     rc         = 0;

  size_t  client_len = 0;   // Not set
  size_t  path_len   = 0;   // Not set

  // Pre-set client comparison result
  if (client_l == NULL) {
    // Any client will match
    _client_cmp = 0;
  } else
  if (client_l[0] == '\0') {
    // No need to compare clientes
    _client_cmp = 1;
  } else {
    // string or line?
    int len = strlen(client_l);
    if ((len > 0) && (client_l[len - 1] != '\n')) {
      // string: signal it by setting its lengh
      client_len = len;
    }
  }

  // Pre-set path comparison result
  if (client_l == NULL) {
    // No path will match
    path_cmp = 1;
  } else
  if (path_l == NULL) {
    // Any path will match
    path_cmp = 0;
  } else
  if ((_client_cmp > 0) || (path_l[0] == '\0')) {
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

  // Need to know whether line of data is active or not
  bool active_data_line = true;

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

    // Got a client
    if (_line[0] != '\t') {
      // Compare clientes
      if ((client_l != NULL) && (client_l[0] != '\0')) {
        if ((client_len > 0) && (_line.length() == (client_len + 1))) {
          _client_cmp = Path::compare(client_l, _line.c_str(), client_len);
        } else {
          _client_cmp = Path::compare(client_l, _line.c_str());
        }
      }
      if (_client_cmp <= 0)  {
        if (_client_cmp < 0) {
          // Client exceeded
          _line_status = 3;
        } else
        if ((client_l == NULL)
         || ((path_l != NULL) && (path_l[0] == '\0'))) {
          // Looking for client, found
          _line_status = 2;
        }
      }
      // Next line of data is active
      active_data_line = true;
    } else

    // Got a path
    if (_line[1] != '\t') {
      // Compare paths
      if ((_client_cmp <= 0) && (path_l != NULL) && (path_l[0] != '\0')) {
        if (path_len > 0) {
          if (_line.length() == (path_len + 2)) {
            path_cmp = Path::compare(path_l, &_line[1], path_len);
          } else {
            path_cmp = Path::compare(path_l, &_line[1]);
          }
        } else {
          path_cmp = Path::compare(path_l, _line.c_str());
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
      // Next line of data is active
      active_data_line = true;
    } else

    // Got data
    {
      if (list != NULL) {
        // Deal with exception
        if (list->_line_status == 3) {
          list->_line_status = 1;
          // Copy start of line (timestamp)
          size_t pos = _line.substr(2).find('\t');
          if (pos == string::npos) {
            errno = EUCLEAN;
            return -1;
          }
          pos += 3;
          // Re-create line using previous data
          _line.resize(pos);
          _line.append(list->_line.substr(4));
        } else
        // Check for exception
        if (strncmp(_line.c_str(), "\t\t0\t", 4) == 0) {
          // Get only end of line
          list->_line = _line;
          list->_line_status = 3;
          // Do not copy: merge with following line
          continue;
        }

        // Check for expiry
        if (expire >= 0) {
          // Check time
          time_t ts = 0;
          string reader = &_line[2];
          size_t pos = reader.find('\t');
          if ((pos != string::npos)
            && (sscanf(reader.substr(0, pos).c_str(), "%ld", &ts) == 1)) {
            reader = reader.substr(pos + 1, reader.size() - (pos + 1) - 1);
            const char* checksum = NULL;
            if ((reader[0] == 'f')
              && ((expired != NULL) || (active != NULL))) {
              pos = reader.rfind('\t');
              if (pos != string::npos) {
                checksum = &reader[pos + 1];
              }
            }
            if (active_data_line || ((expire != 0) && (ts >= expire))) {
              if ((checksum != NULL) && (active != NULL)) {
                active->push_back(checksum);
              }
            } else {
              if ((checksum != NULL) && (expired != NULL)) {
                expired->push_back(checksum);
              }
              // Do not copy: expired
              continue;
            }
          }
        } // expire >= 0
      } // list != NULL
      // Next line of data is not active
      active_data_line = false;
    }

    // New data, add
    if (list != NULL) {
      if (_line_status != 1) {
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
        if ((client_l != NULL)
         && (client_l[0] != '\0')
         && (path_l != NULL)) {
          // Write client
          if (list->write(client_l, strlen(client_l)) < 0) {
            // Could not write
            return -1;
          }
          if ((client_len != 0) && (list->write("\n", 1) < 0)) {
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

  // Current client, path and data (from journal)
  Path client;
  Path path;

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

    // Got a client
    if (journal._line[0] != '\t') {
      if (client.length() != 0) {
        int cmp = client.compare(journal._line.c_str());
        // If same client, ignore it
        if (cmp == 0) {
          cerr << "Client duplicated in journal, line " << j_line_no << endl;
          continue;
        }
        // Check path order
        if (cmp > 0) {
          // Cannot go back
          cerr << "Client out of order in journal, line " << j_line_no << endl;
          errno = EUCLEAN;
          rc = -1;
          break;
        }
      }
      // Copy new client
      client = journal._line.c_str();
      // No path for this entry yet
      path = "";
      // Search/copy list
      if (rc_list >= 0) {
        rc_list = list.search(client.c_str(), path.c_str(), this);
        if (rc_list < 0) {
          // Error copying list
          cerr << "Client search failed" << endl;
          rc = -1;
          break;
        }
      }
    } else

    // Got a path
    if (journal._line[1] != '\t') {
      // Must have a client by now
      if (client.length() == 0) {
        // Did not get a client first thing
        cerr << "Client missing in journal, line " << j_line_no << endl;
        errno = EUCLEAN;
        rc = -1;
        break;
      }
      // Check path order
      if (path.length() != 0) {
        if (path.compare(journal._line.c_str()) > 0) {
          // Cannot go back
          cerr << "Path out of order in journal, line " << j_line_no << endl;
          errno = EUCLEAN;
          rc = -1;
          break;
        }
      }
      // Copy new path
      path = journal._line.c_str();
      // Search/copy list
      if (rc_list >= 0) {
        rc_list = list.search(client.c_str(), path.c_str(), this);
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
      if ((client.length() == 0) || (path.length() == 0)) {
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
