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

#include "files.h"
#include "configuration.h"
#include "hbackup.h"

using namespace hbackup;

int hbackup::verbosity(void) {
  return 2;
}

int hbackup::terminating(const char* function) {
  return 0;
}

int main(void) {
  Config*     config;

  cout << "Test: server configuration" << endl;
  config = new Config;

  // db
  config->add(new ConfigItem("db", 0, 1));

  // trash
  config->add(new ConfigItem("trash", 0, 1));

  // filter
  {
    ConfigItem* item = new ConfigItem("filter", 0, 0);
    config->add(item);

    // condition
    item->add(new ConfigItem("condition", 1, 0));
  }

  // client
  {
    ConfigItem* item = new ConfigItem("client", 1, 0);
    config->add(item);

    // hostname
    item->add(new ConfigItem("hostname", 0, 1));

    // option
    item->add(new ConfigItem("option", 0, 0));

    // config
    item->add(new ConfigItem("config", 1, 1));

    // expire
    item->add(new ConfigItem("expire", 0, 1));
  }

  // show debug
  config->debug();

  delete config;

  cout << endl << "Test: client configuration" << endl;
  config = new Config;

  // db
  config->add(new ConfigItem("expire", 0, 1));

  // filter
  {
    ConfigItem* item = new ConfigItem("filter", 0, 0);
    config->add(item);

    // condition
    item->add(new ConfigItem("condition", 1, 0));
  }

  // path
  {
    ConfigItem* item = new ConfigItem("path", 1, 0);
    config->add(item);

    // parser
    item->add(new ConfigItem("parser", 0, 0));

    // filter
    {
      ConfigItem* item2 = new ConfigItem("filter", 0, 0);
      item->add(item2);

      // condition
      item2->add(new ConfigItem("condition", 1, 0));
    }

    // ignore
    item->add(new ConfigItem("ignore", 0, 1));

    // compress
    item->add(new ConfigItem("compress", 0, 1));
  }

  // show debug
  config->debug();

  delete config;

  return 0;
}
