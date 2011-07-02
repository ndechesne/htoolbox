/*
    Copyright (C) 2008 - 2011  Hervé Fache

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
  hlog_report(info, "report, info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  for (Level level = alert; level <= regression;
      level = static_cast<Level>(level + 1)) {
    hlog_generic(level, 0, -1, "%s level", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY, -1,
      "%s level, temporary", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX, -1,
      "%s level, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOLINEFEED, -1,
      "%s level, no line feed", Criticality(level).toString());
    hlog_generic(level, 0, 0, "...LF");
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX, -1,
      "%s level, temporary, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level,
      Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1, ".");
  }
  hlog_generic(alert, Report::HLOG_NOPREFIX, -1, "reset");

  cout << endl << "String conversions" << endl;
  Criticality converted;
  converted.set(regression);
  converted.setFromString("alert");
  hlog_info("%s", converted.toString());
  converted.set(regression);
  converted.setFromString("error");
  hlog_info("%s", converted.toString());
  converted.set(regression);
  converted.setFromString("warning");
  hlog_info("%s", converted.toString());
  converted.set(regression);
  converted.setFromString("info");
  hlog_info("%s", converted.toString());
  converted.set(regression);
  converted.setFromString("verbose");
  hlog_info("%s", converted.toString());
  converted.set(regression);
  converted.setFromString("debug");
  hlog_info("%s", converted.toString());
  converted = alert;
  converted.setFromString("regression");
  hlog_info("%s", converted.toString());

  cout << endl << "Notifications" << endl;
  report.setLevel(alert);
  hlog_alert("console level should be alert, is %s",
    report.consoleLogLevel().toString());
  report.setLevel(verbose);
  hlog_alert("console level should be verbose, is %s",
    report.consoleLogLevel().toString());
  report.setConsoleLogLevel(warning);
  hlog_alert("level should be warning, is %s",
    report.level().toString());
  report.setConsoleLogLevel(debug);
  hlog_alert("level should be debug, is %s",
    report.level().toString());

  cout << endl << "Verbosity level: alert" << endl;
  report.setLevel(alert);
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_report(info, "report, info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  for (Level level = alert; level <= regression;
      level = static_cast<Level>(level + 1)) {
    hlog_generic(level, 0, -1, "%s level", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY, -1,
      "%s level, temporary", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX, -1,
      "%s level, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOLINEFEED, -1,
      "%s level, no line feed", Criticality(level).toString());
    hlog_generic(level, 0, 0, "...LF");
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX, -1,
      "%s level, temporary, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level,
      Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1, ".");
  }
  // This one has an additional LF
  hlog_generic(alert, Report::HLOG_NOPREFIX, -1, "\nreset");

  cout << endl << "Verbosity level: error" << endl;
  report.setLevel(error);
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_report(info, "report, info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  for (Level level = alert; level <= regression;
      level = static_cast<Level>(level + 1)) {
    hlog_generic(level, 0, -1, "%s level", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY, -1,
      "%s level, temporary", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX, -1,
      "%s level, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOLINEFEED, -1,
      "%s level, no line feed", Criticality(level).toString());
    hlog_generic(level, 0, 0, "...LF");
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX, -1,
      "%s level, temporary, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level,
      Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1, ".");
  }
  hlog_generic(alert, Report::HLOG_NOPREFIX, -1, "reset");

  cout << endl << "Verbosity level: warning" << endl;
  report.setLevel(warning);
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_report(info, "report, info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  for (Level level = alert; level <= regression;
      level = static_cast<Level>(level + 1)) {
    hlog_generic(level, 0, -1, "%s level", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY, -1,
      "%s level, temporary", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX, -1,
      "%s level, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOLINEFEED, -1,
      "%s level, no line feed", Criticality(level).toString());
    hlog_generic(level, 0, 0, "...LF");
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX, -1,
      "%s level, temporary, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level,
      Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1, ".");
  }
  hlog_generic(alert, Report::HLOG_NOPREFIX, -1, "reset");

  cout << endl << "Verbosity level: info" << endl;
  report.setLevel(info);
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_report(info, "report, info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  for (Level level = alert; level <= regression;
      level = static_cast<Level>(level + 1)) {
    hlog_generic(level, 0, -1, "%s level", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY, -1,
      "%s level, temporary", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX, -1,
      "%s level, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOLINEFEED, -1,
      "%s level, no line feed", Criticality(level).toString());
    hlog_generic(level, 0, 0, "...LF");
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX, -1,
      "%s level, temporary, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level,
      Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1, ".");
  }
  hlog_generic(alert, Report::HLOG_NOPREFIX, -1, "reset");

  cout << endl << "Verbosity level: verbose" << endl;
  report.setLevel(verbose);
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_report(info, "report, info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  for (Level level = alert; level <= regression;
      level = static_cast<Level>(level + 1)) {
    hlog_generic(level, 0, -1, "%s level", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY, -1,
      "%s level, temporary", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX, -1,
      "%s level, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOLINEFEED, -1,
      "%s level, no line feed", Criticality(level).toString());
    hlog_generic(level, 0, 0, "...LF");
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX, -1,
      "%s level, temporary, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level,
      Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1, ".");
  }
  hlog_generic(alert, Report::HLOG_NOPREFIX, -1, "reset");

  cout << endl << "Verbosity level: debug" << endl;
  report.setLevel(debug);
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_report(info, "report, info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  for (Level level = alert; level <= regression;
      level = static_cast<Level>(level + 1)) {
    hlog_generic(level, 0, -1, "%s level", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY, -1,
      "%s level, temporary", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX, -1,
      "%s level, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOLINEFEED, -1,
      "%s level, no line feed", Criticality(level).toString());
    hlog_generic(level, 0, 0, "...LF");
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX, -1,
      "%s level, temporary, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level,
      Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1, ".");
  }
  hlog_generic(alert, Report::HLOG_NOPREFIX, -1, "reset");

  cout << endl << "Verbosity level: regression" << endl;
  report.setLevel(regression);
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_report(info, "report, info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  for (Level level = alert; level <= regression;
      level = static_cast<Level>(level + 1)) {
    hlog_generic(level, 0, -1, "%s level", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY, -1,
      "%s level, temporary", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX, -1,
      "%s level, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOLINEFEED, -1,
      "%s level, no line feed", Criticality(level).toString());
    hlog_generic(level, 0, 0, "...LF");
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX, -1,
      "%s level, temporary, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level,
      Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1, ".");
  }
  hlog_generic(alert, Report::HLOG_NOPREFIX, -1, "reset");

  cout << endl << "Verbosity level: regression, restricted" << endl;
  report.setLevel(regression);
  report.consoleFilter().addCondition(Report::Filter::reject, __FILE__, 0, 0, regression);
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_report(info, "report, info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  for (Level level = alert; level <= regression;
      level = static_cast<Level>(level + 1)) {
    hlog_generic(level, 0, -1, "%s level", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY, -1,
      "%s level, temporary", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX, -1,
      "%s level, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOLINEFEED, -1,
      "%s level, no line feed", Criticality(level).toString());
    hlog_generic(level, 0, 0, "...LF");
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX, -1,
      "%s level, temporary, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level,
      Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1, ".");
  }
  hlog_generic(alert, Report::HLOG_NOPREFIX, -1, "reset");

  cout << endl << "Verbosity level: regression, file name, line restricted" << endl;
  report.setLevel(regression);
  report.consoleFilter().addCondition(Report::Filter::force, "report_test.cpp", 0, __LINE__ + 10,
    regression, regression);
  hlog_regression("regression level");
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_report(info, "report, info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  for (Level level = alert; level <= regression;
      level = static_cast<Level>(level + 1)) {
    hlog_generic(level, 0, -1, "%s level", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY, -1,
      "%s level, temporary", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX, -1,
      "%s level, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOLINEFEED, -1,
      "%s level, no line feed", Criticality(level).toString());
    hlog_generic(level, 0, 0, "...LF");
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX, -1,
      "%s level, temporary, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level,
      Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1, ".");
  }
  hlog_generic(alert, Report::HLOG_NOPREFIX, -1, "reset");

  cout << endl << "Verbosity level: regression, file name" << endl;
  report.setLevel(regression);
  report.consoleFilter().addCondition(Report::Filter::force, "report_test.cpp", regression);
  hlog_regression("regression level");
  hlog_info("default level");
  hlog_alert("alert level");
  hlog_error("error level");
  hlog_warning("warning level");
  hlog_info("info level");
  hlog_report(info, "report, info level");
  hlog_verbose("verbose level");
  hlog_verbose_arrow(0, "verbose 0 level");
  hlog_verbose_arrow(1, "verbose 1 level");
  hlog_verbose_arrow(2, "verbose 2 level");
  hlog_debug("debug level");
  hlog_debug_arrow(0, "debug 0 level");
  hlog_debug_arrow(1, "debug 1 level");
  hlog_debug_arrow(2, "debug 2 level");
  hlog_regression("regression level");

  for (Level level = alert; level <= regression;
      level = static_cast<Level>(level + 1)) {
    hlog_generic(level, 0, -1, "%s level", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY, -1,
      "%s level, temporary", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX, -1,
      "%s level, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOLINEFEED, -1,
      "%s level, no line feed", Criticality(level).toString());
    hlog_generic(level, 0, 0, "...LF");
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX, -1,
      "%s level, temporary, no prefix", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level,
      Report::HLOG_TEMPORARY | Report::HLOG_NOPREFIX | Report::HLOG_NOLINEFEED, -1,
      "%s level, temporary, no prefix, no LF", Criticality(level).toString());
    hlog_generic(level, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1, ".");
  }
  hlog_generic(alert, Report::HLOG_NOPREFIX, -1, "reset");

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
  hlog_generic(info, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1,
    "A temp line with no LF");
  hlog_generic(info, Report::HLOG_TEMPORARY, 0,
    " and its end");
  hlog_generic(info, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1,
    "Still temp: start...");
  hlog_generic(info, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1,
    "middle...");
  hlog_generic(info, Report::HLOG_TEMPORARY, 0,
    "end");
  hlog_generic(info, Report::HLOG_TEMPORARY | Report::HLOG_NOLINEFEED, -1,
    "Again temp: start...");
  hlog_generic(error, Report::HLOG_TEMPORARY, 0, "error");
  hlog_generic(alert, 0, 0, "alert");


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
  Report::ConsoleOutput con_log("local console");
  con_log.open();
  con_log.setLevel(info);
  // filter 1 -> con_log
  Report::Filter fil("filter 1", &con_log, false);
  // report -> filter 1 -> con_log
  report.add(&fil);
  report.show(info, 0, false);
  // Won't appear as level is info
  report.log("file0", 20, "func0", verbose, false, -1, "file0:20:func0 VERBOSE: blah");
  // Will appear
  report.log("file1", 2, "func1", info, false, -1, "file1:2:func1 INFO: blah");
  report.log("file1", 10, "func2", info, false, -1, "file1:10:func2 INFO: blah");
  report.log("file1", 20, "func3", info, false, -1, "file1:20:func3 INFO: blah");
  // Won't appear as level is info
  report.log("file1", 20, "func4", debug, false, -1, "file1:20:func4 DEBUG: blah");
  report.log("file2", 10, "func5", debug, false, -1, "file2:10:func5 DEBUG: blah");
  // Will appear
  report.log("file2", 20, "func6", info, false, -1, "file2:20:func6 INFO: blah");

  // Reject file2 altogether
  fil.addCondition(Report::Filter::reject, "file2");
  report.show(info, 0, false);
  // Won't appear as level is info
  report.log("file0", 20, "func0", verbose, false, -1, "file0:20:func0 VERBOSE: blah");
  // Will appear
  report.log("file1", 2, "func1", info, false, -1, "file1:2:func1 INFO: blah");
  report.log("file1", 10, "func2", info, false, -1, "file1:10:func2 INFO: blah");
  report.log("file1", 20, "func3", info, false, -1, "file1:20:func3 INFO: blah");
  // Won't appear as level is info
  report.log("file1", 20, "func4", debug, false, -1, "file1:20:func4 DEBUG: blah");
  // Filtered out
  report.log("file2", 10, "func5", debug, false, -1, "file2:10:func5 DEBUG: blah");
  report.log("file2", 20, "func6", info, false, -1, "file2:20:func6 INFO: blah");

  // Reject file1 line > 15
  fil.addCondition(Report::Filter::reject, "file1", 0, 15);
  report.show(info, 0, false);
  // Won't appear as level is info
  report.log("file0", 20, "func0", verbose, false, -1, "file0:20:func0 VERBOSE: blah");
  // Filtered out
  report.log("file1", 2, "func1", info, false, -1, "file1:2:func1 INFO: blah");
  report.log("file1", 10, "func2", info, false, -1, "file1:10:func2 INFO: blah");
  // Will appear
  report.log("file1", 20, "func3", info, false, -1, "file1:20:func3 INFO: blah");
  // Won't appear as level is info
  report.log("file1", 20, "func4", debug, false, -1, "file1:20:func4 DEBUG: blah");
  // Filtered out
  report.log("file2", 10, "func5", debug, false, -1, "file2:10:func5 DEBUG: blah");
  report.log("file2", 20, "func6", info, false, -1, "file2:20:func6 INFO: blah");

  // Accept file1 5 < line < 25 level >= debug
  size_t index_1 = fil.addCondition(Report::Filter::force, "file1", 5, 25, debug);
  report.show(info, 0, false);
  // Won't appear as level is info
  report.log("file0", 20, "func0", verbose, false, -1, "file0:20:func0 VERBOSE: blah");
  // Filtered out
  report.log("file1", 2, "func1", info, false, -1, "file1:2:func1 INFO: blah");
  // Will appear
  report.log("file1", 10, "func2", info, false, -1, "file1:10:func2 INFO: blah");
  report.log("file1", 20, "func3", info, false, -1, "file1:20:func3 INFO: blah");
  report.log("file1", 20, "func4", debug, false, -1, "file1:20:func4 DEBUG: blah");
  // Filtered out
  report.log("file2", 10, "func5", debug, false, -1, "file2:10:func5 DEBUG: blah");
  report.log("file2", 20, "func6", info, false, -1, "file2:20:func6 INFO: blah");

  // Accept all if level <= verbose
  size_t index_2 = fil.addCondition(Report::Filter::force,
    Report::Filter::ALL_FILES, 0, 0, alert, verbose);
  report.show(info, 0, false);
  // Will appear
  report.log("file0", 20, "func0", verbose, false, -1, "file0:20:func0 VERBOSE: blah");
  // Filtered out
  report.log("file1", 2, "func1", info, false, -1, "file1:2:func1 INFO: blah");
  report.log("file1", 10, "func2", info, false, -1, "file1:10:func2 INFO: blah");
  // Will appear
  report.log("file1", 20, "func3", info, false, -1, "file1:20:func3 INFO: blah");
  report.log("file1", 20, "func4", debug, false, -1, "file1:20:func4 DEBUG: blah");
  // Filtered out
  report.log("file2", 10, "func5", debug, false, -1, "file2:10:func5 DEBUG: blah");
  // Will appear
  report.log("file2", 20, "func6", info, false, -1, "file2:20:func6 INFO: blah");

  // Remove rule on debug
  fil.removeCondition(index_1);
  report.show(info, 0, false);
  // Will appear
  report.log("file0", 20, "func0", verbose, false, -1, "file0:20:func0 VERBOSE: blah");
  report.log("file1", 2, "func1", info, false, -1, "file1:2:func1 INFO: blah");
  report.log("file1", 10, "func2", info, false, -1, "file1:10:func2 INFO: blah");
  report.log("file1", 20, "func3", info, false, -1, "file1:20:func3 INFO: blah");
  // Filtered out
  report.log("file1", 20, "func4", debug, false, -1, "file1:20:func4 DEBUG: blah");
  report.log("file2", 10, "func5", debug, false, -1, "file2:10:func5 DEBUG: blah");
  // Will appear
  report.log("file2", 20, "func6", info, false, -1, "file2:20:func6 INFO: blah");

  // Remove rule for all files
  fil.removeCondition(index_2);
  // Add rule to accept file2
  fil.addCondition(Report::Filter::accept, "file2");
  report.show(info, 0, false);
  // Won't appear as level is info
  report.log("file0", 20, "func0", verbose, false, -1, "file0:20:func0 VERBOSE: blah");
  // Filtered out
  report.log("file1", 2, "func1", info, false, -1, "file1:2:func1 INFO: blah");
  report.log("file1", 10, "func2", info, false, -1, "file1:10:func2 INFO: blah");
  // Will appear
  report.log("file1", 20, "func3", info, false, -1, "file1:20:func3 INFO: blah");
  // Won't appear as level is info
  report.log("file1", 20, "func4", debug, false, -1, "file1:20:func4 DEBUG: blah");
  // Filtered out
  report.log("file2", 10, "func5", debug, false, -1, "file2:10:func5 DEBUG: blah");
  // Will appear
  report.log("file2", 20, "func6", info, false, -1, "file2:20:func6 INFO: blah");

  // Accept func5 from file2 level >= debug
  fil.addCondition(Report::Filter::force, "file2", "func5", debug, debug);
  report.show(info, 0, false);
  // Won't appear as level is info
  report.log("file0", 20, "func0", verbose, false, -1, "file0:20:func0 VERBOSE: blah");
  // Filtered out
  report.log("file1", 2, "func1", info, false, -1, "file1:2:func1 INFO: blah");
  report.log("file1", 10, "func2", info, false, -1, "file1:10:func2 INFO: blah");
  // Will appear
  report.log("file1", 20, "func3", info, false, -1, "file1:20:func3 INFO: blah");
  // Won't appear as level is info
  report.log("file1", 20, "func4", debug, false, -1, "file1:20:func4 DEBUG: blah");
  // Filtered out
  report.log("file2", 10, "func5", debug, false, -1, "file2:10:func5 DEBUG: blah");
  // Will appear
  report.log("file2", 20, "func6", info, false, -1, "file2:20:func6 INFO: blah");

  report.remove(&fil);
  hlog_info("should not appear");
  report.startConsoleLog();









































































  cout << endl << "Log to file" << endl;
  Report::FileOutput log_file("report.log", 1, 5, true);
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
  if (missing_log_file.open() < 0) {
    hlog_error("%s creating log file: '%s'", strerror(errno), "missing/report.log");
  }
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
  Report my_report("my report");
  tl_report = &my_report;
  hlog_alert("some message with a number %d", 9);
  hlog_info("message with a number %d", 10);
  hlog_debug("with a number %d", 11);
  tl_report = NULL;


  cout << endl << "Log to file, no LF" << endl;
  Report::FileOutput log_file2("report2.log");
  if (log_file2.open() < 0) {
    hlog_error("%s opening log file", strerror(errno));
    return 0;
  }
  report.add(&log_file2);
  report.stopConsoleLog();
  report.setLevel(debug);
  hlog_generic(info, Report::HLOG_NOLINEFEED, -1, "message start...");
  hlog_generic(info, Report::HLOG_NOLINEFEED, -1, "middle...");
  hlog_verbose("different level");
  hlog_generic(info, Report::HLOG_NOLINEFEED, -1, "message start...");
  hlog_info("end");
  hlog_info("other message");
  log_file2.close();
  int sys_rc = 0;
  sys_rc = system("cat report2.log");


  cout << endl << "End of tests" << endl;
  return 0;
}
