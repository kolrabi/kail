#include "iltest.h"

static ILint Num = 0;
static ILint FilterNum = -1;
static ILint FirstNum = -1;

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

static const char *TypeName(ILenum Type) {
  switch(Type) {
    case IL_BYTE:             return "SB";
    case IL_UNSIGNED_BYTE:    return "UB";
    case IL_SHORT:            return "SS";
    case IL_UNSIGNED_SHORT:   return "US";
    case IL_INT:              return "SI";
    case IL_UNSIGNED_INT:     return "UI";
    case IL_DOUBLE:           return "FD";
    case IL_FLOAT:            return "FF";
    case IL_HALF:             return "FH";
  }
  return "XX";
}

static ILenum Formats[] = {
  IL_COLOR_INDEX,
  IL_ALPHA,
  IL_RGB,
  IL_RGBA,
  IL_BGR,
  IL_BGRA,
  IL_LUMINANCE,
  IL_LUMINANCE_ALPHA,
  0
};

static const char *FormatName(ILenum Format) {
  switch(Format) {
    case IL_COLOR_INDEX:      return "I___";
    case IL_ALPHA:            return "A___";
    case IL_RGB:              return "RGB_";
    case IL_RGBA:             return "RGBA";
    case IL_BGR:              return "BGR_";
    case IL_BGRA:             return "BGRA";
    case IL_LUMINANCE:        return "L___";
    case IL_LUMINANCE_ALPHA:  return "LA__";
  }
  return "XXXX";
}

static void TestFrom(ILenum fromFormat, ILenum fromType, ILuint reference) {
  ILuint  image     = 0;
  ILint   origW, origH;
  ILenum *type = Types, *format = Formats;
  ILfloat similarity;

  if (fromFormat == IL_COLOR_INDEX && fromType != IL_UNSIGNED_BYTE)
    return;

  ilBindImage(reference);
  CHECK(ilConvertImage(fromFormat, fromType));
  origW = ilGetInteger(IL_IMAGE_WIDTH);
  origH = ilGetInteger(IL_IMAGE_HEIGHT);

  while(*type) {
    if ((*type != fromType || *format != fromFormat) && 
        (FilterNum == -1 || FilterNum == Num) && 
        (FirstNum  == -1 || FirstNum  <= Num) &&
        (*format != IL_COLOR_INDEX || *type == IL_UNSIGNED_BYTE) ) {

      fprintf(stderr, "%s/%s -> ", FormatName(fromFormat), TypeName(fromType));
      fprintf(stderr, "%s/%s: ",   FormatName(*format),    TypeName(*type));

      // duplicate
      ilBindImage(reference);
      image = ilCloneCurImage();
      CHECK(image != 0);
      
      // convert
      ilBindImage(image);
      CHECK(ilConvertImage(*format, *type));

      CHECK_EQ(ilGetInteger(IL_IMAGE_WIDTH), origW);
      CHECK_EQ(ilGetInteger(IL_IMAGE_HEIGHT), origH);

      // compare two images
      ilBindImage(image);
      similarity = iluSimilarity(reference);
      fprintf(stderr, "%f\n", similarity);

      char tmp[256];
      snprintf(tmp, sizeof(tmp), "test/%04d.%s_%s.%s_%s.%03.0f.png", 
        Num,
        FormatName(fromFormat), TypeName(fromType), 
        FormatName(*format),    TypeName(*type),
        similarity*100);

      ILfloat needed = 0.99f;

      // alpha textures from the test image when converted to RGBA look nothing
      // like the original rgba test image, so we have to be a little more
      // generous here
      if (*format == IL_ALPHA)
        needed = 0.75f;

      if (similarity < needed || FilterNum != -1) {
        CHECK(testSaveImage(tmp, image));
      }
      CHECK_GREATER(similarity, needed);

      // cleanup
      ilDeleteImages(1, &image);
    }
    Num++;
    format ++;
    if (!*format) {
      type ++;
      format = Formats;
    }
  }
}

int main(int argc, char **argv) {
  ILuint  image     = 0;
  ILuint  reference = 0;
  ILenum *type = Types, *format = Formats;

  // syntax: ILtestConvert <reference> [TestNum]
  if (argc < 2) {
    return -1;
  }

  if (argc > 2) {
    FilterNum = atoi(argv[2]);
    if (FilterNum < 0) {
      FirstNum = -FilterNum;
      FilterNum = -1;
    }
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

  while(*type) {
    ilBindImage(reference);
    image = ilCloneCurImage();
    TestFrom(*format, *type, image);
    ilDeleteImages(1, &image);
    format ++;
    if (!*format) {
      type ++;
      format = Formats;
    }
  }

  // cleanup
  ilDeleteImages(1, &reference);  
  ilShutDown();

  return 0;
}
