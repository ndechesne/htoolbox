/*
     Copyright (C) 2008-2010  Herve Fache

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

#include "stdlib.h" // for system
#include <iostream>

using namespace std;

#include "hreport.h"

using namespace hreport;

time_t time(time_t* tm) {
  time_t val = 1249617904;
  if (tm != NULL) {
    *tm = val;
  }
  return val;
}

int main(void) {
  cout << "Report tests" << endl;

  cout << endl << "Default behaviour" << endl;
  out(info, msg_standard, "default level", -1, NULL);
  out(alert, msg_standard, "alert level", -1, NULL);
  out(alert, msg_standard, "alert 0 level", 0, NULL);
  out(error, msg_standard, "error level", -1, NULL);
  out(error, msg_standard, "error 0 level", 0, NULL);
  out(warning, msg_standard, "warning level", -1, NULL);
  out(warning, msg_standard, "warning 0 level", 0, NULL);
  out(warning, msg_standard, "warning 1 level", 1, NULL);
  out(warning, msg_standard, "warning 2 level", 2, NULL);
  out(info, msg_standard, "info level", -1, NULL);
  out(info, msg_standard, "info 0 level", 0, NULL);
  out(info, msg_standard, "info 1 level", 1, NULL);
  out(info, msg_standard, "info 2 level", 2, NULL);
  out(verbose, msg_standard, "verbose level", -1, NULL);
  out(verbose, msg_standard, "verbose 0 level", 0, NULL);
  out(verbose, msg_standard, "verbose 1 level", 1, NULL);
  out(verbose, msg_standard, "verbose 2 level", 2, NULL);
  out(debug, msg_standard, "debug level", -1, NULL);
  out(debug, msg_standard, "debug 0 level", 0, NULL);
  out(debug, msg_standard, "debug 1 level", 1, NULL);
  out(debug, msg_standard, "debug 2 level", 2, NULL);

  cout << endl << "Verbosity level: alert" << endl;
  report.setLevel(alert);
  out(info, msg_standard, "default level", -1, NULL);
  out(alert, msg_standard, "alert level", -1, NULL);
  out(alert, msg_standard, "alert 0 level", 0, NULL);
  out(error, msg_standard, "error level", -1, NULL);
  out(error, msg_standard, "error 0 level", 0, NULL);
  out(warning, msg_standard, "warning level", -1, NULL);
  out(warning, msg_standard, "warning 0 level", 0, NULL);
  out(warning, msg_standard, "warning 1 level", 1, NULL);
  out(warning, msg_standard, "warning 2 level", 2, NULL);
  out(info, msg_standard, "info level", -1, NULL);
  out(info, msg_standard, "info 0 level", 0, NULL);
  out(info, msg_standard, "info 1 level", 1, NULL);
  out(info, msg_standard, "info 2 level", 2, NULL);
  out(verbose, msg_standard, "verbose level", -1, NULL);
  out(verbose, msg_standard, "verbose 0 level", 0, NULL);
  out(verbose, msg_standard, "verbose 1 level", 1, NULL);
  out(verbose, msg_standard, "verbose 2 level", 2, NULL);
  out(debug, msg_standard, "debug level", -1, NULL);
  out(debug, msg_standard, "debug 0 level", 0, NULL);
  out(debug, msg_standard, "debug 1 level", 1, NULL);
  out(debug, msg_standard, "debug 2 level", 2, NULL);

  cout << endl << "Verbosity level: error" << endl;
  report.setLevel(error);
  out(info, msg_standard, "default level", -1, NULL);
  out(alert, msg_standard, "alert level", -1, NULL);
  out(alert, msg_standard, "alert 0 level", 0, NULL);
  out(error, msg_standard, "error level", -1, NULL);
  out(error, msg_standard, "error 0 level", 0, NULL);
  out(warning, msg_standard, "warning level", -1, NULL);
  out(warning, msg_standard, "warning 0 level", 0, NULL);
  out(warning, msg_standard, "warning 1 level", 1, NULL);
  out(warning, msg_standard, "warning 2 level", 2, NULL);
  out(info, msg_standard, "info level", -1, NULL);
  out(info, msg_standard, "info 0 level", 0, NULL);
  out(info, msg_standard, "info 1 level", 1, NULL);
  out(info, msg_standard, "info 2 level", 2, NULL);
  out(verbose, msg_standard, "verbose level", -1, NULL);
  out(verbose, msg_standard, "verbose 0 level", 0, NULL);
  out(verbose, msg_standard, "verbose 1 level", 1, NULL);
  out(verbose, msg_standard, "verbose 2 level", 2, NULL);
  out(debug, msg_standard, "debug level", -1, NULL);
  out(debug, msg_standard, "debug 0 level", 0, NULL);
  out(debug, msg_standard, "debug 1 level", 1, NULL);
  out(debug, msg_standard, "debug 2 level", 2, NULL);

  cout << endl << "Verbosity level: warning" << endl;
  report.setLevel(warning);
  out(info, msg_standard, "default level", -1, NULL);
  out(alert, msg_standard, "alert level", -1, NULL);
  out(alert, msg_standard, "alert 0 level", 0, NULL);
  out(error, msg_standard, "error level", -1, NULL);
  out(error, msg_standard, "error 0 level", 0, NULL);
  out(warning, msg_standard, "warning level", -1, NULL);
  out(warning, msg_standard, "warning 0 level", 0, NULL);
  out(warning, msg_standard, "warning 1 level", 1, NULL);
  out(warning, msg_standard, "warning 2 level", 2, NULL);
  out(info, msg_standard, "info level", -1, NULL);
  out(info, msg_standard, "info 0 level", 0, NULL);
  out(info, msg_standard, "info 1 level", 1, NULL);
  out(info, msg_standard, "info 2 level", 2, NULL);
  out(verbose, msg_standard, "verbose level", -1, NULL);
  out(verbose, msg_standard, "verbose 0 level", 0, NULL);
  out(verbose, msg_standard, "verbose 1 level", 1, NULL);
  out(verbose, msg_standard, "verbose 2 level", 2, NULL);
  out(debug, msg_standard, "debug level", -1, NULL);
  out(debug, msg_standard, "debug 0 level", 0, NULL);
  out(debug, msg_standard, "debug 1 level", 1, NULL);
  out(debug, msg_standard, "debug 2 level", 2, NULL);

  cout << endl << "Verbosity level: info" << endl;
  report.setLevel(info);
  out(info, msg_standard, "default level", -1, NULL);
  out(alert, msg_standard, "alert level", -1, NULL);
  out(alert, msg_standard, "alert 0 level", 0, NULL);
  out(error, msg_standard, "error level", -1, NULL);
  out(error, msg_standard, "error 0 level", 0, NULL);
  out(warning, msg_standard, "warning level", -1, NULL);
  out(warning, msg_standard, "warning 0 level", 0, NULL);
  out(warning, msg_standard, "warning 1 level", 1, NULL);
  out(warning, msg_standard, "warning 2 level", 2, NULL);
  out(info, msg_standard, "info level", -1, NULL);
  out(info, msg_standard, "info 0 level", 0, NULL);
  out(info, msg_standard, "info 1 level", 1, NULL);
  out(info, msg_standard, "info 2 level", 2, NULL);
  out(verbose, msg_standard, "verbose level", -1, NULL);
  out(verbose, msg_standard, "verbose 0 level", 0, NULL);
  out(verbose, msg_standard, "verbose 1 level", 1, NULL);
  out(verbose, msg_standard, "verbose 2 level", 2, NULL);
  out(debug, msg_standard, "debug level", -1, NULL);
  out(debug, msg_standard, "debug 0 level", 0, NULL);
  out(debug, msg_standard, "debug 1 level", 1, NULL);
  out(debug, msg_standard, "debug 2 level", 2, NULL);

  cout << endl << "Verbosity level: verbose" << endl;
  report.setLevel(verbose);
  out(info, msg_standard, "default level", -1, NULL);
  out(alert, msg_standard, "alert level", -1, NULL);
  out(alert, msg_standard, "alert 0 level", 0, NULL);
  out(error, msg_standard, "error level", -1, NULL);
  out(error, msg_standard, "error 0 level", 0, NULL);
  out(warning, msg_standard, "warning level", -1, NULL);
  out(warning, msg_standard, "warning 0 level", 0, NULL);
  out(warning, msg_standard, "warning 1 level", 1, NULL);
  out(warning, msg_standard, "warning 2 level", 2, NULL);
  out(info, msg_standard, "info level", -1, NULL);
  out(info, msg_standard, "info 0 level", 0, NULL);
  out(info, msg_standard, "info 1 level", 1, NULL);
  out(info, msg_standard, "info 2 level", 2, NULL);
  out(verbose, msg_standard, "verbose level", -1, NULL);
  out(verbose, msg_standard, "verbose 0 level", 0, NULL);
  out(verbose, msg_standard, "verbose 1 level", 1, NULL);
  out(verbose, msg_standard, "verbose 2 level", 2, NULL);
  out(debug, msg_standard, "debug level", -1, NULL);
  out(debug, msg_standard, "debug 0 level", 0, NULL);
  out(debug, msg_standard, "debug 1 level", 1, NULL);
  out(debug, msg_standard, "debug 2 level", 2, NULL);

  cout << endl << "Verbosity level: debug" << endl;
  report.setLevel(debug);
  out(info, msg_standard, "default level", -1, NULL);
  out(alert, msg_standard, "alert level", -1, NULL);
  out(alert, msg_standard, "alert 0 level", 0, NULL);
  out(error, msg_standard, "error level", -1, NULL);
  out(error, msg_standard, "error 0 level", 0, NULL);
  out(warning, msg_standard, "warning level", -1, NULL);
  out(warning, msg_standard, "warning 0 level", 0, NULL);
  out(warning, msg_standard, "warning 1 level", 1, NULL);
  out(warning, msg_standard, "warning 2 level", 2, NULL);
  out(info, msg_standard, "info level", -1, NULL);
  out(info, msg_standard, "info 0 level", 0, NULL);
  out(info, msg_standard, "info 1 level", 1, NULL);
  out(info, msg_standard, "info 2 level", 2, NULL);
  out(verbose, msg_standard, "verbose level", -1, NULL);
  out(verbose, msg_standard, "verbose 0 level", 0, NULL);
  out(verbose, msg_standard, "verbose 1 level", 1, NULL);
  out(verbose, msg_standard, "verbose 2 level", 2, NULL);
  out(debug, msg_standard, "debug level", -1, NULL);
  out(debug, msg_standard, "debug 0 level", 0, NULL);
  out(debug, msg_standard, "debug 1 level", 1, NULL);
  out(debug, msg_standard, "debug 2 level", 2, NULL);

  cout << endl << "Line autoblanking" << endl;
  out(info, msg_standard, "", -3, NULL);
  out(info, msg_standard, "Short line", -3, NULL);
  out(info, msg_standard, "A longer line", -3, NULL);
  out(info, msg_standard, "Tiny one", -3, NULL);
  out(info, msg_standard, ".", -3, NULL);
  out(info, msg_standard, "A longer line", -3, NULL);
  out(info, msg_standard, "An even longer line", -3, NULL);
  out(info, msg_standard, "A normal line", -1, NULL);


  cout << endl << "Macros" << endl;
  hlog_alert("message: %s", "alert");
  hlog_error("message: %s", "error");
  hlog_warning("message: %s", "warning");
  hlog_info("message: %s", "info");
  hlog_verbose("message: %s", "verbose");
  hlog_debug("message: %s", "debug");
  hlog_info_temp("temporary message: %s", "info");
  hlog_verbose_temp("temporary message: %s", "verbose");
  hlog_debug_temp("temporary message: %s", "debug");
  hlog_info_temp("%s", "");


  cout << endl << "Log to file" << endl;
  report.startFileLog("report.log", 1, 100);
  report.stopConsoleLog();
  report.setLevel(debug);
  hlog_alert("message: %s", "alert");
  hlog_error("message: %s", "error");
  hlog_warning("message: %s", "warning");
  hlog_info("message: %s", "info");
  hlog_verbose("message: %s", "verbose");
  hlog_debug("message: %s", "debug");
  hlog_info_temp("temporary message: %s", "info");
  hlog_verbose_temp("temporary message: %s", "verbose");
  hlog_debug_temp("temporary message: %s", "debug");
  hlog_info_temp("%s", "");

  cout << endl << "Error logging to file" << endl;
  report.startFileLog("missing/report.log");
  report.startConsoleLog();
  report.startFileLog("missing/report.log");
  report.stopConsoleLog();
  report.stopFileLog();
  report.stopFileLog();


  cout << endl << "Rotate file" << endl;
  int _unused;
  _unused = system("for name in `ls report.log*`; do "
                   "echo \"$name:\"; cat $name; done");
  // should rotate, log then empty
  report.startFileLog("report.log", 3, 5);
  _unused = system("for name in `ls report.log*`; do "
                   "echo \"$name:\"; cat $name; done");
  // should not rotate
  report.startFileLog("report.log", 3, 5);
  _unused = system("for name in `ls report.log*`; do "
                   "echo \"$name:\"; cat $name; done");
  _unused = system("echo \"1\" > report.log");
  // should rotate
  report.startFileLog("report.log", 3, 5);

  _unused = system("for name in `ls report.log*`; do "
                   "echo \"$name:\"; cat $name; done");
  _unused = system("echo \"1\" > report.log");
  // should rotate
  report.startFileLog("report.log", 3, 5);

  _unused = system("for name in `ls report.log*`; do "
                   "echo \"$name:\"; cat $name; done");
  _unused = system("echo \"1\" > report.log");
  // should rotate
  report.startFileLog("report.log", 3, 5);

  _unused = system("for name in `ls report.log*`; do "
                   "echo \"$name:\"; cat $name; done");
  // should not rotate
  report.startFileLog("report.log", 3, 5);

  _unused = system("for name in `ls report.log*`; do "
                   "echo \"$name:\"; cat $name; done");


  cout << endl << "End of tests" << endl;

  return 0;
}
