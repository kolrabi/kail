#include <IL/devil_internal_exports.h>
#include "IL/il_endian.h"
#include "IL/il_internal.h"

#include <stdlib.h>
#include <string.h>

#define TEST(n) if (!strcmp(*argv, #n)) result = test_##n(argv+1); else
#define CHECK(x) if (!(x)) { fprintf(stderr, "FAILED in line %d: %s\n", __LINE__, #x); return 1; }
#define CHECK_EQ(x, y) if ((x)!=(y)) { fprintf(stderr, "FAILED in line %d: %s (%d) == %s (%d)\n", __LINE__, #x, x, #y, y); return 1; }

static ILuint image;

static int test_open_read(char **argv) {
  ilBindImage(image);
  ILimage *ilimage = iLockCurImage();

  printf("!!! %p\n", ilimage);

  CHECK(ilimage != NULL);
  CHECK(ilimage->io.openReadOnly != NULL);

#ifdef _UNICODE
  wchar_t file[1024];
  mbstowcs(file, *argv, sizeof(file)/sizeof(wchar_t));
#else
  const char *file = *argv;
#endif

  SIOopenRO(&ilimage->io, file);

  CHECK(ilimage->io.handle != NULL);

  ILint res;
  ILuint pos = SIOtell(&ilimage->io);
  CHECK(pos == 0);

  // test seek set
  res = SIOseek(&ilimage->io, 0, IL_SEEK_SET);
  CHECK(res == 0);
  pos = SIOtell(&ilimage->io);
  CHECK(pos == 0);

  // test seek end
  res = SIOseek(&ilimage->io, 0, IL_SEEK_END);
  CHECK(res == 0);
  pos = SIOtell(&ilimage->io);
  CHECK_EQ(pos, 11);

  // test seek cur
  res = SIOseek(&ilimage->io, -8, IL_SEEK_CUR);
  CHECK(res == 0);
  pos = SIOtell(&ilimage->io);
  CHECK_EQ(pos, 3);

  // test getc
  ILubyte b = SIOgetc(&ilimage->io);
  CHECK_EQ(b, 'D');

  ILubyte bb[6];
  ILuint r = SIOread(&ilimage->io, bb, 1, 6);
  CHECK_EQ(r, 6);

  CHECK(memcmp(bb, "efghij", 6) == 0);
  
  r = SIOread(&ilimage->io, bb, 1, 6);
  CHECK_EQ(r, 1);
  CHECK_EQ(bb[0], 'k');

  CHECK_EQ( SIOgetc(&ilimage->io), IL_EOF );

  CHECK_EQ( SIOeof(&ilimage->io), IL_TRUE );

  SIOclose(&ilimage->io);

  return 0;
}

int main(int argc, char **argv) {
  int result = 1;

  (void)argc;
  argv++;

  if (!*argv) return -1;

  ilInit();
  ilGenImages(1, &image);

  TEST(open_read)
  // TEST(open_read_lump)
    fprintf(stderr, "unknown test %s\n", *argv);

  ilDeleteImages(1, &image);
  ilShutDown();

  return result;
}