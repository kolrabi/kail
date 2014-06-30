#include "iltest.h"

int main(int argc, char **argv) {
  ILuint  image     = 0;
  ILuint  reference = 0;

  // syntax: ILtestFormatSave <reference> <target>
  if (argc < 3) {
    return -1;
  }

  ilInit();
  iluInit();

  ilEnable(IL_ORIGIN_SET);    // flip image on load if necessary
  ilEnable(IL_FILE_MODE);     // overwrite files

  // load reference image
  ilGenImages(1, &reference);
  CHECK(reference != 0);
  CHECK(testLoadImage(argv[1], reference));

  // save target image
  CHECK(testSaveImage(argv[2], reference));

  // load saved image
  ilGenImages(1, &image);
  CHECK(image != 0);
  CHECK(testLoadImage(argv[2], image));

  // compare two images
  ilBindImage(image);
  CHECK_GREATER(iluSimilarity(reference), 0.98f);

  // cleanup
  ilDeleteImages(1, &image);  
  ilDeleteImages(1, &reference);  
  ilShutDown();

  return 0;
}