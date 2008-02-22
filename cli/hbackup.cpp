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

using namespace hbackup;

// DEFAULTS

// Verbosity
static int verbose_level = 0;

// Signal received?
static int killed = 0;

int hbackup::terminating(const char* unused) {
  return killed;
}

void sighandler(int signal) {
  if (killed) {
    cout << "Already aborting..." << endl;
  } else {
    out(warning) << "Signal received, aborting..." << endl;
  }
  killed = signal;
}

class MyOutput : public StdOutput {
  public:
    virtual void failure(CmdLineInterface& c, ArgException& e) {
      out(error) << e.what() << endl;
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
      out(info) << c.getMessage() << " version " << c.getVersion() << endl;
    }
};

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

    // Initialize
    SwitchArg initSwitch("i", "initialize", "Initialize if needed",
      cmd, false);

    // List data
    SwitchArg listSwitch("l", "list", "List available data",
      cmd, false);

    // Check configuration
    SwitchArg pretendSwitch("p", "pretend", "Only check configuration",
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
      verbose_level = 0;
      setVerbosityLevel(hbackup::warning);
    }

    // Set verbosity level to verbose
    if (verboseSwitch.getValue()) {
      verbose_level = 1;
      setVerbosityLevel(hbackup::verbose);
    }

    // Set verbosity level to debug
    if (debugSwitch.getValue()) {
      verbose_level = 2;
      setVerbosityLevel(hbackup::debug);
    }

    // Report specified clients
    if (clientArg.getValue().size() != 0) {
      for (vector<string>::const_iterator i = clientArg.getValue().begin();
          i != clientArg.getValue().end(); i++) {
        hbackup.addClient(i->c_str());
      }
    }

    // Check backup mode
    if (userSwitch.getValue()) {
      user_mode = true;
    }
    // Read config before using HBackup
    if (user_mode) {
      if (hbackup.open(getenv("HOME"), true)) {
        return 2;
      }
    } else
    if (hbackup.open(configArg.getValue().c_str(), false)) {
      return 2;
    }
    // Check that data referenced in DB exists
    if (scanSwitch.getValue()) {
      out(info) << "Scanning database" << endl;
      if (hbackup.scan()) {
        return 3;
      }
    } else
    // Check that data referenced in DB exists and is not corrupted
    if (checkSwitch.getValue()) {
      out(info) << "Checking database" << endl;
      if (hbackup.check()) {
        return 3;
      }
    } else
    // List DB contents
    if (listSwitch.getValue()) {
      out(info) << "Showing list" << endl;
      string client;
      string path;
      if (clientArg.getValue().size() > 1) {
        out(error) << "Maximum one client allowed" << endl;
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
      for (list<string>::iterator i = records.begin(); i != records.end();
          i++) {
        cout << " " << *i << endl;
      }
    } else
    // Restore data
    if (restoreArg.getValue().size() != 0) {
      out(info) << "Restoring" << endl;
      string client;
      if (clientArg.getValue().size() > 1) {
        out(error) << "Maximum one client allowed" << endl;
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
      out(info) << "Backing up in ";
      if (user_mode) {
        out(info, 0) << "user";
      } else {
        out(info, 0) << "server";
      }
      out(info, 0) << " mode" << endl;
      if (hbackup.backup(initSwitch.getValue())) {
        return 3;
      }
    }
    hbackup.close();
  } catch (ArgException &e) {
    out(error) << e.error() << " for arg " << e.argId() << endl;
  };
  return 0;
}
