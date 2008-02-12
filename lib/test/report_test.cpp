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
using namespace report;

int hbackup::verbosity(void) {
  return 0;
}

int hbackup::terminating(const char* function) {
  return 0;
}

int main(void) {
  cout << "Report tests" << endl;

  cout << endl << "Default behaviour" << endl;
  out() << "default level" << endl;
  out(alert) << "alert level" << endl;
  out(error) << "error level" << endl;
  out(warning) << "warning level" << endl;
  out(info) << "info level" << endl;
  out(debug) << "debug level" << endl;

  cout << endl << "Verbosity level: alert" << endl;
  setOutLevel(alert);
  out() << "default level" << endl;
  out(alert) << "alert level" << endl;
  out(error) << "error level" << endl;
  out(warning) << "warning level" << endl;
  out(info) << "info level" << endl;
  out(debug) << "debug level" << endl;

  cout << endl << "Verbosity level: error" << endl;
  setOutLevel(error);
  out() << "default level" << endl;
  out(alert) << "alert level" << endl;
  out(error) << "error level" << endl;
  out(warning) << "warning level" << endl;
  out(info) << "info level" << endl;
  out(debug) << "debug level" << endl;

  cout << endl << "Verbosity level: warning" << endl;
  setOutLevel(warning);
  out() << "default level" << endl;
  out(alert) << "alert level" << endl;
  out(error) << "error level" << endl;
  out(warning) << "warning level" << endl;
  out(info) << "info level" << endl;
  out(debug) << "debug level" << endl;

  cout << endl << "Verbosity level: info" << endl;
  setOutLevel(info);
  out() << "default level" << endl;
  out(alert) << "alert level" << endl;
  out(error) << "error level" << endl;
  out(warning) << "warning level" << endl;
  out(info) << "info level" << endl;
  out(debug) << "debug level" << endl;

  cout << endl << "Verbosity level: debug" << endl;
  setOutLevel(debug);
  out() << "default level" << endl;
  out(alert) << "alert level" << endl;
  out(error) << "error level" << endl;
  out(warning) << "warning level" << endl;
  out(info) << "info level" << endl;
  out(debug) << "debug level" << endl;

  cout << endl << "End of tests" << endl;

  return 0;
}
