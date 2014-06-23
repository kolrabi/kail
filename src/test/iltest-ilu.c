#include <IL/devil_internal_exports.h>
#include "IL/il_endian.h"
#include <IL/ilu.h>

#include <stdlib.h>
#include <string.h>

#define CHECK(x) if (!(x)) { fprintf(stderr, "FAILED in line %d: %s\n", __LINE__, #x); return 1; }
#define CHECK_EQ(x, y) if ((x)!=(y)) { fprintf(stderr, "FAILED in line %d: %s (%d) == %s (%d)\n", __LINE__, #x, x, #y, y); return 1; }

static ILuint image;

static void load(const char *file_) {
#ifdef _UNICODE
  wchar_t file[1024];
  mbstowcs(file, file_, sizeof(file)/sizeof(wchar_t));
#else
  const char *file = file_;
#endif

  image = iluLoadImage(file);  
  ilBindImage(image);
}

static void save(const char *file_) {
#ifdef _UNICODE
  wchar_t file[1024];
  mbstowcs(file, file_, sizeof(file)/sizeof(wchar_t));
#else
  const char *file = file_;
#endif

  ilBindImage(image);
  ilSaveImage(file);
}

int main(int argc, char **argv) {
  ILenum error;

  // test in out filter
  if (argc < 3) {
    return -1;
  }

  ilInit();
  iluInit();

  load(argv[1]);

  CHECK(image != 0);
  iluImageParameter(ILU_FILTER, ILU_BILINEAR);

  if (!strcmp(argv[3], "alienify")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluAlienify();
  } else if (!strcmp(argv[3], "bluravg")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluBlurAvg(5);
  } else if (!strcmp(argv[3], "blurgaussian")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluBlurGaussian(5);
  } else if (!strcmp(argv[3], "contrast")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluContrast(0.5);
  } else if (!strcmp(argv[3], "crop")) {
    iluCrop(200, 150, 0, 300, 200, 1);
  } else if (!strcmp(argv[3], "edge_e")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluEdgeDetectE();
  } else if (!strcmp(argv[3], "edge_p")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluEdgeDetectP();
  } else if (!strcmp(argv[3], "edge_s")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluEdgeDetectS();
  } else if (!strcmp(argv[3], "canvas")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluEnlargeCanvas(320, 220, 1);
  } else if (!strcmp(argv[3], "equalize")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluEqualize();
  } else if (!strcmp(argv[3], "flip")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluFlipImage();
  } else if (!strcmp(argv[3], "gamma")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluGammaCorrect(2.2);
  } else if (!strcmp(argv[3], "mirror")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluMirror();
  } else if (!strcmp(argv[3], "negative")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluNegative();
  } else if (!strcmp(argv[3], "noisify")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluNoisify(5);
  } else if (!strcmp(argv[3], "pixelize")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluPixelize(8);
  } else if (!strcmp(argv[3], "replace")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluReplaceColour(255,0,0,0.5);
  } else if (!strcmp(argv[3], "rotate")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluRotate(30);
  } else if (!strcmp(argv[3], "saturate1")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluSaturate1f(0.5);
  } else if (!strcmp(argv[3], "saturate4")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluSaturate4f(1,0,0,0.5);
  } else if (!strcmp(argv[3], "scale_down_nearest")) {
    iluImageParameter(ILU_FILTER, ILU_NEAREST);
    iluEnlargeImage(0.5,0.5,1.0);
  } else if (!strcmp(argv[3], "scale_down_linear")) {
    iluImageParameter(ILU_FILTER, ILU_LINEAR);
    iluEnlargeImage(0.5,0.5,1.0);
  } else if (!strcmp(argv[3], "scale_down_bilinear")) {
    iluImageParameter(ILU_FILTER, ILU_BILINEAR);
    iluEnlargeImage(0.5,0.5,1.0);
  } else if (!strcmp(argv[3], "scale_up_nearest")) {
    iluEnlargeImage(0.25,0.25,1.0);
    iluImageParameter(ILU_FILTER, ILU_NEAREST);
    iluEnlargeImage(2.0,2.0,1.0);
  } else if (!strcmp(argv[3], "scale_up_linear")) {
    iluEnlargeImage(0.25,0.25,1.0);
    iluImageParameter(ILU_FILTER, ILU_LINEAR);
    iluEnlargeImage(2.0,2.0,1.0);
  } else if (!strcmp(argv[3], "scale_up_bilinear")) {
    iluEnlargeImage(0.25,0.25,1.0);
    iluImageParameter(ILU_FILTER, ILU_BILINEAR);
    iluEnlargeImage(2.0,2.0,1.0);
  } else if (!strcmp(argv[3], "scalealpha")) {
    iluEnlargeImage(0.5,0.5,1.0);
    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
    iluScaleAlpha(0.5);
  } else if (!strcmp(argv[3], "scalecolours")) {
    iluEnlargeImage(0.5,0.5,1.0);
    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
    iluScaleColours(0.9, 1.0, 1.5);
  } else if (!strcmp(argv[3], "sharpen")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluSharpen(1.5, 2);
  } else if (!strcmp(argv[3], "swap")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluSwapColours();
  } else if (!strcmp(argv[3], "wave")) {
    iluEnlargeImage(0.5,0.5,1.0);
    iluWave(30);
  } else {
    CHECK(0);
  }

  error = ilGetError();
  CHECK_EQ(error, 0);

  save(argv[2]);
 
  ilDeleteImages(1, &image);  
  ilShutDown();

  return 0;
}