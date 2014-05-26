#include <IL/devil_internal_exports.h>
#include "IL/il_endian.h"

#include <stdlib.h>
#include <string.h>

#define TEST(n) if (!strcmp(*argv, #n)) result = test_##n(); else
#define CHECK(x) if (!(x)) { fprintf(stderr, "FAILED in line %d: %s\n", __LINE__, #x); return 1; }

static void *     allocated_default = NULL;
static void *     allocated_custom  = NULL;

static int        custom_alloc_called = 0;
static int        custom_free_called  = 0;
static int        wrong_pointer_free  = 0;

static void * ILAPIENTRY custom_alloc(ILsizei size) {
  custom_alloc_called ++;
  return malloc(size);
}

static void ILAPIENTRY custom_free(void *p) {
  custom_free_called ++;
  if (p != allocated_custom) wrong_pointer_free ++;
  free(p);
}

static int test_default_alloc() {
  allocated_default = ialloc(1024);
  CHECK(allocated_default != NULL);

  ifree(allocated_default);
  allocated_default = NULL;
  return 0;
}

static int test_custom_alloc() {
  ilSetMemory(&custom_alloc, &custom_free);

  allocated_custom = ialloc(1024);
  CHECK(allocated_custom != NULL);
  CHECK(custom_alloc_called == 1);

  ifree(allocated_custom);
  allocated_custom = NULL;

  CHECK(custom_free_called == 1);
  return 0;
}

static int test_mixed_alloc() {
  allocated_default = ialloc(1024);
  CHECK(allocated_default != NULL);
  CHECK(custom_alloc_called == 0);

  ilSetMemory(&custom_alloc, &custom_free);

  allocated_custom = ialloc(1024);
  CHECK(allocated_custom != NULL);
  CHECK(custom_alloc_called == 1);

  ifree(allocated_default);
  allocated_default = NULL;

  // should have called default free, it was allocated with default alloc
  CHECK(custom_free_called == 0);

  ilSetMemory(NULL, NULL);

  ifree(allocated_custom);
  allocated_custom = NULL;

  // should have called custom free, it was allocated with custom alloc
  CHECK(custom_free_called == 1);

  return 0;
}

int test_endian() {
  ILushort a = 0x1234;
  ILuint   b = 0x12345678;
  ILshort  c = 0x1234;
  ILint    d = 0x12345678;

  iSwapUShort(&a);
  iSwapUInt(&b);
  iSwapShort(&c);
  iSwapInt(&d);

  CHECK(a == 0x3412);
  CHECK(b == 0x78563412);
  CHECK(c == 0x3412);
  CHECK(d == 0x78563412);
  return 0;
}

int main(int argc, char **argv) {
  int result = 1;

  (void)argc;
  argv++;

  if (!*argv) return -1;

  fprintf(stderr, "%s\n", *argv);

  TEST(default_alloc)
  TEST(custom_alloc)
  TEST(mixed_alloc)
  TEST(endian)
    fprintf(stderr, "unknown test %s\n", *argv);

  if (result) {
    fprintf(stdout, "FAIL\n");
  } else {
    fprintf(stdout, "PASS\n");
  }

  return result;
}