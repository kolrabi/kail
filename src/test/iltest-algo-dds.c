#include "iltest.h"

int main(int argc, char **argv) {
  ILuint  image     = 0;
  ILuint  reference = 0;

  ILboolean useSquish;
  ILenum  format;

  // syntax: ILtestAlgoSquish <reference> <usesquish> <format> <dest>
  if (argc < 5) {
    return -1;
  }

  useSquish = !!atol(argv[2]);
  format = (ILenum)strtol(argv[3], NULL, 16);

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
  
  // save using squish
  ilBindImage(image);
  if (useSquish) 
    ilEnable(IL_SQUISH_COMPRESS);

  ilBindImage(image);
  CHECK(ilCopyImage(reference));

  ilSetInteger(IL_DXTC_FORMAT, (ILint)format);
  CHECK(testSaveImage(argv[4], image));
  CHECK(testLoadImage(argv[4], image));

  // compare two images
  ilBindImage(image);
  CHECK_GREATER(iluSimilarity(reference), 0.98f);

  // cleanup
  ilDeleteImages(1, &image);  
  ilDeleteImages(1, &reference);  
  ilShutDown();

  return 0;
}
