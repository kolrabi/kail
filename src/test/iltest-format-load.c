#include "iltest.h"

void dumpMeta(ILuint image) {
  ILint metaCount = ilGetIntegerImage(image, IL_IMAGE_METADATA_COUNT);
  ILenum category, id, type;
  ILuint count, size;
  void *data;
  ILint i;

  fprintf(stderr, "Image has %d meta tags\n", metaCount);
  for (i=0; i<metaCount; i++) {
    ilGetMetadata(i, &category, &id, &type, &count, &size, &data);

    fprintf(stderr, "    %04x %04x %4u %4u (%2u): ", category, id, count, size, type);

    switch (type) {
      case IL_EXIF_TYPE_BYTE:   fprintf(stderr, "%02x\n", *(ILubyte*)data); break;
      case IL_EXIF_TYPE_ASCII:  fprintf(stderr, "%s\n",   (const char*)data); break;
      case IL_EXIF_TYPE_WORD:   fprintf(stderr, "%04x\n", *(ILushort*)data); break;
      case IL_EXIF_TYPE_DWORD:  fprintf(stderr, "%08x\n", *(ILuint*)data); break;
      case IL_EXIF_TYPE_RATIONAL: fprintf(stderr, "%u/%u\n", ((ILuint*)data)[0], ((ILuint*)data)[1]); break;
      default:
        fprintf(stderr, "?? \n");
    }
  }
}

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

  ilEnable(IL_ORIGIN_SET);    // flip image on load if necessary
  ilEnable(IL_FILE_MODE);     // overwrite files

  // load reference
  ilGenImages(1, &reference);
  CHECK(reference != 0);
  CHECK(testLoadImage(argv[1], reference));

  // load image
  ilGenImages(1, &image);
  CHECK(image != 0);
  CHECK(testLoadImage(argv[2], image));
  CHECK(testSaveImage("test.png", image));

  dumpMeta(image);

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