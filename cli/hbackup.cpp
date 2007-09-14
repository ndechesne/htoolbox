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
#include <signal.h>
#include <errno.h>
#include "hbackup.h"

using namespace std;

/* DEFAULTS */

/* Verbosity */
static int verbose = 0;

/* Configuration path */
static string default_config_path = "/etc/hbackup/hbackup.conf";

/* Signal received? */
static int killed = 0;

static void show_version(void) {
  cout << "(c) 2006-2007 Hervé Fache, version "
    << VERSION_MAJOR << "." << VERSION_MINOR;
  if (VERSION_BUGFIX != 0) {
    cout << "." << VERSION_BUGFIX;
  }
  if (BUILD != 0) {
    cout << " (build " << BUILD << ")";
  }
  cout << endl;
}

static void show_help(void) {
  show_version();
  cout << "Options are:" << endl;
  cout << " -h or --help     to print this help and exit" << endl;
  cout << " -V or --version  to print version and exit" << endl;
  cout << " -c or --config   to specify a configuration file other than \
/etc/hbackup/hbackup.conf" << endl;
  cout << " -s or --scan     to scan the database for missing data" << endl;
  cout << " -t or --check    to check the database for corrupted data" << endl;
  cout << " -v or --verbose  to be more verbose (also -vv and -vvv)" << endl;
  cout << " -C or --client   specify client to backup (more than one allowed)"
    << endl;
}

int hbackup::verbosity(void) {
  return verbose;
}

int hbackup::terminating(void) {
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
  int               argn              = 0;
  bool              scan              = false;
  bool              check             = false;
  bool              config_check      = false;
  bool              expect_configpath = false;
  bool              expect_client     = false;
  struct sigaction  action;
  hbackup::HBackup  hbackup;

  /* Set signal catcher */
  action.sa_handler = sighandler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;
  sigaction (SIGINT, &action, NULL);
  sigaction (SIGHUP, &action, NULL);
  sigaction (SIGTERM, &action, NULL);

  /* Analyse arguments */
  while (++argn < argc) {
    char letter = ' ';

    /* Get config path if request */
    if (expect_configpath) {
      config_path       = argv[argn];
      expect_configpath = false;
    }

    /* Get config path if request */
    if (expect_client) {
      hbackup.addClient(argv[argn]);
      expect_client = false;
    }

    /* -* */
    if (argv[argn][0] == '-') {
      /* --* */
      if (argv[argn][1] == '-') {
        if (! strcmp(&argv[argn][2], "config")) {
          letter = 'c';
        } else if (! strcmp(&argv[argn][2], "debug")) {
          letter = 'd';
        } else if (! strcmp(&argv[argn][2], "help")) {
          letter = 'h';
        } else if (! strcmp(&argv[argn][2], "restore")) {
          letter = 'r';
        } else if (! strcmp(&argv[argn][2], "scan")) {
          letter = 's';
        } else if (! strcmp(&argv[argn][2], "check")) {
          letter = 't';
        } else if (! strcmp(&argv[argn][2], "configcheck")) {
          letter = 'p';
        } else if (! strcmp(&argv[argn][2], "verbose")) {
          letter = 'v';
        } else if (! strcmp(&argv[argn][2], "client")) {
          letter = 'C';
        } else if (! strcmp(&argv[argn][2], "version")) {
          letter = 'V';
        }
      } else if (argv[argn][1] == 'v') {
        letter = argv[argn][1];
        if (argv[argn][2] == 'v') {
          verbose++;
          if (argv[argn][3] == 'v') {
            verbose++;
            if (argv[argn][4] != '\0') {
              letter = ' ';
            }
          } else if (argv[argn][3] != '\0') {
            letter = ' ';
          }
        } else if (argv[argn][2] != '\0') {
          letter = ' ';
        }
      } else if (argv[argn][2] == '\0') {
        letter = argv[argn][1];
      }

      switch (letter) {
        case 'c':
          expect_configpath = true;
          break;
        case 'd':
          verbose = 9;
          break;
        case 'h':
          show_help();
          return 0;
        case 's':
          scan = true;
          break;
        case 't':
          check = true;
          break;
        case 'p':
          config_check = true;
          break;
        case 'v':
          verbose++;
          break;
        case 'C':
          expect_client = true;
          break;
        case 'V':
          show_version();
          return 0;
        default:
          fprintf(stderr, "Unrecognised option: %s\n", argv[1]);
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

  // Read config before using HBackup
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
