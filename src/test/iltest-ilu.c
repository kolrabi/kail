#include "iltest.h"

static ILuint image;

static void load(const char *file_) {
  ILchar file[1024];
  charToILchar(file_, file, 1024);

  image = iluLoadImage(file);  
  CHECK(image != 0);

  fprintf(stderr, "image: %u\n", image);
  
  ilBindImage(image);
}

static void save(const char *file_) {
  ILchar file[1024];
  charToILchar(file_, file, 1024);

  ilBindImage(image);
  ilSetInteger(IL_FILE_MODE, IL_TRUE);
  ilSaveImage(file);
}

int main(int argc, char **argv) {
  // syntax: ILtestILU <in> <out> <filter>
  if (argc < 3) {
    return -1;
  }

  ilInit();
  iluInit();

  load(argv[1]);

  CHECK(image != 0);
  iluImageParameter(ILU_FILTER, ILU_BILINEAR);

  if (!strcmp(argv[3], "alienify")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluAlienify();
  } else if (!strcmp(argv[3], "bluravg")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluBlurAvg(5);
  } else if (!strcmp(argv[3], "blurgaussian")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluBlurGaussian(5);
  } else if (!strcmp(argv[3], "contrast")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluContrast(0.5f);
  } else if (!strcmp(argv[3], "crop")) {
    iluCrop(200, 150, 0, 300, 200, 1);
  } else if (!strcmp(argv[3], "edge_e")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluEdgeDetectE();
  } else if (!strcmp(argv[3], "edge_p")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluEdgeDetectP();
  } else if (!strcmp(argv[3], "edge_s")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluEdgeDetectS();
  } else if (!strcmp(argv[3], "canvas")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluEnlargeCanvas(320, 220, 1);
  } else if (!strcmp(argv[3], "equalize")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluEqualize();
  } else if (!strcmp(argv[3], "flip")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluFlipImage();
  } else if (!strcmp(argv[3], "gamma")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluGammaCorrect(2.2f);
  } else if (!strcmp(argv[3], "mirror")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluMirror();
  } else if (!strcmp(argv[3], "negative")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluNegative();
  } else if (!strcmp(argv[3], "noisify")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluNoisify(5);
  } else if (!strcmp(argv[3], "normalize")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluNormalize();
  } else if (!strcmp(argv[3], "pixelize")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluPixelize(8);
  } else if (!strcmp(argv[3], "replace")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluReplaceColour(255,0,0,0.5f);
  } else if (!strcmp(argv[3], "rotate")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluRotate(30);
  } else if (!strcmp(argv[3], "saturate1")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluSaturate1f(0.5f);
  } else if (!strcmp(argv[3], "saturate4")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluSaturate4f(1,0,0,0.5f);
  } else if (!strcmp(argv[3], "scale_down_nearest")) {
    iluImageParameter(ILU_FILTER, ILU_NEAREST);
    iluEnlargeImage(0.5f,0.5f,1.0f);
  } else if (!strcmp(argv[3], "scale_down_linear")) {
    iluImageParameter(ILU_FILTER, ILU_LINEAR);
    iluEnlargeImage(0.5f,0.5f,1.0f);
  } else if (!strcmp(argv[3], "scale_down_bilinear")) {
    iluImageParameter(ILU_FILTER, ILU_BILINEAR);
    iluEnlargeImage(0.5f,0.5f,1.0f);
  } else if (!strcmp(argv[3], "scale_up_nearest")) {
    iluEnlargeImage(0.25f,0.25f,1.0f);
    iluImageParameter(ILU_FILTER, ILU_NEAREST);
    iluEnlargeImage(2.0f,2.0f,1.0f);
  } else if (!strcmp(argv[3], "scale_up_linear")) {
    iluEnlargeImage(0.25f,0.25f,1.0f);
    iluImageParameter(ILU_FILTER, ILU_LINEAR);
    iluEnlargeImage(2.0f,2.0f,1.0f);
  } else if (!strcmp(argv[3], "scale_up_bilinear")) {
    iluEnlargeImage(0.25f,0.25f,1.0f);
    iluImageParameter(ILU_FILTER, ILU_BILINEAR);
    iluEnlargeImage(2.0f,2.0f,1.0f);
  } else if (!strcmp(argv[3], "scalealpha")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
    iluScaleAlpha(0.5f);
  } else if (!strcmp(argv[3], "scalecolours")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
    iluScaleColours(0.9f, 1.0f, 1.5);
  } else if (!strcmp(argv[3], "sharpen")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluSharpen(1.5, 2);
  } else if (!strcmp(argv[3], "swap")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluSwapColours();
  } else if (!strcmp(argv[3], "wave")) {
    iluEnlargeImage(0.5f,0.5f,1.0f);
    iluWave(30);
  } else {
    CHECK(0);
  }

  CHECK_ERROR();

  save(argv[2]);
 
  ilDeleteImages(1, &image);  
  ilShutDown();

  return 0;
}
