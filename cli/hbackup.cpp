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

#include <iostream>
#include <string>
#include <list>

using namespace std;

#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#include "hbackup.h"

// DEFAULTS

// Verbosity
static int verbose = 0;

// Configuration path
static string default_config_path = "/etc/hbackup/hbackup.conf";

// Signal received?
static int killed = 0;

static void show_version(void) {
  cout << "HBackup (c) 2006-2007 Hervé Fache, version "<< VERSION << endl;
}

static void show_help(void) {
  show_version();
  cout << "Options are:" << endl;
  cout << " -h or --help     print this help and exit" << endl;
  cout << " -V or --version  print version and exit" << endl;
  cout << " -c or --config   specify a configuration file other than \
/etc/hbackup/hbackup.conf" << endl;
  cout << " -d or --debug    print debug information" << endl;
  cout << " -l or --list     list the available data" << endl;
  cout << " -r or --restore  actually restore a path" << endl;
  cout << " -s or --scan     scan the database for missing data" << endl;
  cout << " -t or --check    check the database for corrupted data" << endl;
  cout << " -u or --user     user-mode backup" << endl;
  cout << " -v or --verbose  print progress information" << endl;
  cout << " -C or --client   specify client to backup (more than one allowed)"
    << endl;
}

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

int main(int argc, char **argv) {
  string            config_path       = "";
  string            destination       = "";
  string            prefix            = "";
  string            path              = "";
  time_t            date              = 0;
  int               argn              = 0;
  bool              expect_configpath = false;
  // Administration
  bool              scan              = false;
  bool              check             = false;
  bool              config_check      = false;
  // Backup
  bool              user              = false;
  bool              expect_client     = false;
  // Restoration
  bool              show_list         = false;
  bool              restore           = false;
  bool              expect_dest       = false;
  bool              expect_prefix     = false;
  bool              expect_path       = false;
  bool              expect_date       = false;
  // Signal handler
  struct sigaction  action;
  hbackup::HBackup  hbackup;

  // Set signal catcher
  action.sa_handler = sighandler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;
  sigaction (SIGINT, &action, NULL);
  sigaction (SIGHUP, &action, NULL);
  sigaction (SIGTERM, &action, NULL);

  // Analyse arguments
  while (++argn < argc) {
    char letter = ' ';

    // Get config path if requested
    if (expect_configpath) {
      config_path       = argv[argn];
      expect_configpath = false;
    }

    // Get client name if requested
    if (expect_client) {
      hbackup.addClient(argv[argn]);
      expect_client = false;
    }

    // Get client name if requested
    if (expect_dest) {
      destination   = argv[argn];
      expect_dest   = false;
      expect_prefix = true;
    } else

    // Get prefix if requested
    if (expect_prefix) {
      prefix        = argv[argn];
      expect_prefix = false;
      expect_date   = true;
    } else

    // Get date if requested
    if (expect_date) {
      if (sscanf(argv[argn], "%ld", &date) != 1) {
        cerr << "Cannot read date from " << argv[argn] << endl;;
        return 2;
      }
      expect_date = false;
      expect_path = true;
    } else

    // Get path if requested
    if (expect_path) {
      path = argv[argn];
      expect_path = false;
    }

    // -*
    if (argv[argn][0] == '-') {
      // --*
      if (argv[argn][1] == '-') {
        if (! strcmp(&argv[argn][2], "config")) {
          letter = 'c';
        } else if (! strcmp(&argv[argn][2], "debug")) {
          letter = 'd';
        } else if (! strcmp(&argv[argn][2], "help")) {
          letter = 'h';
        } else if (! strcmp(&argv[argn][2], "list")) {
          letter = 'l';
        } else if (! strcmp(&argv[argn][2], "restore")) {
          letter = 'r';
        } else if (! strcmp(&argv[argn][2], "scan")) {
          letter = 's';
        } else if (! strcmp(&argv[argn][2], "check")) {
          letter = 't';
        } else if (! strcmp(&argv[argn][2], "configcheck")) {
          letter = 'p';
        } else if (! strcmp(&argv[argn][2], "user")) {
          letter = 'u';
        } else if (! strcmp(&argv[argn][2], "verbose")) {
          letter = 'v';
        } else if (! strcmp(&argv[argn][2], "client")) {
          letter = 'C';
        } else if (! strcmp(&argv[argn][2], "version")) {
          letter = 'V';
        } else if (! strcmp(&argv[argn][2], "")) {
          break;
        }
      } else if (argv[argn][2] == '\0') {
        letter = argv[argn][1];
      }

      switch (letter) {
        case 'c':
          expect_configpath = true;
          break;
        case 'd':
          verbose = 2;
          break;
        case 'h':
          show_help();
          return 0;
        case 'l':
          show_list     = true;
          expect_prefix = true;
          break;
        case 'r':
          restore     = true;
          expect_dest = true;
          break;
        case 's':
          scan = true;
          break;
        case 't':
          check = true;
          break;
        case 'p':
          config_check = true;
          break;
        case 'u':
          user = true;
          break;
        case 'v':
          verbose = 1;
          break;
        case 'C':
          expect_client = true;
          break;
        case 'V':
          show_version();
          return 0;
        default:
          cerr << "Unrecognised option: " << argv[argn] << endl;;
          return 2;
      }
    }
  }

  if (expect_configpath) {
    cerr << "Missing config path" << endl;
    return 2;
  }

  if (config_path == "") {
    config_path = default_config_path;
  }

  if (hbackup::verbosity() > 0) {
    show_version();
  }

  // Read config before using HBackup
  if (user) {
    if (hbackup.setUserPath(getenv("HOME"))) {
      return 2;
    }
  } else
  if (hbackup.readConfig(config_path.c_str())) {
    return 2;
  }
  // Check that data referenced in DB exists
  if (scan) {
    if (hbackup::verbosity() > 0) {
      cout << "Scanning database" << endl;
    }
    if (hbackup.check()) {
      return 3;
    }
  } else
  // Check that data referenced in DB exists and is not corrupted
  if (check) {
    if (hbackup::verbosity() > 0) {
      cout << "Checking database" << endl;
    }
    if (hbackup.check(true)) {
      return 3;
    }
  } else
  // List DB contents
  if (show_list) {
    if (hbackup::verbosity() > 0) {
      cout << "Showing list" << endl;
    }
    list<string> records;
    if (hbackup.getList(records, prefix.c_str(), path.c_str(), date)) {
      return 3;
    }
    for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
      cout << " " << *i << endl;
    }
  } else
  // Restore data
  if (restore) {
    if (hbackup::verbosity() > 0) {
      cout << "Restoring" << endl;
    }
    if (hbackup.restore(destination.c_str(), prefix.c_str(), path.c_str(),
          date)) {
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
  return 0;
}
