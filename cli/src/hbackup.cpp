/*
     Copyright (C) 2006-2010  Herve Fache

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
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "config.h"

#include "hbackup.h"
#include "report.h"

// Signals
void sighandler(int signal) {
  if (hbackup::aborting()) {
    hlog_error("Already aborting...");
  } else {
    hlog_warning("signal %d received, aborting...", signal);
    hbackup::abort();
 }
}

// Progress
static void copy_prog(long long previous, long long current, long long total) {
  if ((current != total) || (previous != 0)) {
    hlog_verbose_temp("File copy progress: %5.1lf%%",
      100 * static_cast<double>(current) / static_cast<double>(total));
  }
}

static void list_prog(long long previous, long long current, long long total) {
  if ((current != total) || (previous != 0)) {
    hlog_verbose_temp("List read progress: %5.1lf%%",
      100 * static_cast<double>(current) / static_cast<double>(total));
  }
}

class MyOutput : public StdOutput {
  public:
    virtual void failure(CmdLineInterface& c, ArgException& e) {
      cerr << "Error: " << e.what() << endl;
      usage(c);
      exit(1);
    }

    virtual void usage(CmdLineInterface& c) {
      cout << c.getMessage() << endl;
      list<Arg*> args = c.getArgList();
      ArgListIterator i = args.end();
      while (i-- != args.begin()) {
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
    CmdLine cmd(PACKAGE_NAME " (c) 2006-2010 Hervé Fache", ' ',
      PACKAGE_VERSION);
    MyOutput output;
    cmd.setOutput(&output);

    // Configuration file
    ValueArg<string> configArg("c", "config",
      "Specify configuration file", false, HBACKUP_CONFIG, "file name", cmd);

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

    // Hard links
    SwitchArg hardSwitch("H", "hardlinks", "Create hierarchy of hard links",
      cmd, false);

    // Initialize
    SwitchArg initSwitch("i", "initialize", "Initialize if needed",
      cmd, false);

    // List data
    SwitchArg listSwitch("l", "list", "List available data",
      cmd, false);

    // Symbolic links
    SwitchArg symSwitch("L", "symlinks", "Create hierarchy of symlinks",
      cmd, false);

    // No log
    SwitchArg noLogSwitch("N", "nolog", "Do not create a log file",
      cmd, false);

    // Specify path
    ValueArg<string> pathArg("P", "path", "Specify path",
      false, "", "path", cmd);

    // Quiet
    SwitchArg quietSwitch("q", "quiet", "Be quiet",
      cmd, false);

    // Very quiet
    SwitchArg veryQuietSwitch("Q", "veryquiet", "Do not even print warnings",
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
    htools::report.setLevel(htools::info);

    // Set verbosity level to very quiet
    if (veryQuietSwitch.getValue()) {
      htools::report.setLevel(htools::error);
    }

    // Set verbosity level to quiet
    if (quietSwitch.getValue()) {
      htools::report.setLevel(htools::warning);
    }

    // Set verbosity level to verbose
    if (verboseSwitch.getValue()) {
      htools::report.setLevel(htools::verbose);
    }

    // Set verbosity level to debug
    if (debugSwitch.getValue()) {
      htools::report.setLevel(htools::debug);
    }

    // Set file copy progress callback function
    if (hlog_is_worth(htools::verbose)) {
      hbackup::setCopyProgressCallback(copy_prog);
    }

    // Set previous list read progress callback function
    if (hlog_is_worth(htools::verbose)) {
      hbackup::setListProgressCallback(list_prog);
    }

    // Check mode
    if (userSwitch.getValue()) {
      user_mode = true;
    }
    // Read config before using HBackup
    if (user_mode) {
      if (hbackup.open(getenv("HOME"), noLogSwitch.getValue(), true)) {
        return 2;
      }
    } else {
      if (hbackup.open(configArg.getValue().c_str(), noLogSwitch.getValue())) {
        return 2;
      }
    }
    // If listing or restoring., need list of clients
    if ((restoreArg.getValue().size() != 0) || (listSwitch.getValue())) {
      if (hbackup.list(&clients) != 0) {
        return 1;
      } else {
        use_clients = true;
      }
    }
    // Report specified clients
    for (vector<string>::const_iterator i = clientArg.getValue().begin();
        i != clientArg.getValue().end(); i++) {
      if (use_clients) {
        // Length of name excluding last character
        ssize_t length = strlen(i->c_str()) - 1;
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
    hbackup.show(1);
    // Fix any interrupted backup
    if (fixSwitch.getValue()) {
      hlog_info("Fixing database");
      if (hbackup.fix()) {
        return 3;
      }
    } else
    // Check that data referenced in DB exists
    if (scanSwitch.getValue()) {
      hlog_info("Scanning database");
      if (hbackup.scan()) {
        return 3;
      }
    } else
    // Check that data in DB is not corrupted
    if (checkSwitch.getValue()) {
      hlog_info("Checking database");
      if (hbackup.check()) {
        return 3;
      }
    } else
    // List DB contents
    if (listSwitch.getValue()) {
      hlog_info("Showing list");
      string path;
      if (user_mode && (clientArg.getValue().size() > 0)) {
        cerr << "Error: Wrong number of clients" << endl;
        return 1;
      }
      list<string> names;
      if (hbackup.list(&names, hbackup::HBackup::none,
          pathArg.getValue().c_str(), dateArg.getValue())) {
        return 3;
      }
      for (list<string>::const_iterator i = names.begin(); i != names.end(); ++i) {
        cout << "\t" << *i << endl;
      }
    } else
    // Restore data
    if (restoreArg.getValue().size() != 0) {
      hlog_info("Restoring");
      if (clientArg.getValue().size() != (user_mode ? 0 : 1)) {
        cerr << "Error: Wrong number of clients" << endl;
        return 1;
      }
      if (hardSwitch.getValue() && symSwitch.getValue()) {
        cerr << "Error: Cannot create both hard and symbolic links" << endl;
        return 1;
      }
      hbackup::HBackup::LinkType links;
      if (symSwitch.getValue()) {
        links = hbackup::HBackup::symbolic;
      } else
      if (hardSwitch.getValue()) {
        links = hbackup::HBackup::hard;
      } else
      {
        links = hbackup::HBackup::none;
      }
      if (hbackup.restore(restoreArg.getValue().c_str(), links,
          pathArg.getValue().c_str(), dateArg.getValue())) {
        return 3;
      }
    } else
    // Backup
    {
      hlog_info("Backing up in %s mode", user_mode ? "user" : "server");
      if (hbackup.backup(initSwitch.getValue())) {
        return 3;
      }
    }
    hbackup.close();
  } catch (ArgException &e) {
    cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
    return 1;
  };
  return 0;
}