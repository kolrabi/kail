#include "iltest.h"

int main(int argc, char **argv) {
  ILuint    image, reference;
  ILfloat   similarity;
  int       frameCount, width, height, depth;

  // syntax: ILtestFormatLoad <reference> <image> <width> <height> <depth> <frames>
  if (argc < 7) {
    return -1;
  }

  width       = atoi(argv[3]);
  height      = atoi(argv[4]);
  depth       = atoi(argv[5]);
  frameCount  = atoi(argv[6]);

  ilInit();
  iluInit();

  // load reference
  ilGenImages(1, &reference);
  CHECK(reference != 0);
  CHECK(testLoadImage(argv[1], reference));

  // load image
  ilGenImages(1, &image);
  CHECK(image != 0);
  CHECK(testLoadImage(argv[2], image));

  // check parameters
  ilBindImage(image);
  CHECK_EQ(ilGetInteger(IL_NUM_IMAGES), frameCount);
  ilActiveImage(frameCount);

  CHECK_EQ(ilGetInteger(IL_IMAGE_WIDTH),  width);
  CHECK_EQ(ilGetInteger(IL_IMAGE_HEIGHT), height);
  CHECK_EQ(ilGetInteger(IL_IMAGE_DEPTH),  depth);

  // check similarity
  iluScale(ilGetIntegerImage(reference, IL_IMAGE_WIDTH), ilGetIntegerImage(reference, IL_IMAGE_HEIGHT), 1);
  similarity = iluSimilarity(reference);
  fprintf(stderr, "Similarity: %f\n", similarity);
  // CHECK_GREATER(similarity, 0.95);

  // cleanup
  ilDeleteImages(1, &image);  
  ilDeleteImages(1, &reference);  
  ilShutDown();

  return 0;
}