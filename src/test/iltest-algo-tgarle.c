#include "iltest.h"

int main(int argc, char **argv) {
  ILuint  image     = 0;
  ILuint  reference = 0;

  // syntax: ILtestAlgoQuant <reference> 
  if (argc < 2) {
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

  // duplicate
  image = ilCloneCurImage();
  CHECK(image != 0);
  
  // save rle encoded
  ilBindImage(image);
  ilEnable(IL_TGA_RLE);

  BENCHMARK( ilDetermineSize(IL_TGA);, 100 );

  CHECK(testSaveImage("test_tgarle.tga", image));
  CHECK(testLoadImage("test_tgarle.tga", image));

  // compare two images
  ilBindImage(image);
  CHECK_GREATER(iluSimilarity(reference), 0.98f);

  // cleanup
  ilDeleteImages(1, &image);  
  ilDeleteImages(1, &reference);  
  ilShutDown();

  return 0;
}
