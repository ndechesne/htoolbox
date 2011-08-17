#include <limits.h>

#include <report.h>
#include "shared_path.h"

using namespace htoolbox;

int main(void) {
  report.setLevel(debug);

  char path[PATH_MAX] = "/my/path";
  hlog_info("path = '%s', length = %zu", path, strlen(path));
  {
    SharedPath spath(path, strlen(path), "ends");
    hlog_info("path = '%s', length = %zu", path, spath.length());
    spath.accept();
    hlog_info("path = '%s', length = %zu", path, spath.length());
    spath.append("here");
    hlog_info("path = '%s', length = %zu", path, spath.length());
    spath.append(10);
    hlog_info("path = '%s', length = %zu", path, spath.length());
  }
  hlog_info("path = '%s', length = %zu", path, strlen(path));

  return 0;
}
