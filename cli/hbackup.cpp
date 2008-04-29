/*
     Copyright (C) 2006-2008  Herve Fache

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

#include <tclap/CmdLine.h>

using namespace TCLAP;

#include <list>

using namespace std;

#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#include "config.h"
#include "hbackup.h"

// Verbosity
static hbackup::VerbosityLevel verbose_level = hbackup::info;

// Signals
void sighandler(int signal) {
  if (hbackup::aborting()) {
    cout << "Already aborting..." << endl;
  } else {
    cout << "Warning: signal received, aborting..." << endl;
    hbackup::abort();
 }
}

static list<string>* _records = NULL;

static void setValueReceiver(list<string>* values) {
  _records = values;
}

static void resetValueReceiver() {
  _records = NULL;
}

// Messages
static void output(
    hbackup::VerbosityLevel  level,
    hbackup::MessageType     type,
    const char*     message = NULL,
    int             number  = -1,
    const char*     prepend = NULL) {
  static unsigned int _size_to_overwrite = 0;
  if (level > verbose_level) {
    return;
  }
  stringstream s;
  if ((type != hbackup::msg_standard) || (number < 0)) {
    switch (level) {
      case hbackup::alert:
        s << "ALERT! ";
        break;
      case hbackup::error:
        s << "Error: ";
        break;
      case hbackup::warning:
        s << "Warning: ";
        break;
      default:;
    }
  } else if (number > 0) {
    string arrow;
    arrow = " ";
    arrow.append(number, '-');
    arrow.append("> ");
    s << arrow;
  }
  if (prepend != NULL) {
    s << prepend;
    if (number >= -1) {
      s << ":";
    }
    s << " ";
  }
  bool add_colon = false;
  switch (type) {
    case hbackup::msg_errno:
      s << strerror(number);
      add_colon = true;
      break;
    case hbackup::msg_number:
      s << number;
      add_colon = true;
      break;
    default:;
  }
  if (message != NULL) {
    if (add_colon) {
      s << ": ";
    }
    s << message;
  }
  if (number == -3) {
    if (s.str().size() > _size_to_overwrite) {
      _size_to_overwrite = s.str().size();
    } else
    if (s.str().size() < _size_to_overwrite) {
      string blank;
      blank.append(_size_to_overwrite - s.str().size(), ' ');
      _size_to_overwrite = s.str().size();
      s << blank;
    }
    s << '\r';
  } else {
    if (_size_to_overwrite > 0) {
      string blank;
      blank.append(_size_to_overwrite, ' ');
      cout << blank << '\r' << flush;
      _size_to_overwrite = 0;
    }
    s << endl;
  }
  switch (level) {
    case hbackup::value:
      if (_records != NULL) {
        string value = s.str();
        value.erase(value.size() - 1);
        _records->push_back(value);
      } else {
        cout << '\t' << s.str() << flush;
      }
      break;
    case hbackup::alert:
    case hbackup::error:
      cerr << s.str() << flush;
      break;
    case hbackup::warning:
    case hbackup::info:
    case hbackup::verbose:
    case hbackup::debug:
      cout << s.str() << flush;
  }
}

// Progress
static void progress(long long previous, long long current, long long total) {
  if ((current != total) || (previous != 0)) {
    stringstream s;
    s << "Done: " << setw(5) << setiosflags(ios::fixed) << setprecision(1)
      << 100.0 * current /total << "%";
    output(hbackup::info, hbackup::msg_standard, s.str().c_str(), -3);
  }
}

class MyOutput : public StdOutput {
  public:
    virtual void failure(CmdLineInterface& c, ArgException& e) {
      cerr << "Error: " << e.what() << endl;
      usage(c);
    }

    virtual void usage(CmdLineInterface& c) {
      cout << c.getMessage() << endl;
      list<Arg*> args = c.getArgList();
      for (ArgListIterator i = args.begin(); i != args.end(); i++) {
        cout << "  " << (*i)->longID() << endl;
        cout << "    " << (*i)->getDescription() << endl;
        cout << endl;
      }
    }

    virtual void version(CmdLineInterface& c) {
      cout << c.getMessage() << " version " << c.getVersion() << endl;
    }
};

static bool         use_clients = false;
static list<string> clients;

int main(int argc, char **argv) {
  hbackup::HBackup hbackup;

  // Set progress callback function
  hbackup::setProgressCallback(progress);
  setMessageCallback(output);

  // Accept hubackup as a replacement for hbackup -u
  bool   user_mode = false;
  string prog_name = basename(argv[0]);
  if (prog_name == "hubackup") {
    user_mode = true;
  }

  // Signal handler
  struct sigaction action;
  action.sa_handler = sighandler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;
  sigaction (SIGINT, &action, NULL);
  sigaction (SIGHUP, &action, NULL);
  sigaction (SIGTERM, &action, NULL);

  // Analyse arguments
  try {
    // Description
    CmdLine cmd(PACKAGE_NAME " (c) 2006-2008 Hervé Fache", ' ', VERSION);
    MyOutput output;
    cmd.setOutput(&output);

    // Configuration file
    ValueArg<string> configArg("c", "config",
      "Specify configuration file", false, "/etc/hbackup/hbackup.conf",
      "file name", cmd);

    // Specify client(s)
    MultiArg<string> clientArg("C", "client", "Specify client name",
      false, "client", cmd);

    // Debug
    SwitchArg debugSwitch("d", "debug", "Print debug information",
      cmd, false);

    // Specify date
    ValueArg<time_t> dateArg("D", "date", "Specify date",
      false, 0, "UNIX epoch", cmd);

    // Debug
    SwitchArg fixSwitch("f", "fix", "Fix backup database",
      cmd, false);

    // Initialize
    SwitchArg initSwitch("i", "initialize", "Initialize if needed",
      cmd, false);

    // List data
    SwitchArg listSwitch("l", "list", "List available data",
      cmd, false);

    // Specify path
    ValueArg<string> pathArg("P", "path", "Specify path",
      false, "", "path", cmd);

    // Quiet
    SwitchArg quietSwitch("q", "quiet", "Be quiet",
      cmd, false);

    // Restore
    ValueArg<string> restoreArg("r", "restore", "Restore to directory",
      false, "", "directory", cmd);

    // Scan DB
    SwitchArg scanSwitch("s", "scan", "Scan DB for missing data",
      cmd, false);

    // Check DB
    SwitchArg checkSwitch("t", "check", "Check DB for corrupted data",
      cmd, false);

    // User mode backup
    SwitchArg userSwitch("u", "user", "Perform user-mode backup",
      cmd, false);

    // Verbose
    SwitchArg verboseSwitch("v", "verbose", "Be verbose about actions",
      cmd, false);

    // Parse command line
    cmd.parse(argc, argv);


    // Set verbosity level to info
    setVerbosityLevel(hbackup::info);

    // Set verbosity level to quiet
    if (quietSwitch.getValue()) {
      verbose_level = hbackup::warning;
    }

    // Set verbosity level to verbose
    if (verboseSwitch.getValue()) {
      verbose_level = hbackup::verbose;
    }

    // Set verbosity level to debug
    if (debugSwitch.getValue()) {
      verbose_level = hbackup::debug;
    }

    // Check mode
    if (userSwitch.getValue()) {
      user_mode = true;
    }
    // Read config before using HBackup
    if (user_mode) {
      if (hbackup.open(getenv("HOME"), true)) {
        return 2;
      }
    } else {
      if (hbackup.open(configArg.getValue().c_str(), false)) {
        return 2;
      }
    }
    // If listing or restoring., need list of clients
    if ((restoreArg.getValue().size() != 0) || (listSwitch.getValue())) {
      setValueReceiver(&clients);
      if (hbackup.getList() != 0) {
        return 1;
      } else {
        use_clients = true;
      }
      resetValueReceiver();
    }
    // Report specified clients
    for (vector<string>::const_iterator i = clientArg.getValue().begin();
        i != clientArg.getValue().end(); i++) {
      if (use_clients) {
        // Length of name excluding last character
        int length = strlen(i->c_str()) - 1;
        if (length < 0) {
          cerr << "Error: Empty client name" << endl;
          return 1;
        }
        bool wildcard = false;
        if ((*i)[length] == '*') {
          wildcard = true;
        }
        // Make sure we know this client
        for (list<string>::iterator client = clients.begin();
            client != clients.end(); client++) {
          if ((wildcard && ((length == 0)
            || (strncmp(i->c_str(), client->c_str(), length) == 0)))
          || (strcmp(i->c_str(), client->c_str()) == 0)) {
            hbackup.addClient(client->c_str());
          }
        }
      } else {
        // Just add
        hbackup.addClient(i->c_str());
      }
    }
    if (verbose_level >= hbackup::verbose) {
      cout << "Configuration:" << endl;
      hbackup.show(1);
    }
    // Fix any interrupted backup
    if (fixSwitch.getValue()) {
      if (verbose_level >= hbackup::info) {
        cout << "Fixing database" << endl;
      }
      if (hbackup.fix()) {
        return 3;
      }
    } else
    // Check that data referenced in DB exists
    if (scanSwitch.getValue()) {
      if (verbose_level >= hbackup::info) {
        cout << "Scanning database" << endl;
      }
      if (hbackup.scan()) {
        return 3;
      }
    } else
    // Check that data in DB is not corrupted
    if (checkSwitch.getValue()) {
      if (verbose_level >= hbackup::info) {
        cout << "Checking database" << endl;
      }
      if (hbackup.check()) {
        return 3;
      }
    } else
    // List DB contents
    if (listSwitch.getValue()) {
      if (verbose_level >= hbackup::info) {
        cout << "Showing list" << endl;
      }
      string path;
      if (user_mode && (clientArg.getValue().size() > 0)) {
        cerr << "Error: Wrong number of clients" << endl;
        return 1;
      }
      if (hbackup.getList(pathArg.getValue().c_str(), dateArg.getValue())) {
        return 3;
      }
    } else
    // Restore data
    if (restoreArg.getValue().size() != 0) {
      if (verbose_level >= hbackup::info) {
        cout << "Restoring" << endl;
      }
      if (clientArg.getValue().size() != (user_mode ? 0 : 1)) {
        cerr << "Error: Wrong number of clients" << endl;
        return 1;
      }
      if (hbackup.restore(restoreArg.getValue().c_str(),
          pathArg.getValue().c_str(), dateArg.getValue())) {
        return 3;
      }
    } else
    // Backup
    {
      if (verbose_level >= hbackup::info) {
        cout << "Backing up in " << (user_mode ? "user" : "server") << " mode"
          << endl;
      }
      if (hbackup.backup(initSwitch.getValue())) {
        return 3;
      }
    }
    hbackup.close();
  } catch (ArgException &e) {
    cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
  };
  return 0;
}
