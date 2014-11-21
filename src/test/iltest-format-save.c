#include "iltest.h"

int main(int argc, char **argv) {
  ILuint  image     = 0;
  ILuint  reference = 0;
  ILuint  format    = 0;

  // syntax: ILtestFormatSave <reference> <target> [format]
  if (argc < 3) {
    return -1;
  }

  if (argc > 3) {
    format = (ILenum)strtol(argv[3], NULL, 16);
  }
    

  ilInit();
  iluInit();

  ilEnable(IL_ORIGIN_SET);    // flip image on load if necessary
  ilEnable(IL_FILE_MODE);     // overwrite files
  ilSetInteger(IL_QUANTIZATION_MODE, IL_NEU_QUANT);

  // load reference image
  ilGenImages(1, &reference);
  CHECK(reference != 0);
  CHECK(testLoadImage(argv[1], reference));

  // save target image
  if (format) {
    CHECK(testSaveImageFormat(argv[2], reference, format));
  } else {
    CHECK(testSaveImage(argv[2], reference));
  }

  // only compare images, not palettes
  if (format < IL_JASC_PAL || format > IL_PLT_PAL) {
    // load saved image
    ilGenImages(1, &image);
    CHECK(image != 0);
    CHECK(testLoadImage(argv[2], image));
    CHECK(testSaveImage("test-save.jpg", image));

    // compare two images
    ilBindImage(image);
    CHECK_GREATER(iluSimilarity(reference), 0.95f);
    ilDeleteImages(1, &image);  
  }

  // cleanup
  ilDeleteImages(1, &reference);  
  ilShutDown();

  return 0;
}
