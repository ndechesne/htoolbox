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

int main(void) {
  Config*     config;

  setVerbosityLevel(debug);

  cout << endl << "Test: client configuration" << endl;
  config = new Config;

  // expire
  config->add(new ConfigItem("expire", 0, 1, 1));

  // filter
  {
    ConfigItem* item = new ConfigItem("filter", 0, 0, 2);
    config->add(item);

    // condition
    item->add(new ConfigItem("condition", 1, 0, 2));
  }

  // path
  {
    ConfigItem* item = new ConfigItem("path", 1, 0, 1);
    config->add(item);

    // parser
    item->add(new ConfigItem("parser", 0, 0, 1, 2));

    // filter
    {
      ConfigItem* item2 = new ConfigItem("filter", 0, 0, 2);
      item->add(item2);

      // condition
      item2->add(new ConfigItem("condition", 1, 0, 2));
    }

    // ignore
    item->add(new ConfigItem("ignore", 0, 1, 1));

    // compress
    item->add(new ConfigItem("compress", 0, 1, 1));
  }

  Stream client_config("etc/localhost.list");
  if (client_config.open("r") == 0) {
    config->read(client_config);
    client_config.close();
  } else {
    cout << "failed to open it!" << endl;
  }
  config->clear();
  if (client_config.open("r") == 0) {
    config->read(client_config, Stream::flags_accept_cr_lf);
    client_config.close();
  } else {
    cout << "failed to open it!" << endl;
  }

  // show debug
  config->show();

  delete config;

  cout << "Test: server configuration" << endl;
  config = new Config;

  // db
  config->add(new ConfigItem("db", 0, 1, 1));

  // trash
  config->add(new ConfigItem("trash", 0, 1, 1));

  // filter
  {
    ConfigItem* item = new ConfigItem("filter", 0, 0, 2);
    config->add(item);

    // condition
    item->add(new ConfigItem("condition", 1, 0, 2));
  }

  // client
  {
    ConfigItem* item = new ConfigItem("client", 1, 0, 1, 2);
    config->add(item);

    // hostname
    item->add(new ConfigItem("hostname", 0, 1, 1));

    // protocol
    item->add(new ConfigItem("protocol", 1, 1, 1));

    // option
    item->add(new ConfigItem("option", 0, 0, 1, 2));

    // timeout_nowarning
    item->add(new ConfigItem("timeout_nowarning", 0, 1));

    // config
    item->add(new ConfigItem("config", 0, 1, 1));

    // expire
    item->add(new ConfigItem("expire", 0, 1, 1));

    // filter
    {
      ConfigItem* sub_item = new ConfigItem("filter", 0, 0, 2);
      item->add(sub_item);

      // condition
      sub_item->add(new ConfigItem("condition", 1, 0, 2));
    }

    // path
    {
      ConfigItem* sub_item = new ConfigItem("path", 0, 0, 1);
      item->add(sub_item);

      // parser
      sub_item->add(new ConfigItem("parser", 0, 0, 1, 2));

      // filter
      {
        ConfigItem* item2 = new ConfigItem("filter", 0, 0, 2);
        sub_item->add(item2);

        // condition
        item2->add(new ConfigItem("condition", 1, 0, 2));
      }

      // ignore
      sub_item->add(new ConfigItem("ignore", 0, 1, 1));

      // compress
      sub_item->add(new ConfigItem("compress", 0, 1, 1));
    }
  }

  Stream general_config("etc/hbackup.conf");
  if (general_config.open("r") == 0) {
    config->read(general_config);
    general_config.close();
  } else {
    cout << "failed to open it!" << endl;
  }

  // show debug
  config->show();

  // clear list
  config->clear();

  cout << "Test: broken configuration" << endl;
  Stream broken_config("etc/broken.conf");
  if (broken_config.open("r") == 0) {
    config->read(broken_config);
    broken_config.close();
  } else {
    cout << "failed to open it!" << endl;
  }

  // show debug
  config->show();

  delete config;

  return 0;
}
