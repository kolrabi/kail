#include "iltest.h"

static ILuint image;
static ILchar fileName[1024];
static const char *data;

static void checkRead(ILimage *ilimage) {
  ILint res;
  ILuint pos;

  CHECK(ilimage->io.handle != NULL);

  pos = SIOtell(&ilimage->io);
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
  { 
    ILubyte b = (ILubyte)SIOgetc(&ilimage->io);
    CHECK_EQ(b, 'D'); 
  }

  {
    ILubyte bb[6];
    ILuint r = SIOread(&ilimage->io, bb, 1, 6);
    CHECK_EQ(r, 6);
    CHECK(memcmp(bb, "efghij", 6) == 0);
  
    r = SIOread(&ilimage->io, bb, 1, 6);
    CHECK_EQ(r, 1);
    CHECK_EQ(bb[0], 'k');
  }

  CHECK_EQ( SIOgetc(&ilimage->io), IL_EOF );

  CHECK_EQ( SIOeof(&ilimage->io), IL_TRUE );

  SIOclose(&ilimage->io);
}

TEST(open_read) {
  ILimage *ilimage;
  ilBindImage(image);
  ilimage = iLockCurImage();

  CHECK(ilimage != NULL);
  CHECK(ilimage->io.openReadOnly != NULL);

  SIOopenRO(&ilimage->io, fileName);

  checkRead(ilimage);
}

TEST(open_read_lump) {
  ILimage *ilimage;
  ilBindImage(image);
  ilimage = iLockCurImage();

  CHECK(ilimage != NULL);
  iSetInputLump(ilimage, (void*)data, strlen(data));
  CHECK_EQ(ilimage->io.lumpSize, 11);

  checkRead(ilimage);
}

int main(int argc, char **argv) {
  if (argc != 3) {
    return 1;
  }

  ilInit();
  iluInit();

  ilGenImages(1, &image);
  charToILchar(argv[2], fileName, 1024);
  data = argv[2];

  argv++;

  RUN_TEST(open_read)
  RUN_TEST(open_read_lump)
    fprintf(stderr, "unknown test %s\n", *argv);

  ilDeleteImages(1, &image);
  ilShutDown();

  return 0;
}
