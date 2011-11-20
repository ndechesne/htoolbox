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

#ifndef _HASH_TREE_H
#define _HASH_TREE_H

namespace htoolbox {

// The class T must implement the following operator for this to work:
//   operator const char*() const { return hash; }

template<class T>
class HashTree {
  HashTree<T>*    _parent;
  union {
    HashTree<T>*  nodes[16];
    T*            hobjs[16];
  } child;
  int16_t _mask;      // Bit mask: 1 means child is hobj
  int16_t _level;
  int8_t size() const;
  static int getIndex(char c) {
    return c >= 'a' ? c - 'a' + 10: c >= 'A' ? c - 'A' + 10 : c != '-' ? c - '0' : 0;
  }
public:
  HashTree<T>(HashTree<T>* parent = NULL, int16_t level = 0):
      _parent(parent), _mask(0), _level(level) {
    for (int i = 0; i < 16; ++i) {
      child.nodes[i] = NULL;
    }
  }
  ~HashTree<T>() {
    for (int i = 0; i < 16; ++i) {
      if (child.nodes[i] != NULL) {
        if ((_mask & (1 << i)) == 0) {
          delete child.nodes[i];
        } else {
          delete child.hobjs[i];
        }
      }
    }
  }
  // Return existing leaf
  T* add(T* hobj);
  T* find(const char* hash, HashTree<T>** node = NULL);
  T* find(const T* hobj, HashTree<T>** node = NULL) {
    return this->find(static_cast<const char*>(*hobj), node);
  }
  T* remove(const char* hash);
  T* remove(const T* hobj) {
    return this->remove(static_cast<const char*>(*hobj));
  }
  T* next(const char* hash, HashTree<T>** hint = NULL);
  T* next(const T* hobj, HashTree<T>** hint = NULL) {
    return this->next(static_cast<const char*>(*hobj), hint);
  }
  void show(int level = 0) const;
};

template<class T>
T* HashTree<T>::add(T* hobj) {
  HashTree<T>* node;
  T* found = find(*hobj, &node);
  int i = getIndex((*hobj)[node->_level]);
  hlog_regression("adding %s, i=%d, level=%d, found=%p node=%p this=%p",
    static_cast<const char*>(*hobj), i, node->_level, found, node, this);
  // Not found, go on
  if (found == NULL) {
    // No child: easy, add it
    if (node->child.nodes[i] == NULL) {
      node->child.hobjs[i] = hobj;
      node->_mask |= static_cast<int16_t>(1 << i);
      hlog_regression("added, mask=%x", node->_mask);
    } else {
      HashTree<T>* new_child =
        new HashTree<T>(node, static_cast<int16_t>(node->_level + 1));
      new_child->add(node->child.hobjs[i]);
      new_child->add(hobj);
      node->child.nodes[i] = new_child;
      node->_mask &= static_cast<int16_t>(~(1 << i));
    }
  }
  return found;
}

template<class T>
T* HashTree<T>::find(const char* hash, HashTree<T>** node_p) {
  if (node_p != NULL) {
    *node_p = this;
  }
  int i = getIndex(hash[_level]);
  if (child.hobjs[i] == NULL) {
    return NULL;
  } else
  if ((_mask & (1 << i)) == 0) {
    return child.nodes[i]->find(hash, node_p);
  } else
  if (strcasecmp(static_cast<const char*>(*child.hobjs[i]), hash) != 0) {
    return NULL;
  } else
  {
    return child.hobjs[i];
  }
}

template<class T>
T* HashTree<T>::remove(const char* hash) {
  HashTree<T>* node;
  T* obsolete = find(hash, &node);
  if (obsolete == NULL) {
    return NULL;
  }
  int i = getIndex(hash[node->_level]);
  node->child.nodes[i] = NULL;
  while ((node->_parent != NULL) && (node->size() == 0)) {
    node = node->_parent;
    i = getIndex(hash[node->_level]);
    delete node->child.nodes[i];
    node->child.nodes[i] = NULL;
  }
  return obsolete;
}

template<class T>
T* HashTree<T>::next(const char* hash, HashTree<T>** hint) {
  HashTree<T>* node = this;
  if (hash == NULL) {
    hlog_regression("find first");
    do {
      int i;
      for (i = 0; i < 16; ++i) {
        if (node->child.nodes[i] != NULL) {
          if ((node->_mask & (1 << i)) == 0) {
            node = node->child.nodes[i];
            break;
          } else {
            if (hint != NULL) {
              *hint = node;
            }
            return node->child.hobjs[i];
          }
        }
      }
      if (i >= 16) {
        break;;
      }
    } while (true);
    return NULL;
  }
  bool found;
  hlog_regression("find next from %s", hash);
  if (hint == NULL) {
    hlog_regression("hint invalid");
    // Find hint (leaf that contains/would contain the hash)
    found = find(hash, &node) != NULL;
    hlog_regression("found %p, level=%d", node, node != NULL ? node->_level : -1);
  } else {
    found = true;
  }
  // Find next
  int i = getIndex(hash[node->_level]) + (found ? 1 : 0);
  do {
    for (; i < 16; ++i) {
      hlog_regression("i=%d, level=%d, hash=%s", i, node->_level, hash);
      if (node->child.nodes[i] != NULL) {
        if ((node->_mask & (1 << i)) == 0) {
          node = node->child.nodes[i];
          i = -1; // ++ follows
          hlog_regression("go down, level=%d", node->_level);
          continue;
        } else {
          hlog_regression("found %s",
            static_cast<const char*>(*node->child.hobjs[i]));
          if (hint != NULL) {
            *hint = node;
          }
          return node->child.hobjs[i];
        }
      }
    }
    node = node->_parent;
    if (node != NULL) {
      i = getIndex(hash[node->_level]) + 1;
    }
    hlog_regression("sgo up, level=%d", node != NULL ? node->_level : -1);
  } while (node != NULL);
  return NULL;
}

template<class T>
int8_t HashTree<T>::size() const {
  int8_t count = 0;
  for (int i = 0; i < 16; ++i) {
    if (child.nodes[i] != NULL) {
      ++count;
    }
  }
  return count;
}

template<class T>
void HashTree<T>::show(int level) const {
  ++level;
  for (int i = 0; i < 16; ++i) {
    if (child.nodes[i] != NULL) {
      if ((_mask & (1 << i)) == 0) {
        hlog_verbose_arrow(level, "[%x] (node)", i);
        child.nodes[i]->show(level);
      } else {
        hlog_verbose_arrow(level, "[%x] %s", i,
          static_cast<const char*>(*child.hobjs[i]));
      }
    }
  }
}

}

#endif // _HASH_TREE_H
