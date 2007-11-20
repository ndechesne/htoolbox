/*
     Copyright (C) 2006-2007  Herve Fache

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

// DEFAULTS

// Verbosity
static int verbose = 0;

// Signal received?
static int killed = 0;

int hbackup::verbosity(void) {
  return verbose;
}

int hbackup::terminating(const char* unused) {
  return killed;
}

void sighandler(int signal) {
  if (killed) {
    cerr << "Already aborting..." << endl;
  } else {
    cerr << "Signal received, aborting..." << endl;
  }
  killed = signal;
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

int main(int argc, char **argv) {
  hbackup::HBackup hbackup;

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
    CmdLine cmd(PACKAGE_NAME " (c) 2006-2007 Hervé Fache", ' ', VERSION);
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

    // List data
    SwitchArg listSwitch("l", "list", "List available data",
      cmd, false);

    // Check configuration
    SwitchArg pretendSwitch("p", "pretend", "Only check configuration",
      cmd, false);

    // Specify path
    ValueArg<string> pathArg("P", "path", "Specify path",
      false, "", "path", cmd);

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

    // Set verbosity level to verbose
    if (verboseSwitch.getValue()) {
      verbose = 1;
    }

    // Set verbosity level to debug
    if (debugSwitch.getValue()) {
      verbose = 2;
    }

    // Report specified clients
    if (clientArg.getValue().size() != 0) {
      for (vector<string>::const_iterator i = clientArg.getValue().begin();
          i != clientArg.getValue().end(); i++) {
        hbackup.addClient(i->c_str());
      }
    }

    // Read config before using HBackup
    if (userSwitch.getValue()) {
      if (hbackup.setUserPath(getenv("HOME"))) {
        return 2;
      }
    } else
    if (hbackup.readConfig(configArg.getValue().c_str())) {
      return 2;
    }
    // Check that data referenced in DB exists
    if (scanSwitch.getValue()) {
      if (hbackup::verbosity() > 0) {
        cout << "Scanning database" << endl;
      }
      if (hbackup.check()) {
        return 3;
      }
    } else
    // Check that data referenced in DB exists and is not corrupted
    if (checkSwitch.getValue()) {
      if (hbackup::verbosity() > 0) {
        cout << "Checking database" << endl;
      }
      if (hbackup.check(true)) {
        return 3;
      }
    } else
    // List DB contents
    if (listSwitch.getValue()) {
      if (hbackup::verbosity() > 0) {
        cout << "Showing list" << endl;
      }
      string client;
      string path;
      if (clientArg.getValue().size() > 1) {
        cerr << "Error: maximum one client allowed" << endl;
        return 1;
      } else
      if (clientArg.getValue().size() != 0) {
        client = clientArg.getValue()[0];
      }
      list<string> records;
      if (hbackup.getList(records, client.c_str(), pathArg.getValue().c_str(),
          dateArg.getValue())) {
        return 3;
      }
      for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
        cout << " " << *i << endl;
      }
    } else
    // Restore data
    if (restoreArg.getValue().size() != 0) {
      if (hbackup::verbosity() > 0) {
        cout << "Restoring" << endl;
      }
      string client;
      if (clientArg.getValue().size() > 1) {
        cerr << "Error: maximum one client allowed" << endl;
        return 1;
      } else
      if (clientArg.getValue().size() != 0) {
        client = clientArg.getValue()[0];
      }
      if (hbackup.restore(restoreArg.getValue().c_str(), client.c_str(),
        pathArg.getValue().c_str(), dateArg.getValue())) {
        return 3;
      }
    } else
    // Backup
    {
      if (hbackup::verbosity() > 0) {
        cout << "Backing up" << endl;
      }
      if (hbackup.backup()) {
        return 3;
      }
    }
    hbackup.close();
  } catch (ArgException &e) {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
  };
  return 0;
}
