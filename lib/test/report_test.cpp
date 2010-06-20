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

#include <iostream>

using namespace std;

#include "hreport.h"

using namespace hreport;

int main(void) {
  cout << "Report tests" << endl;

  cout << endl << "Default behaviour" << endl;
  out(info, msg_standard, "default level");
  out(alert, msg_standard, "alert level");
  out(alert, msg_standard, "alert 0 level", 0);
  out(error, msg_standard, "error level");
  out(error, msg_standard, "error 0 level", 0);
  out(warning, msg_standard, "warning level");
  out(warning, msg_standard, "warning 0 level", 0);
  out(warning, msg_standard, "warning 1 level", 1);
  out(warning, msg_standard, "warning 2 level", 2);
  out(info, msg_standard, "info level");
  out(info, msg_standard, "info 0 level", 0);
  out(info, msg_standard, "info 1 level", 1);
  out(info, msg_standard, "info 2 level", 2);
  out(verbose, msg_standard, "verbose level");
  out(verbose, msg_standard, "verbose 0 level", 0);
  out(verbose, msg_standard, "verbose 1 level", 1);
  out(verbose, msg_standard, "verbose 2 level", 2);
  out(debug, msg_standard, "debug level");
  out(debug, msg_standard, "debug 0 level", 0);
  out(debug, msg_standard, "debug 1 level", 1);
  out(debug, msg_standard, "debug 2 level", 2);

  cout << endl << "Verbosity level: alert" << endl;
  Report::setLevel(alert);
  out(info, msg_standard, "default level");
  out(alert, msg_standard, "alert level");
  out(alert, msg_standard, "alert 0 level", 0);
  out(error, msg_standard, "error level");
  out(error, msg_standard, "error 0 level", 0);
  out(warning, msg_standard, "warning level");
  out(warning, msg_standard, "warning 0 level", 0);
  out(warning, msg_standard, "warning 1 level", 1);
  out(warning, msg_standard, "warning 2 level", 2);
  out(info, msg_standard, "info level");
  out(info, msg_standard, "info 0 level", 0);
  out(info, msg_standard, "info 1 level", 1);
  out(info, msg_standard, "info 2 level", 2);
  out(verbose, msg_standard, "verbose level");
  out(verbose, msg_standard, "verbose 0 level", 0);
  out(verbose, msg_standard, "verbose 1 level", 1);
  out(verbose, msg_standard, "verbose 2 level", 2);
  out(debug, msg_standard, "debug level");
  out(debug, msg_standard, "debug 0 level", 0);
  out(debug, msg_standard, "debug 1 level", 1);
  out(debug, msg_standard, "debug 2 level", 2);

  cout << endl << "Verbosity level: error" << endl;
  Report::setLevel(error);
  out(info, msg_standard, "default level");
  out(alert, msg_standard, "alert level");
  out(alert, msg_standard, "alert 0 level", 0);
  out(error, msg_standard, "error level");
  out(error, msg_standard, "error 0 level", 0);
  out(warning, msg_standard, "warning level");
  out(warning, msg_standard, "warning 0 level", 0);
  out(warning, msg_standard, "warning 1 level", 1);
  out(warning, msg_standard, "warning 2 level", 2);
  out(info, msg_standard, "info level");
  out(info, msg_standard, "info 0 level", 0);
  out(info, msg_standard, "info 1 level", 1);
  out(info, msg_standard, "info 2 level", 2);
  out(verbose, msg_standard, "verbose level");
  out(verbose, msg_standard, "verbose 0 level", 0);
  out(verbose, msg_standard, "verbose 1 level", 1);
  out(verbose, msg_standard, "verbose 2 level", 2);
  out(debug, msg_standard, "debug level");
  out(debug, msg_standard, "debug 0 level", 0);
  out(debug, msg_standard, "debug 1 level", 1);
  out(debug, msg_standard, "debug 2 level", 2);

  cout << endl << "Verbosity level: warning" << endl;
  Report::setLevel(warning);
  out(info, msg_standard, "default level");
  out(alert, msg_standard, "alert level");
  out(alert, msg_standard, "alert 0 level", 0);
  out(error, msg_standard, "error level");
  out(error, msg_standard, "error 0 level", 0);
  out(warning, msg_standard, "warning level");
  out(warning, msg_standard, "warning 0 level", 0);
  out(warning, msg_standard, "warning 1 level", 1);
  out(warning, msg_standard, "warning 2 level", 2);
  out(info, msg_standard, "info level");
  out(info, msg_standard, "info 0 level", 0);
  out(info, msg_standard, "info 1 level", 1);
  out(info, msg_standard, "info 2 level", 2);
  out(verbose, msg_standard, "verbose level");
  out(verbose, msg_standard, "verbose 0 level", 0);
  out(verbose, msg_standard, "verbose 1 level", 1);
  out(verbose, msg_standard, "verbose 2 level", 2);
  out(debug, msg_standard, "debug level");
  out(debug, msg_standard, "debug 0 level", 0);
  out(debug, msg_standard, "debug 1 level", 1);
  out(debug, msg_standard, "debug 2 level", 2);

  cout << endl << "Verbosity level: info" << endl;
  Report::setLevel(info);
  out(info, msg_standard, "default level");
  out(alert, msg_standard, "alert level");
  out(alert, msg_standard, "alert 0 level", 0);
  out(error, msg_standard, "error level");
  out(error, msg_standard, "error 0 level", 0);
  out(warning, msg_standard, "warning level");
  out(warning, msg_standard, "warning 0 level", 0);
  out(warning, msg_standard, "warning 1 level", 1);
  out(warning, msg_standard, "warning 2 level", 2);
  out(info, msg_standard, "info level");
  out(info, msg_standard, "info 0 level", 0);
  out(info, msg_standard, "info 1 level", 1);
  out(info, msg_standard, "info 2 level", 2);
  out(verbose, msg_standard, "verbose level");
  out(verbose, msg_standard, "verbose 0 level", 0);
  out(verbose, msg_standard, "verbose 1 level", 1);
  out(verbose, msg_standard, "verbose 2 level", 2);
  out(debug, msg_standard, "debug level");
  out(debug, msg_standard, "debug 0 level", 0);
  out(debug, msg_standard, "debug 1 level", 1);
  out(debug, msg_standard, "debug 2 level", 2);

  cout << endl << "Verbosity level: verbose" << endl;
  Report::setLevel(verbose);
  out(info, msg_standard, "default level");
  out(alert, msg_standard, "alert level");
  out(alert, msg_standard, "alert 0 level", 0);
  out(error, msg_standard, "error level");
  out(error, msg_standard, "error 0 level", 0);
  out(warning, msg_standard, "warning level");
  out(warning, msg_standard, "warning 0 level", 0);
  out(warning, msg_standard, "warning 1 level", 1);
  out(warning, msg_standard, "warning 2 level", 2);
  out(info, msg_standard, "info level");
  out(info, msg_standard, "info 0 level", 0);
  out(info, msg_standard, "info 1 level", 1);
  out(info, msg_standard, "info 2 level", 2);
  out(verbose, msg_standard, "verbose level");
  out(verbose, msg_standard, "verbose 0 level", 0);
  out(verbose, msg_standard, "verbose 1 level", 1);
  out(verbose, msg_standard, "verbose 2 level", 2);
  out(debug, msg_standard, "debug level");
  out(debug, msg_standard, "debug 0 level", 0);
  out(debug, msg_standard, "debug 1 level", 1);
  out(debug, msg_standard, "debug 2 level", 2);

  cout << endl << "Verbosity level: debug" << endl;
  Report::setLevel(debug);
  out(info, msg_standard, "default level");
  out(alert, msg_standard, "alert level");
  out(alert, msg_standard, "alert 0 level", 0);
  out(error, msg_standard, "error level");
  out(error, msg_standard, "error 0 level", 0);
  out(warning, msg_standard, "warning level");
  out(warning, msg_standard, "warning 0 level", 0);
  out(warning, msg_standard, "warning 1 level", 1);
  out(warning, msg_standard, "warning 2 level", 2);
  out(info, msg_standard, "info level");
  out(info, msg_standard, "info 0 level", 0);
  out(info, msg_standard, "info 1 level", 1);
  out(info, msg_standard, "info 2 level", 2);
  out(verbose, msg_standard, "verbose level");
  out(verbose, msg_standard, "verbose 0 level", 0);
  out(verbose, msg_standard, "verbose 1 level", 1);
  out(verbose, msg_standard, "verbose 2 level", 2);
  out(debug, msg_standard, "debug level");
  out(debug, msg_standard, "debug 0 level", 0);
  out(debug, msg_standard, "debug 1 level", 1);
  out(debug, msg_standard, "debug 2 level", 2);

  cout << endl << "Line autoblanking" << endl;
  out(info, msg_standard, "", -3);
  out(info, msg_standard, "Short line", -3);
  out(info, msg_standard, "A longer line", -3);
  out(info, msg_standard, "Tiny one", -3);
  out(info, msg_standard, ".", -3);
  out(info, msg_standard, "A longer line", -3);
  out(info, msg_standard, "An even longer line", -3);
  out(info, msg_standard, "A normal line");


  cout << endl << "End of tests" << endl;

  return 0;
}
