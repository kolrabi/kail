#include "iltest.h"

static void dumpMeta(ILuint image) {
  ILuint metaCount = (ILuint)ilGetIntegerImage(image, IL_IMAGE_METADATA_COUNT);
  ILenum type;
  ILuint count, size, j;
  void *data;
  ILuint i;
  ILint tmp[512];
  ILfloat et = 1.0f;
  memset(tmp, 0, sizeof(tmp));

  for (i=0; i<metaCount; i++) {
    ILenum IFD, ID;
    if (!ilEnumMetadata(i, &IFD, &ID)) break;

    ilGetMetadata(IFD, ID, &type, &count, &size, &data);

    fprintf(stderr, "    %04x %04x %4u %4u (%2u): ", IFD, ID, count, size, type);

    switch (type) {
      case IL_EXIF_TYPE_BYTE:   fprintf(stderr, "%02x\n", *(ILubyte*)data); break;
      case IL_EXIF_TYPE_ASCII:  fprintf(stderr, "%s\n",   (const char*)data); break;
      case IL_EXIF_TYPE_WORD:   for (j=0; j<count; j++) fprintf(stderr, "%04x ", ((ILushort*)data)[j]); fprintf(stderr, "\n"); break;
      case IL_EXIF_TYPE_DWORD:  fprintf(stderr, "%08x\n", *(ILuint*)data); break;
      case IL_EXIF_TYPE_RATIONAL: fprintf(stderr, "%u/%u\n", ((ILuint*)data)[0], ((ILuint*)data)[1]); break;
      case IL_EXIF_TYPE_SBYTE:   fprintf(stderr, "%d\n", *(ILbyte*)data); break;
      case IL_EXIF_TYPE_SWORD:   fprintf(stderr, "%d\n", *(ILshort*)data); break;
      case IL_EXIF_TYPE_SDWORD:  fprintf(stderr, "%d\n", *(ILint*)data); break;
      case IL_EXIF_TYPE_SRATIONAL: fprintf(stderr, "%d/%d\n", ((ILint*)data)[0], ((ILint*)data)[1]); break;
      default:
        fprintf(stderr, "?? \n");
    }
  }

  count = ilGetIntegerv(IL_META_FSTOP, tmp);
  fprintf(stderr, "FSTOP: %d: %u/%u\n", count, (ILuint)tmp[0], (ILuint)tmp[1]);

  count = ilGetIntegerv(IL_META_EXPOSURE_TIME, tmp);
  et = ilGetFloat(IL_META_EXPOSURE_TIME);
  fprintf(stderr, "Exp:   %d: %u/%u: %f = 1/%f\n", count, (ILuint)tmp[0], (ILuint)tmp[1], et, 1.0/et);

  ilSetFloat(IL_META_EXPOSURE_TIME, 1.0f/20.0f);

  count = ilGetIntegerv(IL_META_EXPOSURE_TIME, tmp);
  et = ilGetFloat(IL_META_EXPOSURE_TIME);
  fprintf(stderr, "Exp:   %d: %u/%u: %f = 1/%f\n", count, (ILuint)tmp[0], (ILuint)tmp[1], et, 1.0/et);


  fprintf(stderr, "Make:  " IL_SFMT "\n", ilGetString(IL_META_MAKE));
  fprintf(stderr, "Model: " IL_SFMT "\n", ilGetString(IL_META_MODEL));
  fprintf(stderr, "SW:    " IL_SFMT "\n", ilGetString(IL_META_SOFTWARE));
  ilSetString(IL_META_SOFTWARE, ilGetString(IL_VERSION_NUM));
}

int main(int argc, char **argv) {
  ILuint    image, reference;
  ILfloat   similarity;
  ILuint    frameCount, width, height, depth;

  // syntax: ILtestFormatLoad <reference> <image> <width> <height> <depth> <frames>
  if (argc < 7) {
    return -1;
  }

  width       = (ILuint)atoi(argv[3]);
  height      = (ILuint)atoi(argv[4]);
  depth       = (ILuint)atoi(argv[5]);
  frameCount  = (ILuint)atoi(argv[6]);

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
  dumpMeta(image);

  // check parameters
  ilBindImage(image);
  CHECK_EQ((ILuint)ilGetInteger(IL_NUM_IMAGES), frameCount);
  CHECK(testSaveSubImage("test-load.jpg", image, frameCount));

  ilBindImage(image);
  ilActiveImage(frameCount);

  CHECK_EQ((ILuint)ilGetInteger(IL_IMAGE_WIDTH),  width);
  CHECK_EQ((ILuint)ilGetInteger(IL_IMAGE_HEIGHT), height);
  CHECK_EQ((ILuint)ilGetInteger(IL_IMAGE_DEPTH),  depth);

  // check similarity
  iluScale((ILuint)ilGetIntegerImage(reference, IL_IMAGE_WIDTH), (ILuint)ilGetIntegerImage(reference, IL_IMAGE_HEIGHT), 1);
  similarity = iluSimilarity(reference);
  fprintf(stderr, "Similarity: %f\n", similarity);
   CHECK_GREATER(similarity, 0.90);

  // cleanup
  ilDeleteImages(1, &image);  
  ilDeleteImages(1, &reference);  
  ilShutDown();

  return 0;
}
