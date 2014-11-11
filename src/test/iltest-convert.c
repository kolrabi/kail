#include "iltest.h"

static ILenum Types[] = {
  IL_BYTE,
  IL_UNSIGNED_BYTE,
  IL_SHORT,
  IL_UNSIGNED_SHORT,
  IL_INT,
  IL_UNSIGNED_INT,
  IL_FLOAT,
  IL_DOUBLE,
  IL_HALF,
  0
};

static ILenum Formats[] = {
  IL_COLOR_INDEX,
//  IL_ALPHA,
  IL_RGB,
  IL_RGBA,
  IL_BGR,
  IL_BGRA,
  IL_LUMINANCE,
  IL_LUMINANCE_ALPHA,
  0
};

int main(int argc, char **argv) {
  ILuint  image     = 0;
  ILuint  reference = 0;

  ILint   origW, origH;
  ILenum *type = Types, *format = Formats;

  // syntax: ILtestConvert <reference> 
  if (argc < 2) {
    return -1;
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

  ilBindImage(reference);
  CHECK(ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE));
  origW = ilGetInteger(IL_IMAGE_WIDTH);
  origH = ilGetInteger(IL_IMAGE_HEIGHT);

  while(*type) {
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "test_%04x_%04x.png", *format, *type);

    // duplicate
    ilBindImage(reference);
    image = ilCloneCurImage();
    CHECK(image != 0);
    
    // save rle encoded
    ilBindImage(image);

    CHECK(ilConvertImage(*format, *type));
    CHECK(testSaveImage(tmp, image));

    CHECK_EQ(ilGetInteger(IL_IMAGE_WIDTH), origW);
    CHECK_EQ(ilGetInteger(IL_IMAGE_HEIGHT), origH);

    // compare two images
    ilBindImage(image);
    /*if (*format == IL_LUMINANCE || *format == IL_LUMINANCE_ALPHA) {
      CHECK_GREATER(iluSimilarity(reference), 0.10f);
    } else {
      CHECK_GREATER(iluSimilarity(reference), 0.98f);
    }*/

    CHECK(ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE));
    snprintf(tmp, sizeof(tmp), "test_%04x_%04x_back.png", *format, *type);
    CHECK(testSaveImage(tmp, image));

    // cleanup
    ilDeleteImages(1, &image);

    format ++;
    if (!*format) {
      type ++;
      format = Formats;
      if (*type != IL_BYTE && *type != IL_UNSIGNED_BYTE)
        format++; // skip color indexed with indices larger than 1 byte
    }
  }

  // cleanup
  ilDeleteImages(1, &reference);  
  ilShutDown();

  return 0;
}
