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
  
  cout << endl << "One instance" << endl;
  Report* report1 = Report::self();

  cout << endl << "Default behaviour" << endl;
  report1->report("default level");
  report1->report("alert level", Report::alert);
  report1->report("error level", Report::error);
  report1->report("warning level", Report::warning);
  report1->report("info level", Report::info);
  report1->report("debug level", Report::debug);
  report1->report("none level", Report::none);

  cout << endl << "Verbosity level: alert" << endl;
  *Report::self() = Report::alert;
  report1->report("default level");
  report1->report("alert level", Report::alert);
  report1->report("error level", Report::error);
  report1->report("warning level", Report::warning);
  report1->report("info level", Report::info);
  report1->report("debug level", Report::debug);
  report1->report("none level", Report::none);

  cout << endl << "Verbosity level: error" << endl;
  *Report::self() = Report::error;
  report1->report("default level");
  report1->report("alert level", Report::alert);
  report1->report("error level", Report::error);
  report1->report("warning level", Report::warning);
  report1->report("info level", Report::info);
  report1->report("debug level", Report::debug);
  report1->report("none level", Report::none);

  cout << endl << "Verbosity level: warning" << endl;
  *Report::self() = Report::warning;
  report1->report("default level");
  report1->report("alert level", Report::alert);
  report1->report("error level", Report::error);
  report1->report("warning level", Report::warning);
  report1->report("info level", Report::info);
  report1->report("debug level", Report::debug);
  report1->report("none level", Report::none);

  cout << endl << "Verbosity level: info" << endl;
  *Report::self() = Report::info;
  report1->report("default level");
  report1->report("alert level", Report::alert);
  report1->report("error level", Report::error);
  report1->report("warning level", Report::warning);
  report1->report("info level", Report::info);
  report1->report("debug level", Report::debug);
  report1->report("none level", Report::none);

  cout << endl << "Verbosity level: debug" << endl;
  *Report::self() = Report::debug;
  report1->report("default level");
  report1->report("alert level", Report::alert);
  report1->report("error level", Report::error);
  report1->report("warning level", Report::warning);
  report1->report("info level", Report::info);
  report1->report("debug level", Report::debug);
  report1->report("none level", Report::none);

  
  cout << endl << "Two instances" << endl;
  Report* report2 = Report::self();

  cout << endl << "Verbosity level: unchanged" << endl;
  report2->report("default level");
  report2->report("alert level", Report::alert);
  report2->report("error level", Report::error);
  report2->report("warning level", Report::warning);
  report2->report("info level", Report::info);
  report2->report("debug level", Report::debug);
  report2->report("none level", Report::none);

  cout << endl << "Verbosity level (first instance): error" << endl;
  *Report::self() = Report::error;
  report2->report("default level");
  report2->report("alert level", Report::alert);
  report2->report("error level", Report::error);
  report2->report("warning level", Report::warning);
  report2->report("info level", Report::info);
  report2->report("debug level", Report::debug);
  report2->report("none level", Report::none);


  cout << endl << "Two instances and direct access" << endl;

  cout << endl << "Verbosity level (second instance): debug" << endl;
  *Report::self() = Report::debug;
  Report::self()->report("default level");
  Report::self()->report("alert level", Report::alert);
  Report::self()->report("error level", Report::error);
  Report::self()->report("warning level", Report::warning);
  Report::self()->report("info level", Report::info);
  Report::self()->report("debug level", Report::debug);
  Report::self()->report("none level", Report::none);

  cout << endl << "Verbosity level (first instance): error" << endl;
  *Report::self() = Report::error;
  Report::self()->report("default level");
  Report::self()->report("alert level", Report::alert);
  Report::self()->report("error level", Report::error);
  Report::self()->report("warning level", Report::warning);
  Report::self()->report("info level", Report::info);
  Report::self()->report("debug level", Report::debug);
  Report::self()->report("none level", Report::none);


  cout << endl << "Two instances and direct access via out()" << endl;

  cout << endl << "Verbosity level (second instance): debug" << endl;
  *Report::self() = Report::debug;
  Report::out("default level");
  Report::out("alert level", Report::alert);
  Report::out("error level", Report::error);
  Report::out("warning level", Report::warning);
  Report::out("info level", Report::info);
  Report::out("debug level", Report::debug);
  Report::out("none level", Report::none);

  cout << endl << "Verbosity level (no instance): error" << endl;
  *Report::self() = Report::error;
  Report::out("default level");
  Report::out("alert level", Report::alert);
  Report::out("error level", Report::error);
  Report::out("warning level", Report::warning);
  Report::out("info level", Report::info);
  Report::out("debug level", Report::debug);
  Report::out("none level", Report::none);


  cout << endl << "End of tests" << endl;

  return 0;
}
