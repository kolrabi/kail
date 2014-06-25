#include <IL/devil_internal_exports.h>
#include "IL/il_endian.h"
#include <IL/ilu.h>

#include <stdlib.h>
#include <string.h>

#define CHECK(x) if (!(x)) { fprintf(stderr, "FAILED in line %d: %s\n", __LINE__, #x); return 1; }
#define CHECK_EQ(x, y) if ((x)!=(y)) { fprintf(stderr, "FAILED in line %d: %s (%d) == %s (%d)\n", __LINE__, #x, x, #y, y); return 1; }

static ILuint image;

static int load(const char *file_, int w, int h, int d, int f) {
  ILboolean loaded;
  ILenum error;
#ifdef _UNICODE
  wchar_t file[1024];
  mbstowcs(file, file_, sizeof(file)/sizeof(wchar_t));
#else
  char file[1024];
  strncpy(file, file_, 1024);
#endif

  ilBindImage(image);
  loaded = ilLoad(IL_TYPE_UNKNOWN, file);
  error = ilGetError();
  CHECK_EQ(error, 0);
  CHECK_EQ(loaded, IL_TRUE);
  CHECK_EQ(ilGetInteger(IL_IMAGE_WIDTH), w);
  CHECK_EQ(ilGetInteger(IL_IMAGE_HEIGHT), h);
  CHECK_EQ(ilGetInteger(IL_IMAGE_DEPTH), d);
  CHECK_EQ(ilGetInteger(IL_NUM_IMAGES), f);

#ifdef _UNICODE
  wcsncat(file, L".loaded.png", 1023-wcslen(file));
#else
  strncat(file, ".loaded.png", 1023-strlen(file));
#endif

  ilSetInteger(IL_FILE_MODE, IL_TRUE);
  iluNormalize();
  ilSaveImage(file);

  return 0;
}

int main(int argc, char **argv) {
  int result;

  // test file w h d frames
  if (argc < 6) {
    return -1;
  }

  ilInit();
  ilGenImages(1, &image);

  CHECK(image != 0);

  result = load(argv[1], atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
 
  ilDeleteImages(1, &image);  
  ilShutDown();

  return result;
}