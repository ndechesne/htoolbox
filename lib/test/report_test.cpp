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

#include "report.h"
#include "hbackup.h"

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
  Report::out() << "default level" << endl;
  Report::out(Report::alert) << "alert level" << endl;
  Report::out(Report::error) << "error level" << endl;
  Report::out(Report::warning) << "warning level" << endl;
  Report::out(Report::info) << "info level" << endl;
  Report::out(Report::debug) << "debug level" << endl;

  cout << endl << "Verbosity level: alert" << endl;
  *Report::self() = Report::alert;
  Report::out() << "default level" << endl;
  Report::out(Report::alert) << "alert level" << endl;
  Report::out(Report::error) << "error level" << endl;
  Report::out(Report::warning) << "warning level" << endl;
  Report::out(Report::info) << "info level" << endl;
  Report::out(Report::debug) << "debug level" << endl;

  cout << endl << "Verbosity level: error" << endl;
  *Report::self() = Report::error;
  Report::out() << "default level" << endl;
  Report::out(Report::alert) << "alert level" << endl;
  Report::out(Report::error) << "error level" << endl;
  Report::out(Report::warning) << "warning level" << endl;
  Report::out(Report::info) << "info level" << endl;
  Report::out(Report::debug) << "debug level" << endl;

  cout << endl << "Verbosity level: warning" << endl;
  *Report::self() = Report::warning;
  Report::out() << "default level" << endl;
  Report::out(Report::alert) << "alert level" << endl;
  Report::out(Report::error) << "error level" << endl;
  Report::out(Report::warning) << "warning level" << endl;
  Report::out(Report::info) << "info level" << endl;
  Report::out(Report::debug) << "debug level" << endl;

  cout << endl << "Verbosity level: info" << endl;
  *Report::self() = Report::info;
  Report::out() << "default level" << endl;
  Report::out(Report::alert) << "alert level" << endl;
  Report::out(Report::error) << "error level" << endl;
  Report::out(Report::warning) << "warning level" << endl;
  Report::out(Report::info) << "info level" << endl;
  Report::out(Report::debug) << "debug level" << endl;

  cout << endl << "Verbosity level: debug" << endl;
  *Report::self() = Report::debug;
  Report::out() << "default level" << endl;
  Report::out(Report::alert) << "alert level" << endl;
  Report::out(Report::error) << "error level" << endl;
  Report::out(Report::warning) << "warning level" << endl;
  Report::out(Report::info) << "info level" << endl;
  Report::out(Report::debug) << "debug level" << endl;

  cout << endl << "End of tests" << endl;

  return 0;
}
