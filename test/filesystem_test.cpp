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

#include <stdlib.h>
#include <limits.h>

#include <report.h>
#include <shared_path.h>
#include "filesystem.h"

using namespace htoolbox;

void get_subdirs(FsNodeDir* parent, char* path) {
  if (parent->read(path) < 0) {
    hlog_error("%m reading dir '%s'", path);
    return;
  }
  FsNode* node = parent->children_head;
  while (node != NULL) {
    if (S_ISDIR(node->mode)) {
      SharedPath s(path, strlen(path), node->name);
      get_subdirs(static_cast<FsNodeDir*>(node), path);
    }
    node = node->sibling;
  }
}

int main(void) {
  report.setLevel(debug);

  char current_path[PATH_MAX];
  strcpy(current_path, getenv("PWD"));
  FsNodeDir* root;

  // Get test1 dir
  {
    SharedPath path(current_path, strlen(current_path), "test1");
    root = FsNode::createRoot("test1");
    if (root->read(current_path, 1) < 0) {
      hlog_error("%m reading dir '%s'", current_path);
    }
    root->show(1);
    delete root;
  }

  // Get test1 and sub-dirs
  {
    SharedPath path(current_path, strlen(current_path), "test1");
    root = FsNode::createRoot(current_path);
    get_subdirs(root, current_path);
    root->show(1);
    delete root;
  }

  return 0;
}
