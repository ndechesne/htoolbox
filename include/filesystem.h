/*
     Copyright (C) 2011  Hervé Fache

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

#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#include <sys/stat.h>

/*
 *  Note that all these structures have no virtual methods which make the code
 * less clean but saves a pointer for each object, and we have loads!
 */

namespace htoolbox {

  struct FsNodeDir;
  //! \brief Structure to hold a generic file metadata
  /*!
    Lists contents, using the given parameters as filters.
    \param name         node name
    \param mode         node mode: permissions and type
    \param uid          user id for the node
    \param gid          group id for the node
    \param sibling      the next node in the directory
  */
  struct FsNode {
    char*           name;
    mode_t          mode;
    uid_t           uid;
    gid_t           gid;
    FsNode*       sibling;
    ~FsNode();
    //! \brief Create root node
    /*!
      \param name         the name/path of the root node, if relevant
      \return the node on success, NULL on failure
    */
    static FsNodeDir* createRoot(const char* path = NULL);
    //! \brief Create node from filesystem path
    /*!
      \param path         the path on the filesystem
      \param dev          the device to check this file's against
      \return the node on success, NULL on failure
    */
    static FsNode* createNode(const char* path, dev_t dev = 0);
    //! \brief Add child to directory
    /*!
      \param node         the node
      \return the node on success, NULL on failure
    */
    FsNode* addChild(FsNode* node);
    //! \brief Create child using name and type
    /*!
      \param name         the name to give to the child
      \param type         the type of child: d=dir, f=reg+hash, l=link, *=other
      \return the node on success, NULL on failure
    */
    FsNode* createChild(const char* name, char type);
    //! \brief Get node from path
    /*!
      \param path         file system path
      \return the node on success, NULL on failure
    */
    FsNode* getNode(const char* path, bool create_missing = false);
    static const mode_t dev_changed_bit = 0200000;
    static const mode_t read_issue_bit = 0400000;
    static const mode_t bits_mask = dev_changed_bit | read_issue_bit;
    bool deviceChanged() const { return (mode & dev_changed_bit) != 0; }
    bool readFailed() const { return (mode & read_issue_bit) != 0; }
    void show(int level = 0) const;
  protected:
    FsNode(const char* name, mode_t mode = 0);
  };

  //! \brief Structure to hold a directory metadata and the head of its list
  /*!
    Lists contents, using the given parameters as filters.
    \param children_head head of contained files list
  */
  struct FsNodeDir : public FsNode {
    FsNode*       children_head;
    FsNodeDir(const char* name): FsNode(name, S_IFDIR), children_head(NULL) {}
    int read(const char* path, dev_t dev = 0);
    void clear();
  };

  //! \brief Structure to hold a file metadata that's not a directory
  /*!
    Lists contents, using the given parameters as filters.
    \param off_t        file size
    \param time_t       file modification time
  */
  struct FsNodeNotDir : public FsNode {
    off_t           size;
    time_t          mtime;
    FsNodeNotDir(const char* name, mode_t mode = 0):
      FsNode(name, mode), size(0), mtime(0) {}
  };

  //! \brief Structure to hold a regular file metadata and contents hash
  /*!
    Lists contents, using the given parameters as filters.
    \param hash         file contents hash
  */
  struct FsNodeFile : public FsNodeNotDir {
    char            hash[35];
    FsNodeFile(const char* name): FsNodeNotDir(name, S_IFREG) {
      hash[0] = '\0';
    }
  };

  //! \brief Structure to hold a symbolic link metadata
  /*!
    Lists contents, using the given parameters as filters.
    \param string       linked string
  */
  struct FsNodeLink : public FsNodeNotDir {
    char*           string;
    FsNodeLink(const char* name): FsNodeNotDir(name, S_IFLNK), string(NULL) {}
  };

}

#endif  // _FILESYSTEM_H
