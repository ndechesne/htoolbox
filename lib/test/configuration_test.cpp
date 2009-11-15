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
  Config*           config;
  ConfigErrors      errors;

  setVerbosityLevel(debug);

  cout << endl << "Test: client configuration" << endl;
  ConfigSyntax client_syntax;

  // expire
  client_syntax.add(new ConfigItem("expire", 0, 1, 1));

  // users
  client_syntax.add(new ConfigItem("users", 0, 1, 1, -1));

  // ignore
  client_syntax.add(new ConfigItem("ignore", 0, 1, 1));

  // filter
  {
    ConfigItem* item = new ConfigItem("filter", 0, 0, 2);
    client_syntax.add(item);

    // condition
    item->add(new ConfigItem("condition", 1, 0, 2));
  }

  // path
  {
    ConfigItem* item = new ConfigItem("path", 1, 0, 1);
    client_syntax.add(item);

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

    // no_compress
    item->add(new ConfigItem("no_compress", 0, 1, 1));
  }

  // show debug
  cout << "Syntax:" << endl;
  client_syntax.show(2);

  config = new Config;
  Stream client_config("etc/localhost.list");
  if (client_config.open(O_RDONLY) == 0) {
    config->read(client_config, 0, client_syntax, &errors);
    client_config.close();
  } else {
    cout << "failed to open it!" << endl;
  }
  errors.show();
  errors.clear();
  config->clear();
  if (client_config.open(O_RDONLY) == 0) {
    config->read(client_config, Stream::flags_accept_cr_lf, client_syntax,
      &errors);
    client_config.close();
  } else {
    cout << "failed to open it!" << endl;
  }
  errors.show();
  errors.clear();

  // show debug
  config->show();

  delete config;

  cout << "Test: server configuration" << endl;
  ConfigSyntax syntax;

  // db
  {
    ConfigItem* item = new ConfigItem("db", 0, 1, 1);
    syntax.add(item);

    // compress
    item->add(new ConfigItem("compress", 0, 1, 1, 1));

    // compress_auto: to be ignored
    item->add(new ConfigItem("compress_auto", 0, 1));
  }

  // filter
  {
    ConfigItem* item = new ConfigItem("filter", 0, 0, 2);
    syntax.add(item);

    // condition
    item->add(new ConfigItem("condition", 1, 0, 2));
  }

  // ignore
  syntax.add(new ConfigItem("ignore", 0, 1, 1));

  // timeout_nowarning
  syntax.add(new ConfigItem("timeout_nowarning", 0, 1));

  // report_copy_error_once
  syntax.add(new ConfigItem("report_copy_error_once", 0, 1));

  // client
  {
    ConfigItem* item = new ConfigItem("client", 1, 0, 1, 2);
    syntax.add(item);

    // hostname
    item->add(new ConfigItem("hostname", 0, 1, 1));

    // protocol
    item->add(new ConfigItem("protocol", 1, 1, 1));

    // option
    item->add(new ConfigItem("option", 0, 0, 1, 2));

    // timeout_nowarning
    item->add(new ConfigItem("timeout_nowarning", 0, 1));

    // report_copy_error_once
    item->add(new ConfigItem("report_copy_error_once", 0, 1));

    // config
    item->add(new ConfigItem("config", 0, 1, 1));

    // expire
    item->add(new ConfigItem("expire", 0, 1, 1));

    // users
    item->add(new ConfigItem("users", 0, 1, 1, -1));

    // ignore
    item->add(new ConfigItem("ignore", 0, 1, 1));

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

      // no_compress
      sub_item->add(new ConfigItem("no_compress", 0, 1, 1));
    }
  }

  // show debug
  cout << "Syntax:" << endl;
  syntax.show(2);

  config = new Config;
  Stream general_config("etc/hbackup.conf");
  if (general_config.open(O_RDONLY) == 0) {
    config->read(general_config, 0, syntax, &errors);
    general_config.close();
  } else {
    cout << "failed to open it!" << endl;
  }
  errors.show();
  errors.clear();

  // show debug
  config->show();

  cout << "Test: configuration save" << endl;

  // save configuration
  Stream save_config("etc/config.gz");
  if (save_config.open(O_WRONLY, 5) == 0) {
    if (config->write(save_config)) {
      cout << "failed to save it!" << endl;
    }
    save_config.close();
  } else {
    cout << "failed to open it!" << endl;
  }

  // clear list
  config->clear();

  if (save_config.open(O_RDONLY, 1) == 0) {
    config->read(save_config, 0, syntax, &errors);
    save_config.close();
  } else {
    cout << "failed to open it!" << endl;
  }
  errors.show();
  errors.clear();

  // show debug
  config->show();

  // clear list
  config->clear();

  cout << "Test: broken configuration" << endl;
  Stream broken_config("etc/broken.conf");
  if (broken_config.open(O_RDONLY) == 0) {
    config->read(broken_config, 0, syntax, &errors);
    broken_config.close();
  } else {
    cout << "failed to open it!" << endl;
  }
  errors.show();
  errors.clear();

  // show debug
  config->show();

  delete config;

  return 0;
}
