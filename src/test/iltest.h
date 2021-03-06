#ifndef ILTEST_H
#define ILTEST_H

#include <IL/devil_internal_exports.h>
#include "IL/il_endian.h"
#include <IL/ilu.h>
// #include <IL/ilut.h>

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define CHECK_ERROR() { \
  ILenum err = ilGetError(); \
  if (err) { \
    fprintf(stderr, "FAILED in line %d: " IL_SFMT, __LINE__, iluErrorString(err)); \
    exit(1); \
  } \
}

#define CHECK(x)              if (!(x)) { \
  fprintf(stderr, "FAILED in line %d: %s\n", __LINE__, #x); \
  CHECK_ERROR(); \
  exit(1);  \
}

#define CHECK_EQ(x, y)        if ((x)!=(y)) { \
  fprintf(stderr, "FAILED in line %d: %s (%f) == %s (%f)\n", __LINE__, #x, (float)(x), #y, (float)(y)); \
  CHECK_ERROR(); \
  exit(1); \
}

#define CHECK_GREATER(x, y)   if ((x)<(y)) { \
  fprintf(stderr, "FAILED in line %d: %s (%f) > %s (%f)\n", __LINE__, #x, (float)x, #y, (float)y); \
  CHECK_ERROR(); \
  exit(1);  \
}

#define BENCHMARK(x, n) { \
  struct timeval _tv1, _tv2; \
  gettimeofday(&_tv1, NULL); \
  for (int i=0; i<(n); i++) { \
    x \
  } \
  gettimeofday(&_tv2, NULL);\
  double _t1 = _tv1.tv_sec + 0.000001 * _tv1.tv_usec; \
  double _t2 = _tv2.tv_sec + 0.000001 * _tv2.tv_usec; \
  fprintf(stderr, "TIME: %lfs/op: %s\n", (_t2-_t1)/(double)(n), #x); \
}

#define RUN_TEST(n) if (!strcmp(*argv, #n)) test_##n(); else
#define TEST(n) static void test_##n()

static inline void charToILchar(const char *in, ILchar *out, size_t len) {
#ifdef _UNICODE
  mbstowcs(out, in, len);
#else
  strncpy(out, in, len);
#endif
}

static inline ILboolean testLoadImage(const char *file_, ILuint image) {
  ILchar    file[1024];

  charToILchar(file_, file, 1024);

  fprintf(stderr, "loading %s\n", file_);

  ilBindImage(image);
  return ilLoad(IL_TYPE_UNKNOWN, file);
}

static inline ILboolean testSaveImage(const char *file_, ILuint image) {
  ILchar    file[1024];
  ILboolean bRet;

  charToILchar(file_, file, 1024);

  fprintf(stderr, "saving  %s\n", file_);

  ilBindImage(image);
  bRet = ilSaveImage(file);

  if (bRet) {
    fprintf(stderr, "Saved size should be: %u\n", ilDetermineSize(ilDetermineType(file)));
  }
  return bRet;
}

static inline ILboolean testSaveSubImage(const char *file_, ILuint image, ILint frame) {
  ILchar    file[1024];
  ILboolean bRet;

  charToILchar(file_, file, 1024);

  fprintf(stderr, "saving  %s\n", file_);

  ilBindImage(image);
  ilActiveImage(frame);
  bRet = ilSaveImage(file);

  if (bRet) {
    fprintf(stderr, "Saved size should be: %u\n", ilDetermineSize(ilDetermineType(file)));
  }
  return bRet;
}

static inline ILboolean testSaveImageFormat(const char *file_, ILuint image, ILenum format) {
  ILchar    file[1024];
  ILboolean bRet;

  charToILchar(file_, file, 1024);

  fprintf(stderr, "saving  %s\n", file_);

  ilBindImage(image);
  bRet = ilSave(format, file);
  return bRet;
}
#endif
