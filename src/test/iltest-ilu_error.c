#include <IL/devil_internal_exports.h>
#include "IL/il_endian.h"

#include <IL/ilu.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) if (!(x)) { fprintf(stderr, "FAILED in line %d: %s\n", __LINE__, #x); return 1; }

static int testError(ILenum error) {
  ILconst_string errorString = iluErrorString(error);
  if (!errorString) return 0;

  fprintf(stderr, "%04x: " IL_SFMT "\n", error, errorString);

  return 1;
}

int main(int argc, char **argv) {
  int result = 0;

  (void)argc;
  (void)argv;

  ilInit();
  iluInit();

  CHECK(testError(IL_NO_ERROR));
  CHECK(testError(IL_INVALID_ENUM));
  CHECK(testError(IL_OUT_OF_MEMORY));
  CHECK(testError(IL_FORMAT_NOT_SUPPORTED));
  CHECK(testError(IL_INTERNAL_ERROR));
  CHECK(testError(IL_INVALID_VALUE));
  CHECK(testError(IL_ILLEGAL_OPERATION));
  CHECK(testError(IL_ILLEGAL_FILE_VALUE));
  CHECK(testError(IL_INVALID_FILE_HEADER));
  CHECK(testError(IL_INVALID_PARAM));
  CHECK(testError(IL_COULD_NOT_OPEN_FILE));
  CHECK(testError(IL_INVALID_EXTENSION));
  CHECK(testError(IL_FILE_ALREADY_EXISTS));
  CHECK(testError(IL_OUT_FORMAT_SAME));
  CHECK(testError(IL_STACK_OVERFLOW));
  CHECK(testError(IL_STACK_UNDERFLOW));
  CHECK(testError(IL_INVALID_CONVERSION));
  CHECK(testError(IL_BAD_DIMENSIONS));
  CHECK(testError(IL_FILE_READ_ERROR));
  CHECK(testError(IL_FILE_WRITE_ERROR));

  CHECK(testError(IL_LIB_GIF_ERROR));
  CHECK(testError(IL_LIB_JPEG_ERROR));
  CHECK(testError(IL_LIB_PNG_ERROR));
  CHECK(testError(IL_LIB_TIFF_ERROR));
  CHECK(testError(IL_LIB_MNG_ERROR));
  CHECK(testError(IL_LIB_JP2_ERROR));
  CHECK(testError(IL_LIB_EXR_ERROR));
  CHECK(testError(IL_UNKNOWN_ERROR));

  ilShutDown();

  return result;
}
