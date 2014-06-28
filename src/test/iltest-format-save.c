#include <IL/devil_internal_exports.h>
#include "IL/il_endian.h"
#include <IL/ilu.h>

#include <stdlib.h>
#include <string.h>

#define CHECK(x) if (!(x)) { fprintf(stderr, "FAILED in line %d: %s\n", __LINE__, #x); return 1; }
#define CHECK_EQ(x, y) if ((x)!=(y)) { fprintf(stderr, "FAILED in line %d: %s (%d) == %s (%d)\n", __LINE__, #x, x, #y, y); return 1; }
#define CHECK_GREATER(x, y) if ((x)<(y)) { fprintf(stderr, "FAILED in line %d: %s (%f) > %s (%f)\n", __LINE__, #x, x, #y, y); return 1; }

static ILuint image, reference;

static ILboolean loadImage(const char *file_, ILuint image) {
  ILboolean loaded;

#ifdef _UNICODE
  wchar_t file[1024];
  mbstowcs(file, file_, sizeof(file)/sizeof(wchar_t));
#else
  char file[1024];
  strncpy(file, file_, 1024);
#endif

  ilBindImage(image);
  ilEnable(IL_ORIGIN_SET);
  fprintf(stderr, "Loading: %s\n", file_);
  loaded = ilLoad(IL_TYPE_UNKNOWN, file);
  fprintf(stderr, "Result: %d\n", loaded);
  return loaded;
}

static ILboolean saveImage(const char *file_, ILuint image) {
  ILboolean saved;

#ifdef _UNICODE
  wchar_t file[1024];
  mbstowcs(file, file_, sizeof(file)/sizeof(wchar_t));
#else
  char file[1024];
  strncpy(file, file_, 1024);
#endif

  ilBindImage(image);
  ilEnable(IL_FILE_MODE);
  fprintf(stderr, "Saving: %s\n", file_);
  saved = ilSaveImage(file);
  fprintf(stderr, "Result: %d\n", saved);
  return saved;
}

int main(int argc, char **argv) {
  int result = 0;

  // test file w h d frames
  if (argc < 3) {
    return -1;
  }

  ilInit();

  ilGenImages(1, &image);
  CHECK(image != 0);

  ilGenImages(1, &reference);
  CHECK(reference != 0);

  CHECK(loadImage(argv[1], reference));
  CHECK(saveImage(argv[2], reference));
  CHECK(loadImage(argv[2], image));

  ilBindImage(image);
  CHECK_GREATER(iluSimilarity(reference), 0.98f);

  ilDeleteImages(1, &image);  
  ilDeleteImages(1, &reference);  
  ilShutDown();

  return result;
}