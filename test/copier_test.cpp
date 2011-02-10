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

#include <errno.h>
#include <string.h>

#include <list>

using namespace std;

#include <report.h>
#include <filereaderwriter.h>
#include <nullwriter.h>
#include <hasher.h>
#include "copier.h"

using namespace htoolbox;

int main(void) {
  enum {
    chunk_size = 64000
  };

  hlog_info("Create test input");
  {
    FileReaderWriter fw("intput", true);
    char hash[256];
    Hasher fd(&fw, false, Hasher::sha1, hash);
    if (fd.open() < 0) {
      hlog_error("%s opening", strerror(errno));
    } else {
      char buffer[64];
      for (int i = 0; i < 64; ++i) {
        buffer[i] = static_cast<char>(i + 32);
      }
      for (int i = 0; i < 10000; ++i) {
        if (fd.put(buffer, sizeof(buffer)) < 0) {
          hlog_error("%s writing", strerror(errno));
        }
      }
      if (fd.close() < 0) {
        hlog_error("%s closing", strerror(errno));
      } else {
        hlog_info("Hash = %s", hash);
      }
    }
  }
  FileReaderWriter fr("intput", false);
  if (fr.open() < 0) {
    hlog_error("%s opening", strerror(errno));
  } else {
    if (fr.close() < 0) {
      hlog_error("%s closing", strerror(errno));
    }
  }
  FileReaderWriter fw("output", true);
  char hash[256];
  Hasher hh(&fw, false, Hasher::sha1, hash);
  Copier cp(&fr, false, chunk_size);
  cp.addDest(&hh, false);


  hlog_info("\nstream() tests");

  // Test one chunk
  if (cp.open() < 0) {
    hlog_error("%s opening", strerror(errno));
  } else {
    ssize_t rc = cp.read();
    if (rc < 0) {
      hlog_error("%s copying", strerror(errno));
    } else {
      hlog_info("Size = %zd", rc);
    }
    if (cp.close() < 0) {
      hlog_error("%s closing", strerror(errno));
    } else {
      hlog_info("Hash = %s", hash);
    }
  }

  // Test two half-chunks
  if (cp.open() < 0) {
    hlog_error("%s opening", strerror(errno));
  } else {
    ssize_t rc = cp.read(NULL, chunk_size / 2);
    if (rc < 0) {
      hlog_error("%s copying", strerror(errno));
    } else {
      hlog_info("Size = %zd", rc);
    }
    rc = cp.read(NULL, chunk_size / 2);
    if (rc < 0) {
      hlog_error("%s copying", strerror(errno));
    } else {
      hlog_info("Size = %zd", rc);
    }
    if (cp.close() < 0) {
      hlog_error("%s closing", strerror(errno));
    } else {
      hlog_info("Hash = %s", hash);
    }
  }

  // Test four quarter-chunks and data get
  if (cp.open() < 0) {
    hlog_error("%s opening", strerror(errno));
  } else {
    char buffer[chunk_size];
    ssize_t rc = cp.read(&buffer[0 * chunk_size / 4], chunk_size / 4);
    if (rc < 0) {
      hlog_error("%s copying", strerror(errno));
    } else {
      hlog_info("Size = %zd", rc);
    }
    rc = cp.read(&buffer[1 * chunk_size / 4], chunk_size / 4);
    if (rc < 0) {
      hlog_error("%s copying", strerror(errno));
    } else {
      hlog_info("Size = %zd", rc);
    }
    rc = cp.read(&buffer[2 * chunk_size / 4], chunk_size / 4);
    if (rc < 0) {
      hlog_error("%s copying", strerror(errno));
    } else {
      hlog_info("Size = %zd", rc);
    }
    rc = cp.read(&buffer[3 * chunk_size / 4], chunk_size / 4);
    if (rc < 0) {
      hlog_error("%s copying", strerror(errno));
    } else {
      hlog_info("Size = %zd", rc);
    }
    if (cp.close() < 0) {
      hlog_error("%s closing", strerror(errno));
    } else {
      hlog_info("Hash = %s", hash);
    }
    NullWriter nl;
    char buffer_hash[256];
    Hasher hn(&nl, false, Hasher::sha1, buffer_hash);
    if ((hn.open() < 0) || (hn.put(buffer, sizeof(buffer)) < chunk_size) ||
        (hn.close() < 0)) {
      hlog_error("%s computing buffer hash", strerror(errno));
    } else {
      hlog_info("Buffer hash = %s", buffer_hash);
    }
  }

  // Test double-chunk
  if (cp.open() < 0) {
    hlog_error("%s opening", strerror(errno));
  } else {
    ssize_t rc = cp.read(NULL, 2 * chunk_size);
    if (rc < 0) {
      hlog_error("%s copying", strerror(errno));
    } else {
      hlog_info("Size = %zd", rc);
    }
    if (cp.close() < 0) {
      hlog_error("%s closing", strerror(errno));
    } else {
      hlog_info("Hash = %s", hash);
    }
  }


  hlog_info("\ncopy() tests");

  // Test full copy
  if (cp.open() < 0) {
    hlog_error("%s opening", strerror(errno));
  } else {
    ssize_t rc = cp.get();
    if (rc < 0) {
      hlog_error("%s copying", strerror(errno));
    } else {
      hlog_info("Size = %zd", rc);
    }
    if (cp.close() < 0) {
      hlog_error("%s closing", strerror(errno));
    } else {
      hlog_info("Hash = %s", hash);
    }
  }

  // Test oversized copy
  if (cp.open() < 0) {
    hlog_error("%s opening", strerror(errno));
  } else {
    ssize_t rc = cp.get(NULL, 10000000);
    if (rc < 0) {
      hlog_error("%s copying", strerror(errno));
    } else {
      hlog_info("Size = %zd", rc);
    }
    if (cp.close() < 0) {
      hlog_error("%s closing", strerror(errno));
    } else {
      hlog_info("Hash = %s", hash);
    }
  }

  // Test chunk-sized copy
  if (cp.open() < 0) {
    hlog_error("%s opening", strerror(errno));
  } else {
    ssize_t rc = cp.get(NULL, chunk_size);
    if (rc < 0) {
      hlog_error("%s copying", strerror(errno));
    } else {
      hlog_info("Size = %zd", rc);
    }
    if (cp.close() < 0) {
      hlog_error("%s closing", strerror(errno));
    } else {
      hlog_info("Hash = %s", hash);
    }
  }

  // Test double-chunk-sized copy
  if (cp.open() < 0) {
    hlog_error("%s opening", strerror(errno));
  } else {
    ssize_t rc = cp.get(NULL, 2 * chunk_size);
    if (rc < 0) {
      hlog_error("%s copying", strerror(errno));
    } else {
      hlog_info("Size = %zd", rc);
    }
    if (cp.close() < 0) {
      hlog_error("%s closing", strerror(errno));
    } else {
      hlog_info("Hash = %s", hash);
    }
  }

  // Test quadruple-chunk-sized copy and data get
  if (cp.open() < 0) {
    hlog_error("%s opening", strerror(errno));
  } else {
    char buffer[4 * chunk_size];
    ssize_t rc = cp.get(buffer, 4 * chunk_size);
    if (rc < 0) {
      hlog_error("%s copying", strerror(errno));
    } else {
      hlog_info("Size = %zd", rc);
    }
    if (cp.close() < 0) {
      hlog_error("%s closing", strerror(errno));
    } else {
      hlog_info("Hash = %s", hash);
    }
    NullWriter nl;
    char buffer_hash[256];
    Hasher hn(&nl, false, Hasher::sha1, buffer_hash);
    if ((hn.open() < 0) || (hn.put(buffer, sizeof(buffer)) < chunk_size) ||
        (hn.close() < 0)) {
      hlog_error("%s computing buffer hash", strerror(errno));
    } else {
      hlog_info("Buffer hash = %s", buffer_hash);
    }
  }


  hlog_info("\nwrite() tests");

  // stream half size, write, stream full size, write, copy part, write, copy
  if (cp.open() < 0) {
    hlog_error("%s opening", strerror(errno));
  } else {
    char buffer[] = "abcdefghijklmnopqrstuvwxyz";
    ssize_t rc = cp.read(NULL, chunk_size / 2);
    if (rc < 0) {
      hlog_error("%s copying", strerror(errno));
    } else {
      hlog_info("Size = %zd", rc);
    }
    rc = cp.put(buffer, strlen(buffer));
    if (rc < 0) {
      hlog_error("%s copying", strerror(errno));
    } else {
      hlog_info("Size = %zd", rc);
    }
    rc = cp.read();
    if (rc < 0) {
      hlog_error("%s copying", strerror(errno));
    } else {
      hlog_info("Size = %zd", rc);
    }
    rc = cp.put(buffer, strlen(buffer));
    if (rc < 0) {
      hlog_error("%s copying", strerror(errno));
    } else {
      hlog_info("Size = %zd", rc);
    }
    rc = cp.get(NULL, 3 * chunk_size);
    if (rc < 0) {
      hlog_error("%s copying", strerror(errno));
    } else {
      hlog_info("Size = %zd", rc);
    }
    rc = cp.put(buffer, strlen(buffer));
    if (rc < 0) {
      hlog_error("%s copying", strerror(errno));
    } else {
      hlog_info("Size = %zd", rc);
    }
    rc = cp.get();
    if (rc < 0) {
      hlog_error("%s copying", strerror(errno));
    } else {
      hlog_info("Size = %zd", rc);
    }
    if (cp.close() < 0) {
      hlog_error("%s closing", strerror(errno));
    } else {
      hlog_info("Hash = %s", hash);
    }
  }

  hlog_info("\nEnd of tests");
  return 0;
}
