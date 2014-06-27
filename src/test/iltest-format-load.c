#include <IL/devil_internal_exports.h>
#include "IL/il_endian.h"
#include <IL/ilu.h>

#include <stdlib.h>
#include <string.h>

#define CHECK(x) if (!(x)) { fprintf(stderr, "FAILED in line %d: %s\n", __LINE__, #x); return 1; }
#define CHECK_EQ(x, y) if ((x)!=(y)) { fprintf(stderr, "FAILED in line %d: %s (%d) == %s (%d)\n", __LINE__, #x, x, #y, y); return 1; }

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
/*
static void saveImage(const char *file_, ILuint image) {
#ifdef _UNICODE
  wchar_t file[1024];
  mbstowcs(file, file_, sizeof(file)/sizeof(wchar_t));
  wcsncat(file, L".loaded.png", 1023-wcslen(file));
#else
  char file[1024];
  strncpy(file, file_, 1024);
  strncat(file, ".loaded.png", 1023-strlen(file));
#endif

  fprintf(stderr, "Saving: %s\n", file_);

  ilBindImage(image);
  ilEnable(IL_FILE_MODE);
  ilSaveImage(file);
}
*/
static int load(const char *file_, int w, int h, int d, int f) {
  ILboolean loaded = loadImage(file_, image);
  ILenum error = ilGetError();
  ILfloat sim;

  CHECK_EQ(error, 0);
  CHECK_EQ(loaded, IL_TRUE);

  CHECK_EQ(ilGetInteger(IL_NUM_IMAGES), f);

  ilActiveImage(f);

  CHECK_EQ(ilGetInteger(IL_IMAGE_WIDTH), w);
  CHECK_EQ(ilGetInteger(IL_IMAGE_HEIGHT), h);
  CHECK_EQ(ilGetInteger(IL_IMAGE_DEPTH), d);

  // saveImage(file_, image);

  fprintf(stderr, "Scaling...\n");
  iluScale(ilGetIntegerImage(reference, IL_IMAGE_WIDTH), ilGetIntegerImage(reference, IL_IMAGE_HEIGHT), 1);
  fprintf(stderr, "Getting similarity...\n");
  sim = iluSimilarity(reference);

  fprintf(stderr, "Similarity: %f\n", sim);

  return 0;
}

int main(int argc, char **argv) {
  int result;

  // test file w h d frames
  if (argc < 7) {
    return -1;
  }

  ilInit();

  ilGenImages(1, &image);
  CHECK(image != 0);

  ilGenImages(1, &reference);
  CHECK(reference != 0);

  loadImage(argv[1], reference);

  result = load(argv[2], atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
 
  ilDeleteImages(1, &image);  
  ilDeleteImages(1, &reference);  
  ilShutDown();

  return result;
}