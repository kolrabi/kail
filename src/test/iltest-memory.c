#include "iltest.h"

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

TEST(default_alloc) {
  allocated_default = ialloc(1024);
  CHECK(allocated_default != NULL);

  ifree(allocated_default);
  allocated_default = NULL;
}

TEST(custom_alloc) {
  ilSetMemory(&custom_alloc, &custom_free);

  allocated_custom = ialloc(1024);
  CHECK(allocated_custom != NULL);
  CHECK(custom_alloc_called == 1);

  ifree(allocated_custom);
  allocated_custom = NULL;

  CHECK(custom_free_called == 1);
}

TEST(mixed_alloc) {
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
}

TEST(endian) {
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
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "no test specified\n");
    return 1;
  }

  argv++;

  ilInit();
  iluInit();

  RUN_TEST(default_alloc)
  RUN_TEST(custom_alloc)
  RUN_TEST(mixed_alloc)
  RUN_TEST(endian) 
  {
    fprintf(stderr, "unknown test %s\n", *argv);
    exit(1);
  }

  ilShutDown();

  return 0;
}
