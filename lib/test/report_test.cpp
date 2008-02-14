/*
     Copyright (C) 2008  Herve Fache

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

using namespace std;

#include "hbackup.h"
#include "report.h"

using namespace hbackup;

int hbackup::verbosity(void) {
  return 0;
}

int hbackup::terminating(const char* function) {
  return 0;
}

int main(void) {
  cout << "Report tests" << endl;

  cout << endl << "Default behaviour" << endl;
  out(info) << "default level" << endl;
  out(alert) << "alert level" << endl;
  out(alert, 0) << "alert 0 level" << endl;
  out(error) << "error level" << endl;
  out(error, 0) << "error 0 level" << endl;
  out(warning) << "warning level" << endl;
  out(warning, 0) << "warning 0 level" << endl;
  out(warning, 1) << "warning 1 level" << endl;
  out(warning, 2) << "warning 2 level" << endl;
  out(info) << "info level" << endl;
  out(info, 0) << "info 0 level" << endl;
  out(info, 1) << "info 1 level" << endl;
  out(info, 2) << "info 2 level" << endl;
  out(verbose) << "verbose level" << endl;
  out(verbose, 0) << "verbose 0 level" << endl;
  out(verbose, 1) << "verbose 1 level" << endl;
  out(verbose, 2) << "verbose 2 level" << endl;
  out(debug) << "debug level" << endl;
  out(debug, 0) << "debug 0 level" << endl;
  out(debug, 1) << "debug 1 level" << endl;
  out(debug, 2) << "debug 2 level" << endl;

  cout << endl << "Verbosity level: alert" << endl;
  setVerbosityLevel(alert);
  out(info) << "default level" << endl;
  out(alert) << "alert level" << endl;
  out(alert, 0) << "alert 0 level" << endl;
  out(error) << "error level" << endl;
  out(error, 0) << "error 0 level" << endl;
  out(warning) << "warning level" << endl;
  out(warning, 0) << "warning 0 level" << endl;
  out(warning, 1) << "warning 1 level" << endl;
  out(warning, 2) << "warning 2 level" << endl;
  out(info) << "info level" << endl;
  out(info, 0) << "info 0 level" << endl;
  out(info, 1) << "info 1 level" << endl;
  out(info, 2) << "info 2 level" << endl;
  out(verbose) << "verbose level" << endl;
  out(verbose, 0) << "verbose 0 level" << endl;
  out(verbose, 1) << "verbose 1 level" << endl;
  out(verbose, 2) << "verbose 2 level" << endl;
  out(debug) << "debug level" << endl;
  out(debug, 0) << "debug 0 level" << endl;
  out(debug, 1) << "debug 1 level" << endl;
  out(debug, 2) << "debug 2 level" << endl;

  cout << endl << "Verbosity level: error" << endl;
  setVerbosityLevel(error);
  out(info) << "default level" << endl;
  out(alert) << "alert level" << endl;
  out(alert, 0) << "alert 0 level" << endl;
  out(error) << "error level" << endl;
  out(error, 0) << "error 0 level" << endl;
  out(warning) << "warning level" << endl;
  out(warning, 0) << "warning 0 level" << endl;
  out(warning, 1) << "warning 1 level" << endl;
  out(warning, 2) << "warning 2 level" << endl;
  out(info) << "info level" << endl;
  out(info, 0) << "info 0 level" << endl;
  out(info, 1) << "info 1 level" << endl;
  out(info, 2) << "info 2 level" << endl;
  out(verbose) << "verbose level" << endl;
  out(verbose, 0) << "verbose 0 level" << endl;
  out(verbose, 1) << "verbose 1 level" << endl;
  out(verbose, 2) << "verbose 2 level" << endl;
  out(debug) << "debug level" << endl;
  out(debug, 0) << "debug 0 level" << endl;
  out(debug, 1) << "debug 1 level" << endl;
  out(debug, 2) << "debug 2 level" << endl;

  cout << endl << "Verbosity level: warning" << endl;
  setVerbosityLevel(warning);
  out(info) << "default level" << endl;
  out(alert) << "alert level" << endl;
  out(alert, 0) << "alert 0 level" << endl;
  out(error) << "error level" << endl;
  out(error, 0) << "error 0 level" << endl;
  out(warning) << "warning level" << endl;
  out(warning, 0) << "warning 0 level" << endl;
  out(warning, 1) << "warning 1 level" << endl;
  out(warning, 2) << "warning 2 level" << endl;
  out(info) << "info level" << endl;
  out(info, 0) << "info 0 level" << endl;
  out(info, 1) << "info 1 level" << endl;
  out(info, 2) << "info 2 level" << endl;
  out(verbose) << "verbose level" << endl;
  out(verbose, 0) << "verbose 0 level" << endl;
  out(verbose, 1) << "verbose 1 level" << endl;
  out(verbose, 2) << "verbose 2 level" << endl;
  out(debug) << "debug level" << endl;
  out(debug, 0) << "debug 0 level" << endl;
  out(debug, 1) << "debug 1 level" << endl;
  out(debug, 2) << "debug 2 level" << endl;

  cout << endl << "Verbosity level: info" << endl;
  setVerbosityLevel(info);
  out(info) << "default level" << endl;
  out(alert) << "alert level" << endl;
  out(alert, 0) << "alert 0 level" << endl;
  out(error) << "error level" << endl;
  out(error, 0) << "error 0 level" << endl;
  out(warning) << "warning level" << endl;
  out(warning, 0) << "warning 0 level" << endl;
  out(warning, 1) << "warning 1 level" << endl;
  out(warning, 2) << "warning 2 level" << endl;
  out(info) << "info level" << endl;
  out(info, 0) << "info 0 level" << endl;
  out(info, 1) << "info 1 level" << endl;
  out(info, 2) << "info 2 level" << endl;
  out(verbose) << "verbose level" << endl;
  out(verbose, 0) << "verbose 0 level" << endl;
  out(verbose, 1) << "verbose 1 level" << endl;
  out(verbose, 2) << "verbose 2 level" << endl;
  out(debug) << "debug level" << endl;
  out(debug, 0) << "debug 0 level" << endl;
  out(debug, 1) << "debug 1 level" << endl;
  out(debug, 2) << "debug 2 level" << endl;

  cout << endl << "Verbosity level: verbose" << endl;
  setVerbosityLevel(verbose);
  out(info) << "default level" << endl;
  out(alert) << "alert level" << endl;
  out(alert, 0) << "alert 0 level" << endl;
  out(error) << "error level" << endl;
  out(error, 0) << "error 0 level" << endl;
  out(warning) << "warning level" << endl;
  out(warning, 0) << "warning 0 level" << endl;
  out(warning, 1) << "warning 1 level" << endl;
  out(warning, 2) << "warning 2 level" << endl;
  out(info) << "info level" << endl;
  out(info, 0) << "info 0 level" << endl;
  out(info, 1) << "info 1 level" << endl;
  out(info, 2) << "info 2 level" << endl;
  out(verbose) << "verbose level" << endl;
  out(verbose, 0) << "verbose 0 level" << endl;
  out(verbose, 1) << "verbose 1 level" << endl;
  out(verbose, 2) << "verbose 2 level" << endl;
  out(debug) << "debug level" << endl;
  out(debug, 0) << "debug 0 level" << endl;
  out(debug, 1) << "debug 1 level" << endl;
  out(debug, 2) << "debug 2 level" << endl;

  cout << endl << "Verbosity level: debug" << endl;
  setVerbosityLevel(debug);
  out(info) << "default level" << endl;
  out(alert) << "alert level" << endl;
  out(alert, 0) << "alert 0 level" << endl;
  out(error) << "error level" << endl;
  out(error, 0) << "error 0 level" << endl;
  out(warning) << "warning level" << endl;
  out(warning, 0) << "warning 0 level" << endl;
  out(warning, 1) << "warning 1 level" << endl;
  out(warning, 2) << "warning 2 level" << endl;
  out(info) << "info level" << endl;
  out(info, 0) << "info 0 level" << endl;
  out(info, 1) << "info 1 level" << endl;
  out(info, 2) << "info 2 level" << endl;
  out(verbose) << "verbose level" << endl;
  out(verbose, 0) << "verbose 0 level" << endl;
  out(verbose, 1) << "verbose 1 level" << endl;
  out(verbose, 2) << "verbose 2 level" << endl;
  out(debug) << "debug level" << endl;
  out(debug, 0) << "debug 0 level" << endl;
  out(debug, 1) << "debug 1 level" << endl;
  out(debug, 2) << "debug 2 level" << endl;

  cout << endl << "End of tests" << endl;

  return 0;
}
