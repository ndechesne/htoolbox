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

#include "report.h"

using namespace htools;

time_t time(time_t* tm) {
  time_t val = 1249617904;
  if (tm != NULL) {
    *tm = val;
  }
  return val;
}

void show_logs() {
  int _unused;
  _unused  = system(
    "for name in `ls report.log*`; do"
    "  echo \"$name:\";"
    "  if echo $name | grep gz > /dev/null; then"
    "    zcat $name;"
    "  else"
    "    cat $name;"
    "  fi;"
    "done");
}

int main(void) {
  cout << "Report tests" << endl;

  cout << endl << "Default behaviour" << endl;
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  cout << endl << "Verbosity level: alert" << endl;
  report.setLevel(alert);
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  cout << endl << "Verbosity level: error" << endl;
  report.setLevel(error);
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  cout << endl << "Verbosity level: warning" << endl;
  report.setLevel(warning);
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  cout << endl << "Verbosity level: info" << endl;
  report.setLevel(info);
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  cout << endl << "Verbosity level: verbose" << endl;
  report.setLevel(verbose);
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  cout << endl << "Verbosity level: debug" << endl;
  report.setLevel(debug);
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  cout << endl << "Verbosity level: regression" << endl;
  report.setLevel(regression);
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  cout << endl << "Verbosity level: regression, restricted" << endl;
  report.setLevel(regression);
  report.addRegressionCondition("");
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  cout << endl << "Verbosity level: regression, file name, line restricted" << endl;
  report.setLevel(regression);
  report.addRegressionCondition("report_test.cpp", 0, __LINE__ + 10);
  hlog_regression("regression level");
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  cout << endl << "Verbosity level: regression, file name" << endl;
  report.setLevel(regression);
  report.addRegressionCondition("report_test.cpp");
  hlog_regression("regression level");
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  cout << endl << "Line autoblanking" << endl;
  hlog_info_temp("%s", "");
  hlog_info_temp("Short line");
  hlog_info_temp("A longer line");
  hlog_info_temp("A huge line: If was an American science fiction magazine launched in March 1952 by Quinn Publications, owned by James L. Quinn. After a series of editors, including Paul W. Fairman, Larry T. Shaw, and Damon Knight, Quinn sold the magazine to Robert Guinn at Galaxy Publishing and in 1961 Frederik Pohl became editor. Under Pohl, If won the Hugo Award  for best professional magazine three years running from 1966 to 1968. In 1969 Guinn sold all his magazines to Universal Publishing and Distribution (UPD). The magazine was not as successful with Ejler Jakobsson as editor and circulation plummeted. In early 1974 Jim Baen took over from Jakobsson as editor, but increasing paper costs meant that UPD could no longer afford to publish both Galaxy and If. Galaxy was regarded as the senior of the two magazines, so If was merged into Galaxy after the December 1974 issue, its 175th issue overall. Over its 22 years, If published many award-winning stories, including Robert A. Heinlein's novel The Moon Is a Harsh Mistress, and Harlan Ellison's short story \"I Have No Mouth and I Must Scream\". Several well-known writers sold their first story to If; the most successful was Larry Niven, whose story \"The Coldest Place\" appeared in the December 1964 issue.");
  hlog_info_temp("Tiny one");
  hlog_info_temp(".");
  hlog_info_temp("A longer line");
  hlog_info_temp("An even longer line");
  hlog_info("A normal line");


  cout << endl << "Macros" << endl;
  hlog_alert("message: %s", "alert");
  hlog_error("message: %s", "error");
  hlog_warning("message: %s", "warning");
  hlog_info("message: %s", "info");
  hlog_verbose("message: %s", "verbose");
  hlog_debug("message: %s", "debug");
  hlog_regression("message: %s", "regression");
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
  hlog_regression("message: %s", "regression");
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
  show_logs();
  // should rotate, log then empty
  report.startFileLog("report.log", 3, 5);
  show_logs();
  // should not rotate
  report.startFileLog("report.log", 3, 5);
  show_logs();
  _unused = system("echo \"1\" > report.log");
  // should rotate
  report.startFileLog("report.log", 3, 5);
  show_logs();
  _unused = system("echo \"1\" > report.log");
  // should rotate
  report.startFileLog("report.log", 3, 5);
  show_logs();
  _unused = system("echo \"1\" > report.log");
  // should rotate
  report.startFileLog("report.log", 3, 5);
  show_logs();
  // should not rotate
  report.startFileLog("report.log", 3, 5);
  show_logs();
  _unused = system("ls report.log*");


  cout << endl << "End of tests" << endl;
  return 0;
}
