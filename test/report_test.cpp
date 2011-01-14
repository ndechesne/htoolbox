/*
     Copyright (C) 2008-2011  Herve Fache

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
#include "stdio.h"
#include "string.h"
#include "errno.h"
#include <iostream>

using namespace std;

#include "report.h"

using namespace htoolbox;

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

  cout << endl << "Notifications" << endl;
  report.setLevel(alert);
  hlog_alert("console level should be alert, is %s",
    Report::levelString(report.consoleLogLevel()));
  report.setLevel(verbose);
  hlog_alert("console level should be verbose, is %s",
    Report::levelString(report.consoleLogLevel()));
  report.setConsoleLogLevel(warning);
  hlog_alert("level should be warning, is %s",
    Report::levelString(report.level()));
  report.setConsoleLogLevel(debug);
  hlog_alert("level should be debug, is %s",
    Report::levelString(report.level()));

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
  report.addConsoleCondition(false, "", regression);
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
  report.addConsoleCondition(false, "report_test.cpp", regression, regression, 0,
    __LINE__ + 10);
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
  report.addConsoleCondition(false, "report_test.cpp", regression);
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


  cout << endl << "Filters" << endl;
  report.stopConsoleLog();
  hlog_info("should not appear");
  Report::ConsoleOutput con_log;
  con_log.open();
  Report::Filter fil1(&con_log, false);
  Report::Filter fil2(&fil1, false);
  report.add(&fil2);
  report.log("file1", 20 , info, false, 0, "file1:20: INFO: filters not enabled");
  report.log("file2", 20 , info, false, 0, "file2:20: INFO: filters not enabled");
  // Remove file2 altogether
  fil1.addCondition(false, "file1");
  report.log("file1", 20 , info, false, 0, "file1:20: INFO: filter 1 enabled");
  report.log("file2", 20 , info, false, 0, "file2:20: INFO: filter 1 enabled");
  // Remove file1 lines > 15
  fil2.addCondition(false, "file1", alert, regression, 15);
  report.log("file1", 10 , info, false, 0, "file1:10: INFO: filters 1&2 enabled");
  report.log("file1", 20 , info, false, 0, "file1:20: INFO: filters 1&2 enabled");
  report.remove(&fil2);
  hlog_info("should not appear");
  report.startConsoleLog();


  cout << endl << "Log to file" << endl;
  Report::FileOutput log_file("report.log", 1, 5);
  if (log_file.open() < 0) {
    hlog_error("%s opening log file", strerror(errno));
    return 0;
  }
  report.add(&log_file);
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
  log_file.close();

  cout << endl << "Error logging to file" << endl;
  Report::FileOutput missing_log_file("missing/report.log");
  report.startConsoleLog();
  missing_log_file.open();
  report.stopConsoleLog();


  cout << endl << "Rotate file" << endl;
  int _unused;
  show_logs();
  // should rotate, log then empty
  log_file.open();
  show_logs();
  // should not rotate
  log_file.open();
  log_file.close();
  show_logs();
  _unused = system("echo \"1\" > report.log");
  // should rotate
  log_file.open();
  log_file.close();
  show_logs();
  _unused = system("echo \"1\" > report.log");
  // should rotate
  log_file.open();
  log_file.close();
  show_logs();
  _unused = system("echo \"1\" > report.log");
  // should rotate
  log_file.open();
  log_file.close();
  show_logs();
  // should not rotate
  log_file.open();
  log_file.close();
  show_logs();
  _unused = system("ls report.log*");


  cout << endl << "Specific report" << endl;
  Report my_report;
  hlog_report(my_report, alert, "some message with a number %d", 9);
  hlog_report(my_report, info, "message with a number %d", 10);
  hlog_report(my_report, debug, "with a number %d", 11);


  cout << endl << "End of tests" << endl;
  return 0;
}
