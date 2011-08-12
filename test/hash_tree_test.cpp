#include <string.h>

#include <report.h>

using namespace htoolbox;

#include "hash_tree.h"

using namespace hbackend;

struct TestClass {
  char h[5];
  operator const char*() const { return h; }
};

int main(void) {
  report.setLevel(debug);
  HashTree<TestClass> root;
  HashTree<TestClass>* hint;
  TestClass* meta;

  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("structure:");
  root.show();
  hlog_info("-");


  meta = new TestClass;
  strcpy(meta->h, "abcd");
  hlog_info("adding %s", meta->h);
  root.add(meta);
  if (root.find(meta) != meta) {
    hlog_error("not found self");
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");


  meta = new TestClass;
  strcpy(meta->h, "abce");
  hlog_info("adding %s", meta->h);
  root.add(meta);
  if (root.find(meta) != meta) {
    hlog_error("not found self");
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");


  meta = new TestClass;
  strcpy(meta->h, "0123");
  hlog_info("adding %s", meta->h);
  root.add(meta);
  if (root.find(meta) != meta) {
    hlog_error("not found self");
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");


  meta = new TestClass;
  strcpy(meta->h, "0023");
  hlog_info("adding %s", meta->h);
  root.add(meta);
  if (root.find(meta) != meta) {
    hlog_error("not found self");
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("list without hint:");
  meta = NULL;
  while ((meta = root.next(meta)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");


  meta = new TestClass;
  strcpy(meta->h, "0000");
  hlog_info("adding %s", meta->h);
  root.add(meta);
  if (root.find(meta) != meta) {
    hlog_error("not found self");
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("list without hint:");
  meta = NULL;
  while ((meta = root.next(meta)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");


  meta = new TestClass;
  strcpy(meta->h, "FFFF");
  hlog_info("adding %s", meta->h);
  root.add(meta);
  if (root.find(meta) != meta) {
    hlog_error("not found self");
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("list without hint:");
  meta = NULL;
  while ((meta = root.next(meta)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");


  meta = new TestClass;
  strcpy(meta->h, "ABC0");
  hlog_info("adding %s", meta->h);
  root.add(meta);
  if (root.find(meta) != meta) {
    hlog_error("not found self");
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("list without hint:");
  meta = NULL;
  while ((meta = root.next(meta)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");


  meta = new TestClass;
  strcpy(meta->h, "0001");
  hlog_info("adding %s", meta->h);
  root.add(meta);
  if (root.find(meta) != meta) {
    hlog_error("not found self");
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("list without hint:");
  meta = NULL;
  while ((meta = root.next(meta)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");


  meta = new TestClass;
  strcpy(meta->h, "1234");
  hlog_info("adding %s", meta->h);
  root.add(meta);
  if (root.find(meta) != meta) {
    hlog_error("not found self");
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("list without hint:");
  meta = NULL;
  while ((meta = root.next(meta)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");


  TestClass query;
  strcpy(query.h, "1234");
  hlog_info("finding %s", query.h);
  meta = root.find(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }


  strcpy(query.h, "0123");
  hlog_info("finding %s", query.h);
  meta = root.find(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }


  strcpy(query.h, "0023");
  hlog_info("finding %s", query.h);
  meta = root.find(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }


  strcpy(query.h, "abce");
  hlog_info("finding %s", query.h);
  meta = root.find(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }


  strcpy(query.h, "abcd");
  hlog_info("finding %s", query.h);
  meta = root.find(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }


  strcpy(query.h, "abc0");
  hlog_info("finding %s", query.h);
  meta = root.find(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }


  strcpy(query.h, "0001");
  hlog_info("finding %s", query.h);
  meta = root.find(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }


  strcpy(query.h, "0000");
  hlog_info("finding %s", query.h);
  meta = root.find(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }


  strcpy(query.h, "ffff");
  hlog_info("finding %s", query.h);
  meta = root.find(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }


  strcpy(query.h, "0002");
  hlog_info("finding %s", query.h);
  meta = root.find(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }


  strcpy(query.h, "5555");
  hlog_info("finding %s", query.h);
  meta = root.find(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }


  strcpy(query.h, "FFFE");
  hlog_info("finding %s", query.h);
  meta = root.find(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }


  strcpy(query.h, "1234");
  hlog_info("removing %s", query.h);
  meta = root.remove(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");


  strcpy(query.h, "0123");
  hlog_info("removing %s", query.h);
  meta = root.remove(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");


  strcpy(query.h, "0023");
  hlog_info("removing %s", query.h);
  meta = root.remove(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");


  strcpy(query.h, "abce");
  hlog_info("removing %s", query.h);
  meta = root.remove(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");


  strcpy(query.h, "abcd");
  hlog_info("removing %s", query.h);
  meta = root.remove(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");


  strcpy(query.h, "abc0");
  hlog_info("removing %s", query.h);
  meta = root.remove(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");


  strcpy(query.h, "0001");
  hlog_info("removing %s", query.h);
  meta = root.remove(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");


  strcpy(query.h, "0000");
  hlog_info("removing %s", query.h);
  meta = root.remove(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");


  strcpy(query.h, "ffff");
  hlog_info("removing %s", query.h);
  meta = root.remove(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");


  strcpy(query.h, "0002");
  hlog_info("removing %s", query.h);
  meta = root.remove(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");


  strcpy(query.h, "5555");
  hlog_info("removing %s", query.h);
  meta = root.remove(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");


  strcpy(query.h, "FFFE");
  hlog_info("removing %s", query.h);
  meta = root.remove(&query);
  if (meta == NULL) {
    hlog_info("not found");
  } else {
    hlog_info("found h=%s", meta->h);
  }
  hlog_info("structure:");
  root.show();
  hlog_info("list:");
  meta = NULL;
  hint = &root;
  while ((meta = hint->next(meta, &hint)) != NULL) {
    hlog_info("  meta: %s", meta->h);
  }
  hlog_info("-");

  return 0;
}
