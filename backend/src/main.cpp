/*
     Copyright (C) 2010  Herve Fache

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

#include <stdint.h>

#include <vector>
#include <list>
#include <string>

using namespace std;

#include <report.h>
#include <configuration.h>

using namespace htools;

#include <hbackup.h>  // progress_f (needs architecture review)
#include <data.h>

using namespace hbackup;

// static const char *default_config_path = "/etc/hbackup/hbackend.conf";
static const char *default_config_path = "../hbackend.conf";

struct LocalConfig : Config::Object {
  string    hostname;
  uint16_t  port;
  LocalConfig() : port(0) {}
  Object* configChildFactory(
    const vector<string>& params,
    const char*           path,
    size_t                line_no);
  void show() {
    hlog_verbose("hostname = '%s'", hostname.c_str());
    hlog_verbose("port     = %d", port);
  }
};

Config::Object* LocalConfig::configChildFactory(
    const vector<string>& params,
    const char*           path,
    size_t                line_no) {
  Object* co = NULL;
  const string& keyword = params[0];
  if (keyword == "hostname") {
    hostname = params[1];
    co = this;
  } else
  if (keyword == "port") {
    port = static_cast<uint16_t>(atoi(params[1].c_str()));
    co = this;
  } else
  {
    hlog_error("%s:%zu: unknown keyword '%s'", path, line_no, keyword.c_str());
  }
  return co;
}

int read_config(const char* path, LocalConfig& conf) {
  Config config;
  config.syntaxAdd(config.syntaxRoot(), "hostname", 0, 1, 1);
  config.syntaxAdd(config.syntaxRoot(), "port", 0, 1, 1);
  hlog_debug("Reading configuration file '%s'", path);
  ssize_t rc = config.read(path, 0, &conf);
  if (rc < 0) {
    return -1;
  }
  return 0;
}

int main(void) {
  report.setLevel(debug);

  LocalConfig config;
  if (read_config(default_config_path, config) < 0) {
    return 1;
  }
  config.show();
  return 0;
}
