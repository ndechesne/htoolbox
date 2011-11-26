/*
     Copyright (C) 2011 Herve Fache

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

#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>

#include "report.h"
#include "shared_path.h"
#include "filesystem.h"

using namespace htoolbox;

FsNode::FsNode(const char* n, mode_t m):
  name(strdup(n)), mode(m), uid(0), gid(0), sibling(NULL) {}

FsNode::~FsNode() {
  free(name);
  if (S_ISDIR(mode)) {
    FsNodeDir* t = static_cast<FsNodeDir*>(this);
    t->clear();
  } else
  if (S_ISLNK(mode)) {
    FsNodeLink* t = static_cast<FsNodeLink*>(this);
    free(t->string);
  }
}

FsNodeDir* FsNode::createRoot(const char* path) {
  FsNodeDir* node = new FsNodeDir(path == NULL ? "" : path);
  node->mode = S_IFDIR;
  return node;
}

FsNode* FsNode::createChild(const char* name, char type) {
  if (S_ISDIR(mode)) {
    FsNode* f;
    switch (type) {
      case 'd':
        f = new FsNodeDir(name);
        break;
      case 'f':
        f = new FsNodeFile(name);
        break;
      case 'l':
        f = new FsNodeLink(name);
        break;
      default:
        f = new FsNodeNotDir(name);
    }
    FsNodeDir* t = static_cast<FsNodeDir*>(this);
    f->sibling = t->children_head;
    t->children_head = f;
    return f;
  } else {
    return NULL;
  }
}

FsNode* FsNode::getNode(
  const char*       path,
  bool              create_missing) {
  if (path[0] == '/') {
    ++path;
  }
  const char* end = strchr(path, '/');
  size_t len;
  if (end == NULL) {
    len = strlen(path);
  } else {
    len = end - path;
  }
  if (len == 0) {
    return this;
  }
  if (S_ISDIR(mode)) {
    FsNodeDir* t = static_cast<FsNodeDir*>(this);
    FsNode* child = t->children_head;
    while (child != NULL) {
      // We use len because path is not 0-terminated, so check child name's length
      if ((strncmp(path, child->name, len) == 0) &&
          (child->name[len] == '\0')) {
        if (end == NULL) {
          // End of path
          return child;
        } else {
          return child->getNode(++end);
        }
      }
      child = child->sibling;
    }
    // The path may not start from /, so add the missing links if needed
    if (create_missing) {
      char missing_name[PATH_MAX];
      strncpy(missing_name, path, len);
      missing_name[len] = '\0';
      FsNode* missing = createChild(missing_name, 'd');
      static_cast<FsNodeDir*>(missing)->mode |= S_IFDIR | 0755;
      if (end == NULL) {
        return missing;
      } else {
        return missing->getNode(++end, true);
      }
    }
  }
  return NULL;
}

void FsNode::show(int level) const {
  char format[64];
  sprintf(format, "%%-%ds %%7o %%8jd %%s", 30 - level);
  if (S_ISDIR(mode)) {
    const FsNodeDir* t = static_cast<const FsNodeDir*>(this);
    off_t zero = 0;
    hlog_verbose_arrow(level, format, name, mode, zero, "");
    const FsNode* child = t->children_head;
    while (child != NULL) {
      child->show(level + 1);
      child = child->sibling;
    }
  } else
  if (S_ISREG(mode)) {
    const FsNodeFile* t = static_cast<const FsNodeFile*>(this);
    hlog_verbose_arrow(level, format, name, mode, t->size, t->hash);
  } else
  if (S_ISLNK(mode)) {
    const FsNodeLink* t = static_cast<const FsNodeLink*>(this);
    hlog_verbose_arrow(level, format, name, mode, t->size, t->string);
  } else
  {
    const FsNodeNotDir* t = static_cast<const FsNodeNotDir*>(this);
    hlog_verbose_arrow(level, format, name, mode, t->size, "");
  }
}

static int direntFilter(const struct dirent* a) {
  if (a->d_name[0] != '.') return 1;
  if (a->d_name[1] == '\0') return 0;
  if (a->d_name[1] != '.') return 1;
  if (a->d_name[2] == '\0') return 0;
  return 1;
}

static int direntCompare(const dirent** a, const dirent** b) {
  return strcmp((*a)->d_name, (*b)->d_name);
}

int FsNodeDir::read(const char* path, dev_t dev) {
  char full_path[PATH_MAX];
  strcpy(full_path, path);
  size_t full_path_len = strlen(full_path);
  // Get entries
  struct dirent** direntList;
  int size = scandir(full_path, &direntList, direntFilter, direntCompare);
  if (size < 0) {
    return -1;
  }
  bool failed = false;
  while (size--) {
    if (! failed) {
      // Stat
      const char* basename = direntList[size]->d_name;
      SharedPath shared(full_path, full_path_len, basename);
      struct stat64 metadata;
      // Any error is fatal here
      if (lstat64(full_path, &metadata) < 0) {
        failed = true;
      } else {
        // Add to list
        char type = '*';
        if (S_ISDIR(metadata.st_mode)) {
          type = 'd';
        } else
        if (S_ISREG(metadata.st_mode)) {
          type = 'f';
        } else
        if (S_ISLNK(metadata.st_mode)) {
          type = 'l';
        }
        FsNode* node = this->createChild(basename, type);
        // Populate
        node->mode = metadata.st_mode & 0177777;
        if ((dev != 0) && (dev != metadata.st_dev)) {
          node->mode |= dev_changed_bit;
        }
        node->uid = metadata.st_uid;
        node->gid = metadata.st_gid;
        if (type != 'd') {
          FsNodeNotDir* not_dir = static_cast<FsNodeNotDir*>(node);
          not_dir->size = metadata.st_size;
          not_dir->mtime = metadata.st_mtime;
          if (type == 'l') {
            FsNodeLink* link = static_cast<FsNodeLink*>(node);
            link->string = static_cast<char*>(
              malloc(static_cast<int>(metadata.st_size) + 1));
            ssize_t count = readlink(full_path, link->string,
              static_cast<int>(metadata.st_size));
            if (count >= 0) {
              metadata.st_size = count;
            } else {
              metadata.st_size = 0;
            }
            link->string[metadata.st_size] = '\0';
          }
        }
      }
    }
    free(direntList[size]);
  }
  free(direntList);
  return failed ? -1 : 0;
}

void FsNodeDir::clear() {
  FsNode* child;
  while ((child = children_head) != NULL) {
    children_head = child->sibling;
    delete child;
  }
}
