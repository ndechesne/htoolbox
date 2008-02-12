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

#include <iostream>
#include <list>

#include <string.h>
#include <errno.h>

using namespace std;

#include "files.h"
#include "list.h"
#include "hbackup.h"

using namespace hbackup;

enum Status {
  unexpected_eof = -2,      // unexpected end of file
  _error,                    // an error occured
  empty,                    // list is empty
  no_data,                  // line contains no valid data
  cached_data,              // line contains searched-for data
  new_data                  // line contains new data
};

struct List::Private {
  string            line;
  Status            line_status;
  // Keep current client and path
  string            client;
  string            path;
  // Need to keep client status for search() (reading)
  // Also used to set current client and path when writing
  int               client_cmp;
  int               path_cmp;
  // Set when a previous version is detected
  bool              old_version;
};

List::List(Path path) : Stream(path), _d(new Private) {}

List::~List() {
  delete _d;
}

int List::open(
    const char*   req_mode,
    int           compression) {
  const char old_header[] = "# version 2";
  const char header[]     = "# version 3";
  int        rc           = 0;

  if (Stream::open(req_mode, compression)) {
    rc = -1;
  } else
  // Open for write
  if (isWriteable()) {
    if (Stream::putLine(header) < 0) {
      Stream::close();
      rc = -1;
    }
    _d->line_status = empty;
  } else
  // Open for read
  {
    // Check rights
    if (Stream::getLine(_d->line) < 0) {
      Stream::close();
      rc = -1;
    } else
    // Expected header?
    if (_d->line == header) {
      _d->old_version = false;
    } else
    // Old header?
    if (_d->line == old_header) {
      _d->old_version = true;
    } else
    // Unknown header
    {
      errno = EUCLEAN;
      rc = -1;
    }
    _d->line_status = no_data;
    fetchLine();
    if ((_d->line_status == _error) ||  (_d->line_status == unexpected_eof)) {
      rc = -1;
    } else
    if (_d->line_status == no_data) {
      _d->line_status = new_data;
    }
    _d->client     = "";
    _d->client_cmp = 0;
    _d->path       = "";
    _d->path_cmp   = 0;
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

bool List::isOldVersion() const {
  return _d->old_version;
}

bool List::isEmpty() const {
  return _d->line_status == empty;
}

ssize_t List::fetchLine(bool use_found) {
  // Line in buffer is marked as used but re-usable?
  if (_d->line_status == cached_data) {
    // Are we told to re-use it?
    if (use_found) {
      // Force re-use
      _d->line_status = new_data;
    } else {
      // Force fetch
      _d->line_status = no_data;
    }
  }

  // Return line/status depending on line status
  switch (_d->line_status) {
    case unexpected_eof:
      // Unexpected end of file
      return empty;
    case empty:
      // Empty list
      return _d->line.length();
    case no_data: {
      // Line contains no re-usable data
      bool    eol;
      ssize_t length = Stream::getLine(_d->line, &eol);
      if (length < 0) {
        _d->line_status = _error;
      } else
      if (length == 0) {
        _d->line_status = unexpected_eof;
      } else
      if (! eol) {
        // All lines end in end-of-line character
        _d->line_status = _error;
      } else
      if (_d->line[0] == '#') {
        // Signal end of file
        _d->line_status = empty;
      } else
      {
        _d->line_status = no_data;
      }
      return length;
    }
    case new_data:
      // Line contains data to be re-used
      _d->line_status = no_data;
      return _d->line.length();
    default:
      // Error
      _d->line_status = no_data;
      return _error;
  }
}

// Insert line into buffer
ssize_t List::putLine(const char* line) {
  if ((_d->line_status == no_data) || (_d->line_status == cached_data)) {
    _d->line        = line;
    _d->line_status = new_data;
    return _d->line.length();
  }
  return -1;
}

// Get line from file/buffer
ssize_t List::getLine(
    string&         buffer) {
  int status = fetchLine(true);
  if (status <= 0) {
    return status;
  }
  buffer          = _d->line;
  _d->line_status = no_data;
  return _d->line.length();
}

void List::keepLine() {
  _d->line_status = new_data;
}

int List::decodeDataLine(
    const string&   line,
    const char*     path,
    Node**          node,
    time_t*         timestamp) {
  // Fields
  char        type;             // file type
  time_t      mtime;            // time of last modification
  long long   size;             // on-disk size, in bytes
  uid_t       uid;              // user ID of owner
  gid_t       gid;              // group ID of owner
  mode_t      mode;             // permissions
  // Arguments
  vector<string> params;

  // Get all arguments from line
  extractParams(line, params, Stream::flags_empty_params, 0, "\t\n");

  // Check result
  if (params.size() < 4) {
    cerr << "Error: wrong number of arguments for line" << endl;
    return -1;
  }

  // DB timestamp
  if ((timestamp != NULL) && sscanf(params[2].c_str(), "%ld", timestamp) != 1) {
    cerr << "Error: cannot decode timestamp" << endl;
    return -1;
  }

  // Type
  if (params[3].size() != 1) {
    cerr << "Error: cannot decode type" << endl;
    return -1;
  }
  type = params[3][0];

  switch (type) {
    case '-':
      if (params.size() != 4) {
        cerr << "Error: wrong number of arguments for remove line" << endl;
        return -1;
      } else {
        *node = NULL;
        return 0;
      }
    case 'f':
    case 'l':
      if (params.size() != 10) {
        cerr << "Error: wrong number of arguments for file/link line" << endl;
        return -1;
      }
      break;
    default:
      if (params.size() != 9) {
        cerr << "Error: wrong number of arguments for line" << endl;
        return -1;
      }
  }

  // Size
  if (sscanf(params[4].c_str(), "%lld", &size) != 1) {
    cerr << "Error: cannot decode size" << endl;
    return -1;
  }

  // Modification time
  if (sscanf(params[5].c_str(), "%ld", &mtime) != 1) {
    cerr << "Error: cannot decode mtime" << endl;
    return -1;
  }

  // User
  if (sscanf(params[6].c_str(), "%u", &uid) != 1) {
    cerr << "Error: cannot decode uid" << endl;
    return -1;
  }

  // Group
  if (sscanf(params[7].c_str(), "%u", &gid) != 1) {
    cerr << "Error: cannot decode gid" << endl;
    return -1;
  }

  // Permissions
  if (sscanf(params[8].c_str(), "%o", &mode) != 1) {
    cerr << "Error: cannot decode mode" << endl;
    return -1;
  }

  switch (type) {
    case 'f':
      *node = new File(path, type, mtime, size, uid, gid, mode,
        params[9].c_str());
      break;
    case 'l':
      *node = new Link(path, type, mtime, size, uid, gid, mode,
        params[9].c_str());
      break;
    default:
      *node = new Node(path, type, mtime, size, uid, gid, mode);
  }
  return 0;
}

char List::getLineType() {
  fetchLine();
  // Status is one of -2: eof!, -1: error, 0: eof, 1: ok
  switch (_d->line_status) {
    case unexpected_eof:
      // Unexpected eof
      return 'U';
    case empty:
      // Eof
      return 'E';
    case no_data:
      break;
    default:
      // Failure
      return 'F';
  }
  // Line should be re-used
  _d->line_status = new_data;
  if (_d->line[0] != '\t') {
    // Client
    return 'C';
  } else
  if (_d->line[1] != '\t') {
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

  if (date < 0) {
    got_path = true;
  } else {
    got_path = false;
  }

  while (true) {
    // Get line
    if (date == -2) {
      length = _d->line.length();
    } else {
      length = fetchLine(true);
    }

    if (length <= 0) {
      errno = EUCLEAN;
      if (length == 0) {
        cerr << "unexpected end of file" << endl;
      }
      return -1;
    }

    // End of file
    if (_d->line[0] == '#') {
      // Make sure we return end of file also if called again
      _d->line_status = empty;
      return 0;
    }

    // Client
    if (_d->line[0] != '\t') {
      if (client != NULL) {
        free(*client);
        *client = NULL;
        asprintf(client, "%s", &_d->line[0]);
        // Change end of line
        (*client)[length] = '\0';
      }
    } else

    // Path
    if (_d->line[1] != '\t') {
      if (path != NULL) {
        free(*path);
        *path = NULL;
        asprintf(path, "%s", &_d->line[1]);
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
        decodeDataLine(_d->line, path == NULL ? "" : *path, node, &ts);
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

int List::addClient(
    const char*     client) {
  ssize_t length = strlen(client);
  if ((Stream::write(client, length) != length)
      || (Stream::write("\n", 1) != 1)) {
    return -1;
  }
  _d->line_status = no_data;
  return 0;
}

int List::addPath(
    const char*     path) {
  ssize_t length = strlen(path);
  if ((Stream::write("\t", 1) != 1)
      || (Stream::write(path, length) != length)
      || (Stream::write("\n", 1) != 1)) {
    return -1;
  }
  _d->line_status = no_data;
  return 0;
}

int List::addData(
    time_t          timestamp,
    const Node*     node,
    bool            bufferize) {
  char* line = NULL;
  int   size;
  int   rc   = 0;

  if (node == NULL) {
    size = asprintf(&line, "\t\t%ld\t-", timestamp);
  } else {
    char* temp = NULL;
    size = asprintf(&temp, "\t\t%ld\t%c\t%lld\t%ld\t%u\t%u\t%o",
      timestamp, node->type(), node->size(), node->mtime(), node->uid(),
      node->gid(), node->mode());
    switch (node->type()) {
      case 'f': {
        const char* extra  = ((File*) node)->checksum();
        size = asprintf(&line, "%s\t%s", temp, extra);
      } break;
      case 'l': {
        const char* extra  = ((Link*) node)->link();
        size = asprintf(&line, "%s\t%s", temp, extra);
      } break;
      default:
        size = asprintf(&line, "%s", temp);
    }
    free(temp);
  }
  if ((timestamp == 0) && bufferize) {
    // Send line to search method, so it can deal with exception
    _d->line        = line;
    _d->line_status = new_data;
  } else
  if (Stream::putLine(line) != size) {
    rc = -1;
  } else {
    _d->line_status = no_data;
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

  size_t  path_len   = 0;   // Not set

  // Pre-set client comparison result
  if (client_l == NULL) {
    // Any client will match
    _d->client_cmp = 0;
  } else
  if (client_l[0] == '\0') {
    // No need to compare clients
    _d->client_cmp = 1;
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
  if ((_d->client_cmp > 0) || (path_l[0] == '\0')) {
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
    rc = fetchLine();

    // Failed
    if (rc <= 0) {
      // Unexpected end of file
      cerr << "Unexpected end of list" << endl;
      errno = EUCLEAN;
      return -1;
    }

    // End of file
    if (_d->line[0] == '#') {
      // Future searches will return eof too
      _d->line_status = empty;
    } else

    // Got a client
    if (_d->line[0] != '\t') {
      // Compare clientes
      if ((client_l != NULL) && (client_l[0] != '\0')) {
        _d->client_cmp = Path::compare(client_l, _d->line.c_str());
      }
      if (_d->client_cmp <= 0)  {
        if (_d->client_cmp < 0) {
          // Client exceeded
          _d->line_status = new_data;
        } else
        if ((client_l == NULL)
         || ((path_l != NULL) && (path_l[0] == '\0'))) {
          // Looking for client, found
          _d->line_status = cached_data;
        }
      }
      // Next line of data is active
      active_data_line = true;
    } else

    // Got a path
    if (_d->line[1] != '\t') {
      // Compare paths
      if ((_d->client_cmp <= 0) && (path_l != NULL) && (path_l[0] != '\0')) {
        if (path_len > 0) {
          if (_d->line.length() == (path_len + 2)) {
            path_cmp = Path::compare(path_l, &_d->line[1], path_len);
          } else {
            path_cmp = Path::compare(path_l, &_d->line[1]);
          }
        } else {
          path_cmp = Path::compare(path_l, _d->line.c_str());
        }
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
    } else

    // Got data
    {
      if (list != NULL) {
        // Deal with exception
        if (list->_d->line_status == new_data) {
          list->_d->line_status = no_data;
          // Copy start of line (timestamp)
          size_t pos = _d->line.substr(2).find('\t');
          if (pos == string::npos) {
            errno = EUCLEAN;
            return -1;
          }
          pos += 3;
          // Re-create line using previous data
          _d->line.resize(pos);
          _d->line.append(list->_d->line.substr(4));
        } else
        // Check for exception
        if (strncmp(_d->line.c_str(), "\t\t0\t", 4) == 0) {
          // Get only end of line
          list->_d->line        = _d->line;
          list->_d->line_status = new_data;
          // Do not copy: merge with following line
          continue;
        }

        // Check for expiry
        if (expire >= 0) {
          bool obsolete;
          vector<string> params;

          extractParams(_d->line, params, Stream::flags_empty_params, 0, "\t");

          // Check expiration
          if (active_data_line) {
            // Active line, never obsolete
            obsolete = false;
          } else
          if (expire == 0) {
            // Always obsolete
            obsolete = true;
          } else
          {
            // Check time
            time_t ts = 0;

            if ((params.size() > 3)
            &&  (sscanf(params[2].c_str(), "%ld", &ts) == 1)
            &&  (ts < expire)) {
              obsolete = true;
            } else {
              obsolete = false;
            }
          }

          // Deal with file data removal
          if ((params[3] == "f")
          &&  (params.size() == 10)) {
            if (! obsolete) {
              if (active != NULL) {
                active->push_back(params[9]);
              }
            } else {
              if (expired != NULL) {
                expired->push_back(params[9]);
              }
            }
          }

          // If obsolete, do not copy
          if (obsolete) {
            continue;
          }
        } // expire >= 0
      } // list != NULL
      // Next line of data is not active
      active_data_line = false;
    }

    // New data, add
    if (list != NULL) {
      if (_d->line_status != no_data) {
        if ((path_l != NULL) && (path_l[0] != '\0')) {
          // Write path
          if ((path_len != 0) && (list->write("\t", 1) < 0)) {
            // Could not write
            return -1;
          }
          if (list->Stream::putLine(path_l) < 0) {
            // Could not write
            return -1;
          }
        } else
        if ((client_l != NULL)
         && (client_l[0] != '\0')
         && (path_l != NULL)) {
          // Write client
          if (list->Stream::putLine(client_l) < 0) {
            // Could not write
            return -1;
          }
        }
      } else
      // Our data is here or after, so let's copy if required
      if (list->Stream::putLine(_d->line.c_str()) < 0) {
        // Could not write
        return -1;
      }
    }
    // If line status is not 1, return search status
    switch (_d->line_status) {
      case cached_data:
        // Found
        return 2;
      case new_data:
        // Exceeded
        return 1;
      case empty:
        // EOF
        return 0;
      default:
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
  _d->line_status = no_data;

  // Parse journal
  while (rc > 0) {
    int  rc_journal = journal.fetchLine();
    j_line_no++;

    // Failed
    if (rc_journal < 0) {
      cerr << "Error reading journal, line " << j_line_no << endl;
      rc = -1;
      break;
    }

    // End of file
    if ((journal._d->line[0] == '#') || (rc_journal == 0)) {
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
    if (journal._d->line[0] != '\t') {
      if (client.length() != 0) {
        int cmp = client.compare(journal._d->line.c_str());
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
      client = journal._d->line.c_str();
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
    if (journal._d->line[1] != '\t') {
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
        if (path.compare(journal._d->line.c_str()) > 0) {
          // Cannot go back
          cerr << "Path out of order in journal, line " << j_line_no << endl;
          errno = EUCLEAN;
          rc = -1;
          break;
        }
      }
      // Copy new path
      path = journal._d->line.c_str();
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
      if (((strncmp(journal._d->line.c_str(), "\t\t0\t", 4) != 0)
        || (list.putLine(journal._d->line.c_str()) < 0))
       && (Stream::putLine(journal._d->line.c_str()) < 0)) {
        // Could not write
        cerr << "Journal copy failed" << endl;
        rc = -1;
        break;
      }
    }
  }

  return rc;
}
