/*
    Copyright (C) 2010 - 2011  Hervé Fache

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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <report.h>
#include "hasher.h"
#include "filereaderwriter.h"

using namespace htoolbox;

int main() {
  report.setLevel(regression);
  int sys_rc = 0;

  {
    FileReaderWriter frw("testfile", true);
    char hash[64];
    Hasher hh(&frw, false, Hasher::md5, hash);
    IReaderWriter& fd = hh;
    if (fd.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      char buffer[5000];
      memset(buffer, 0x41, sizeof(buffer));
      ssize_t rc;
      size_t count = 0;
      do {
        size_t size;
        if (count == 0) {
          size = 3000;
        } else {
          size = sizeof(buffer) - count;
        }
        rc = fd.write(&buffer[count], size);
        if (rc < 0) {
          hlog_regression("%s writing file", strerror(errno));
        } else {
          hlog_regression("written %zd bytes", rc);
        }
        count += rc;
      } while (rc > 0);
      if (fd.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      } else {
        hlog_regression("hash = '%s'", hash);
      }
    }
    sys_rc = system("md5sum testfile");
  }

  {
    char buffer[6000];
    memset(buffer, 0, sizeof(buffer));
    size_t size = 0;

    FileReaderWriter frw("testfile", false);
    char hash[129];
    Hasher hh(&frw, false, Hasher::md5, hash);
    IReaderWriter& fd = hh;
    if (fd.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      ssize_t rc;
      size_t count = 0;
      do {
        if (count == 0) {
          size = 4096;
        } else {
          size = sizeof(buffer) - count;
        }
        rc = fd.read(&buffer[count], size);
        if (rc < 0) {
          hlog_regression("%s reading file", strerror(errno));
        } else {
          hlog_regression("read %zd bytes", rc);
        }
        count += rc;
      } while (rc > 0);
      if (fd.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      } else {
        hlog_regression("hash = '%s'", hash);
      }
      size = count;
    }

    FileReaderWriter frw2("testfile2", true);
    Hasher hh2(&frw2, false, Hasher::md5, hash);
    IReaderWriter& fd2 = hh2;
    if (fd2.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      ssize_t rc;
      rc = fd2.write(buffer, size);
      if (rc < 0) {
        hlog_regression("%s writing file", strerror(errno));
      } else {
        hlog_regression("written %zd bytes", rc);
      }
      if (fd2.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      } else {
        hlog_regression("hash = '%s'", hash);
      }
    }
    sys_rc = system("md5sum testfile2");
  }

  {
    FileReaderWriter frw("testfile", true);
    char hash[64];
    Hasher hh(&frw, false, Hasher::sha1, hash);
    IReaderWriter& fd = hh;
    if (fd.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      char buffer[5000];
      memset(buffer, 0x41, sizeof(buffer));
      ssize_t rc;
      size_t count = 0;
      do {
        size_t size;
        if (count == 0) {
          size = 3000;
        } else {
          size = sizeof(buffer) - count;
        }
        rc = fd.write(&buffer[count], size);
        if (rc < 0) {
          hlog_regression("%s writing file", strerror(errno));
        } else {
          hlog_regression("written %zd bytes", rc);
        }
        count += rc;
      } while (rc > 0);
      if (fd.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      } else {
        hlog_regression("hash = '%s'", hash);
      }
    }
    sys_rc = system("sha1sum testfile");
  }

  {
    char buffer[6000];
    memset(buffer, 0, sizeof(buffer));
    size_t size = 0;

    FileReaderWriter frw("testfile", false);
    char hash[64];
    Hasher hh(&frw, false, Hasher::sha1, hash);
    IReaderWriter& fd = hh;
    if (fd.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      ssize_t rc;
      size_t count = 0;
      do {
        if (count == 0) {
          size = 4096;
        } else {
          size = sizeof(buffer) - count;
        }
        rc = fd.read(&buffer[count], size);
        if (rc < 0) {
          hlog_regression("%s reading file", strerror(errno));
        } else {
          hlog_regression("read %zd bytes", rc);
        }
        count += rc;
      } while (rc > 0);
      if (fd.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      } else {
        hlog_regression("hash = '%s'", hash);
      }
      size = count;
    }

    FileReaderWriter frw2("testfile2", true);
    Hasher hh2(&frw2, false, Hasher::sha1, hash);
    IReaderWriter& fd2 = hh2;
    if (fd2.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      ssize_t rc;
      rc = fd2.write(buffer, size);
      if (rc < 0) {
        hlog_regression("%s writing file", strerror(errno));
      } else {
        hlog_regression("written %zd bytes", rc);
      }
      if (fd2.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      } else {
        hlog_regression("hash = '%s'", hash);
      }
    }
    sys_rc = system("sha1sum testfile2");
  }

  {
    FileReaderWriter frw("testfile", true);
    char hash[64];
    Hasher hh(&frw, false, Hasher::sha256, hash);
    IReaderWriter& fd = hh;
    if (fd.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      char buffer[5000];
      memset(buffer, 0x41, sizeof(buffer));
      ssize_t rc;
      size_t count = 0;
      do {
        size_t size;
        if (count == 0) {
          size = 3000;
        } else {
          size = sizeof(buffer) - count;
        }
        rc = fd.write(&buffer[count], size);
        if (rc < 0) {
          hlog_regression("%s writing file", strerror(errno));
        } else {
          hlog_regression("written %zd bytes", rc);
        }
        count += rc;
      } while (rc > 0);
      if (fd.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      } else {
        hlog_regression("hash = '%s'", hash);
      }
    }
    sys_rc = system("sha256sum testfile");
  }

  {
    char buffer[6000];
    memset(buffer, 0, sizeof(buffer));
    size_t size = 0;

    FileReaderWriter frw("testfile", false);
    char hash[64];
    Hasher hh(&frw, false, Hasher::sha256, hash);
    IReaderWriter& fd = hh;
    if (fd.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      ssize_t rc;
      size_t count = 0;
      do {
        if (count == 0) {
          size = 4096;
        } else {
          size = sizeof(buffer) - count;
        }
        rc = fd.read(&buffer[count], size);
        if (rc < 0) {
          hlog_regression("%s reading file", strerror(errno));
        } else {
          hlog_regression("read %zd bytes", rc);
        }
        count += rc;
      } while (rc > 0);
      if (fd.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      } else {
        hlog_regression("hash = '%s'", hash);
      }
      size = count;
    }

    FileReaderWriter frw2("testfile2", true);
    Hasher hh2(&frw2, false, Hasher::sha256, hash);
    IReaderWriter& fd2 = hh2;
    if (fd2.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      ssize_t rc;
      rc = fd2.write(buffer, size);
      if (rc < 0) {
        hlog_regression("%s writing file", strerror(errno));
      } else {
        hlog_regression("written %zd bytes", rc);
      }
      if (fd2.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      } else {
        hlog_regression("hash = '%s'", hash);
      }
    }
    sys_rc = system("sha256sum testfile2");
  }

  {
    FileReaderWriter frw("testfile", true);
    char hash[64];
    Hasher hh(&frw, false, Hasher::sha512, hash);
    IReaderWriter& fd = hh;
    if (fd.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      char buffer[5000];
      memset(buffer, 0x41, sizeof(buffer));
      ssize_t rc;
      size_t count = 0;
      do {
        size_t size;
        if (count == 0) {
          size = 3000;
        } else {
          size = sizeof(buffer) - count;
        }
        rc = fd.write(&buffer[count], size);
        if (rc < 0) {
          hlog_regression("%s writing file", strerror(errno));
        } else {
          hlog_regression("written %zd bytes", rc);
        }
        count += rc;
      } while (rc > 0);
      if (fd.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      } else {
        hlog_regression("hash = '%s'", hash);
      }
    }
    sys_rc = system("sha512sum testfile");
  }

  {
    char buffer[6000];
    memset(buffer, 0, sizeof(buffer));
    size_t size = 0;

    FileReaderWriter frw("testfile", false);
    char hash[64];
    Hasher hh(&frw, false, Hasher::sha512, hash);
    IReaderWriter& fd = hh;
    if (fd.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      ssize_t rc;
      size_t count = 0;
      do {
        if (count == 0) {
          size = 4096;
        } else {
          size = sizeof(buffer) - count;
        }
        rc = fd.read(&buffer[count], size);
        if (rc < 0) {
          hlog_regression("%s reading file", strerror(errno));
        } else {
          hlog_regression("read %zd bytes", rc);
        }
        count += rc;
      } while (rc > 0);
      if (fd.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      } else {
        hlog_regression("hash = '%s'", hash);
      }
      size = count;
    }

    FileReaderWriter frw2("testfile2", true);
    Hasher hh2(&frw2, false, Hasher::sha512, hash);
    IReaderWriter& fd2 = hh2;
    if (fd2.open() < 0) {
      hlog_regression("%s opening file", strerror(errno));
    } else {
      ssize_t rc;
      rc = fd2.write(buffer, size);
      if (rc < 0) {
        hlog_regression("%s writing file", strerror(errno));
      } else {
        hlog_regression("written %zd bytes", rc);
      }
      if (fd2.close() < 0) {
        hlog_regression("%s closing file", strerror(errno));
      } else {
        hlog_regression("hash = '%s'", hash);
      }
    }
    sys_rc = system("sha512sum testfile2");
  }

  return 0;
}
