/*
    Copyright (C) 2011  Hervé Fache

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

#include <string.h>

#include "criticality.h"

using namespace htoolbox;

const char* Criticality::toString() const {
  switch (_level) {
    case alert:
      return "alert";
    case error:
      return "error";
    case warning:
      return "warning";
    case info:
      return "info";
    case verbose:
      return "verbose";
    case debug:
      return "debug";
    case regression:
      return "regression";
  }
  return "unknown";
}

int Criticality::setFromInt(int level) {
  if ((level < alert) || (level > regression)) {
    return -1;
  }
  _level = static_cast<Level>(level);
  return 0;
}

int Criticality::setFromString(const char* str) {
  int rc = -1;
  size_t len = strlen(str);
  switch (str[0]) {
    case 'a':
    case 'A':
      if (strncasecmp(str, "alert", len) == 0) {
        _level = alert;
        rc = 0;
      }
      break;
    case 'e':
    case 'E':
      if (strncasecmp(str, "error", len) == 0) {
        _level = error;
        rc = 0;
      }
      break;
    case 'w':
    case 'W':
      if (strncasecmp(str, "warning", len) == 0) {
        _level = warning;
        rc = 0;
      }
      break;
    case 'i':
    case 'I':
      if (strncasecmp(str, "info", len) == 0) {
        _level = info;
        rc = 0;
      }
      break;
    case 'v':
    case 'V':
      if (strncasecmp(str, "verbose", len) == 0) {
        _level = verbose;
        rc = 0;
      }
      break;
    case 'd':
    case 'D':
      if (strncasecmp(str, "debug", len) == 0) {
        _level = debug;
        rc = 0;
      }
      break;
    case 'r':
    case 'R':
      if (strncasecmp(str, "regression", len) == 0) {
        _level = regression;
        rc = 0;
      }
      break;
  }
  return rc;
}
