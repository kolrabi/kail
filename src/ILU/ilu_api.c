//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Filename: src/ILU/ilu_api.c
//
//-----------------------------------------------------------------------------

/**
 * @file
 * @brief Contains public ILU entry functions.
 *
 * Just calls the internal versions of the functions for now.
 *
 * @defgroup ILU Image Library Utilities
 *               Image manipulation utilities beyond just loading
 *               and saving.
 *
 * @ingroup ilu_ILU
 * @{
 * 
 * @defgroup ilu_setup  Initialization / Deinitalization
 * @defgroup ilu_filter Image Filters
 *                      Applying convolution matrices to images.
 *                      Blurring, sharpening, edge detection, etc.
 * @defgroup ilu_colour Image Colours
 *                      Colour corrections.
 *                      Brightness, contrast, gamma, etc.
 * @defgroup ilu_geometry Image Geometry
 *                      Simple image transformations.
 *                      Resizing, rescaling, flipping, mirroring, etc.
 * @defgroup ilu_util   Utilities
 * @defgroup ilu_state  Global Parameters and Configurations.
 *                      
 */
#include "ilu_filter.h"
#include "ilu_states.h"
#include "ilu_region.h"

/**
 * Funny as hell filter that I stumbled upon accidentally.
 * @bug Only works with IL_UNSIGNED_BYTE typed non paletted images.
 * @ingroup ilu_filter
 * @todo Honour set region
 */
ILboolean ILAPIENTRY iluAlienify(void) {
  return iAlienify(iGetCurImage());
}

/**
 * Average each pixel with its surrounding pixels @a Iter times.
 *
 * The 3x3 convolution filter used to do the average blur is:
 *
 * @f$ \frac{
 *       \left[ \begin{array}{rrr}  
 *         1 & 1 & 1 \\ 
 *         1 & 1 & 1 \\
 *         1 & 1 & 1    
 *       \end{array} \right] 
 *      }{     9     }
 * @f$
 * @ingroup ilu_filter
 * @todo Honour set region
 */
ILboolean ILAPIENTRY iluBlurAvg(ILuint Iter) {
  return iApplyFilter(iGetCurImage(), Iter, filter_average, filter_average_scale, filter_average_bias);
}

/**
 * Blurs an image using a Gaussian convolution filter, which usually gives 
 * better results than the filter used by iluBlurAvg. The filter is applied up 
 * to Iter number of times, giving more of a blurring effect the higher Iter is. 
 * The 3x3 convolution filter used to do the gaussian blur is:
 *
 * @f$ \frac{
 *       \left[ \begin{array}{rrr}  
 *         1 & 2 & 1 \\ 
 *         2 & 4 & 2 \\
 *         1 & 2 & 1    
 *       \end{array} \right] 
 *      }{    16     }
 * @f$
 * @ingroup ilu_filter
 * @todo Honour set region
 */
ILboolean ILAPIENTRY iluBlurGaussian(ILuint Iter) {
  return iApplyFilter(iGetCurImage(), Iter, filter_gaussian, filter_gaussian_scale, filter_gaussian_bias);
}

/**
 * Generates as many mipmaps for an image as possible. Mipmaps are then generated 
 * for the image, down to a 1x1x1 image. To use the mipmaps, see ilActiveMipmap.
 * @ingroup ilu_util
 */ 
ILboolean ILAPIENTRY iluBuildMipmaps() {
  ILimage *  Image = iGetCurImage();
  return iBuildMipmaps(Image, Image->Width >> 1, Image->Height >> 1, Image->Depth >> 1);
}

/**
 * Returns the number of colours used in the current image.
 * Creates a copy of the image data, quicksorts it and counts the number 
 * of unique colours in the image. This value is returned without affecting 
 * the original image. 
 * @bug Only works with IL_UNSIGNED_BYTE typed non paletted images.
 * @ingroup ilu_colour
 */
ILuint ILAPIENTRY iluColoursUsed() {
  return iColoursUsed(iGetCurImage());
}

/** 
 * Compares the current image to the image having the name in Comp. 
 * If both images are identical, IL_TRUE is returned. IL_FALSE is returned if the 
 * images are not identical. The bound image before calling this function remains 
 * the bound image after calling ilCompareImage. 
 * @ingroup ilu_util
 */
ILboolean ILAPIENTRY iluCompareImage(ILuint Comp) {
  ILimage * Image     = iGetCurImage();
  ILimage * Original  = iGetImage(Comp);

  return iCompareImage(Image, Original);
}

/**
 * changes the contrast of an image by using interpolation and extrapolation. 
 * Common values for Contrast are in the range -0.5 to 1.7. Anything below 0.0 
 * generates a negative of the image with varying contrast. 1.0 outputs the 
 * original image. 0.0 - 1.0 lowers the contrast of the image. 1.0 - 1.7 increases 
 * the contrast of the image. This effect is caused by interpolating (or 
 * extrapolating) the source image with a totally grey image. 
 * @bug Only works with IL_UNSIGNED_BYTE typed non paletted images.
 * @ingroup ilu_colour
 */
ILboolean ILAPIENTRY iluContrast(ILfloat Contrast) {
  return iContrast(iGetCurImage(), Contrast);
}

/**
 * Applies a user supplied convolution matrix.
 * The matrix must be a 3x3 matrix.
 * @ingroup ilu_filter
 * @todo Honour set region
 */
ILAPI ILboolean ILAPIENTRY iluConvolution(ILint *matrix, ILint scale, ILint bias) {
  return iApplyFilter(iGetCurImage(), 1, matrix, scale, bias);
}

/**
 * "Crops" the current image to new dimensions. The new image appears the 
 * same as the original, but portions of the image are clipped-off, depending on 
 * the values of the parameters of these functions. If XOff + Width, YOff + Height 
 * or ZOff + Depth is larger than the current image's dimensions, 
 * ILU_ILLEGAL_OPERATION is set. 
 * @ingroup ilu_geometry
 */
ILboolean ILAPIENTRY iluCrop(ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth) {
  return iCrop(iGetCurImage(), XOff, YOff, ZOff, Width, Height, Depth);
}

/**
 * Delete a single image.
 * @deprecated Use ilDeleteImage() instead.
 * @ingroup ilu_util
 */
void ILAPIENTRY iluDeleteImage(ILuint Id) {
  ilDeleteImages(1, &Id);
}

/**
 * Performs edge detection using an emboss convolution filter. 
 * The 3x3 convolution filter used to do the edge detected is:
 *
 * @f$ \frac{
 *       \left[ \begin{array}{rrr}  
 *         -1 & 0 & 1 \\ 
 *         -1 & 0 & 1 \\
 *         -1 & 0 & 1    
 *       \end{array} \right] 
 *      }{    1      }
 * @f$
 * @ingroup ilu_filter
 * @todo Honour set region
 */
ILboolean ILAPIENTRY iluEdgeDetectE() {
  return iApplyFilter(iGetCurImage(), 1, filter_embossedge, filter_embossedge_scale, filter_embossedge_bias);
}

/**
 * Detects the edges in the current image by combining two convolution filters.
 * The filters used are Prewitt filters. The two filters are: 
 *
 * @f$ 
 *      A = 
 *       \left[ \begin{array}{rrr}  
 *         -1 & -1 & -1 \\ 
 *         0 & 0 & 0 \\
 *         1 & 1 & 1    
 *       \end{array} \right] 
 *      ,
 *      B = 
 *       \left[ \begin{array}{rrr}  
 *         1 & 0 & -1 \\ 
 *         1 & 0 & -1 \\
 *         1 & 0 & -1    
 *       \end{array} \right] 
 * @f$
 * @ingroup ilu_filter
 * @todo Honour set region
 */
ILboolean ILAPIENTRY iluEdgeDetectP() {
  return iApplyFilter2(iGetCurImage(), 1, 
    filter_h_prewitt, filter_h_prewitt_scale, filter_h_prewitt_bias,
    filter_v_prewitt, filter_v_prewitt_scale, filter_v_prewitt_bias
  );
}

/**
 * Detects the edges in the current image by combining two convolution filters.
 * The filters used are Sobel filters. The two filters are: 
 *
 * @f$ 
 *      A = 
 *       \left[ \begin{array}{rrr}  
 *         1 & 2 & 1 \\ 
 *         0 & 0 & 0 \\
 *         -1 & -2 & -1    
 *       \end{array} \right] 
 *      ,
 *      B = 
 *       \left[ \begin{array}{rrr}  
 *         1 & 0 & -1 \\ 
 *         2 & 0 & -2 \\
 *         1 & 0 & -1    
 *       \end{array} \right] 
 * @f$
 * @ingroup ilu_filter
 * @todo Honour set region
 */
ILboolean ILAPIENTRY iluEdgeDetectS() {
  return iApplyFilter2(iGetCurImage(), 1, 
    filter_h_sobel, filter_h_sobel_scale, filter_h_sobel_bias,
    filter_v_sobel, filter_v_sobel_scale, filter_v_sobel_bias
  );
}

/** 
 * Embosses an image, causing it to have a "relief" feel to it. 
 * The 3x3 convolution filter used is:
 *
 * @f$ 
 *       \left[ \begin{array}{rrr}  
 *         -1 & 0 & 1 \\ 
 *         -1 & 0 & 1 \\
 *         -1 & 0 & 1    
 *       \end{array} \right] 
 *      
 *      + 0.5
 * @f$
 * @ingroup ilu_filter
 * @todo Honour set region
 */
ILboolean ILAPIENTRY iluEmboss() {
  return iApplyFilter(iGetCurImage(), 1, filter_emboss, filter_emboss_scale, filter_emboss_bias);
}

/**
 * Enlarges the canvas of the current image.
 * Clears the background to the colour specified in ilClearColour. 
 * To control the placement of the image, use iluImageParameter().
 * @ingroup ilu_geometry
 */ 
ILboolean ILAPIENTRY iluEnlargeCanvas(ILuint Width, ILuint Height, ILuint Depth) {
  return iEnlargeCanvas(iGetCurImage(), Width, Height, Depth, iluPlacement);
}

/**
 * Enlarges an image's dimensions by multipliers, via iluScale. 
 * This function could be useful if you wanted to double the size of all 
 * images or something similar. 
 * @ingroup ilu_geometry
 */
ILboolean ILAPIENTRY iluEnlargeImage(ILfloat XDim, ILfloat YDim, ILfloat ZDim) {
  if (XDim <= 0.0f || YDim <= 0.0f || ZDim <= 0.0f) {
    iSetError(ILU_INVALID_PARAM);
    return IL_FALSE;
  }

  ILimage *  Image = iGetCurImage();
  return iluScale((ILuint)(Image->Width * XDim), (ILuint)(Image->Height * YDim),
          (ILuint)(Image->Depth * ZDim));
// TODO:  return iScale(Image, (ILuint)(Image->Width * XDim), (ILuint)(Image->Height * YDim),
//          (ILuint)(Image->Depth * ZDim));
}

/**
 * Equalizes the intensities of an image.
 * The lowest intensity will result in black and the highest in white.
 * @ingroup ilu_colour
 */
ILboolean ILAPIENTRY iluEqualize() {
  return iEqualize(iGetCurImage());
}

/**
 * Returns a human-readable string of the error in @a Error. 
 * This can be useful for displaying the human-readable error in your program 
 * to let the user know wtf just happened. 
 * @see ilGetError
 * @ingroup ilu_util
 */
ILconst_string ILAPIENTRY iluErrorString(ILenum Error) {
  return iErrorString(Error);
}

/**
 * Flips an image over its x axis.
 * @ingroup ilu_geometry
 */
ILboolean ILAPIENTRY iluFlipImage() {
  return iFlipImage(iGetCurImage());
}

/**
 * Performs gamma correction of an image.
 * If Gamma is less than 1.0, the image is darkened. If Gamma is greater than 
 * 1.0, the image is brightened.
 * @ingroup ilu_colour
 */
ILboolean ILAPIENTRY iluGammaCorrect(ILfloat Gamma) {
  return iGammaCorrect(iGetCurImage(), Gamma);
}

/** 
 * Create a single new image and binds it.
 * @deprecated Use ilGenImage() instead.
 * @ingroup ilu_util
 */
ILuint ILAPIENTRY iluGenImage() {
  ILuint Id;
  ilGenImages(1, &Id);
  ilBindImage(Id);
  return Id;
}

/**
 * Retrieves information about the current bound image.
 * @ingroup ilu_util
 */
void ILAPIENTRY iluGetImageInfo(ILinfo *Info) {
  ILimage *  Image = iGetCurImage();
  if (Image == NULL || Info == NULL) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return;
  }

  Info->Id          = ilGetCurName();
  Info->Data        = ilGetData();
  Info->Width       = Image->Width;
  Info->Height      = Image->Height;
  Info->Depth       = Image->Depth;
  Info->Bpp         = Image->Bpp;
  Info->SizeOfData  = Image->SizeOfData;
  Info->Format      = Image->Format;
  Info->Type        = Image->Type;
  Info->Origin      = Image->Origin;
  Info->Palette     = Image->Pal.Palette;
  Info->PalType     = Image->Pal.PalType;
  Info->PalSize     = Image->Pal.PalSize;
  Info->NumNext     = iGetIntegerImage(Image, IL_NUM_IMAGES);
  Info->NumMips     = iGetIntegerImage(Image, IL_NUM_MIPMAPS);
  Info->NumLayers   = iGetIntegerImage(Image, IL_NUM_LAYERS);

}

/**
 * Returns the current value of the @a Mode.
 *
 * Valid @a Modes are:
 *
 * Mode                   | R/W | Default               | Description
 * ---------------------- | --- | --------------------- | ---------------------------------------------
 * ILU_VERSION_NUM        | R   | ILU_VERSION           | The version of the image library utilities.
 * ILU_FILTER             | RW  | ILU_NEAREST           | Currently selected interpolation filter. Can be ILU_NEAREST, ILU_LINEAR. ILU_BILINEAR, ILU_SCALE_BOX, ILU_SCALE_TRIANGLE, ILU_SCALE_BELL, ILU_SCALE_BSPLINE, ILU_SCALE_LANCZOS3 or ILU_SCALE_MITCHELL.
 *
 * @ingroup ilu_state
 */
ILint ILAPIENTRY iluGetInteger(ILenum Mode) {
  ILint Temp;
  Temp = 0;
  iluGetIntegerv(Mode, &Temp);
  return Temp;
}

/**
 * Returns the current value of the @a Mode and stores it in @a Param
 * @ingroup ilu_state
 */
void ILAPIENTRY iluGetIntegerv(ILenum Mode, ILint *Param) {
  return iGetIntegerv(Mode, Param);
}

/**
 * Returns a constant string detailing aspects about this library.
 *
 * Valid StringNames are:
 *
 * Name                           | R/W | Description
 * ------------------------------ | --- | -----------
 * @a ILU_VENDOR                  | R   | The name of the vendor of this version of the ILU implementation.
 * @a ILU_VERSION_NUM             | R   | Current version string of the ILU implementation.
 *
 * @param  StringName String to get.
 * @ingroup ilu_state
 */
ILconst_string ILAPIENTRY iluGetString(ILenum StringName) {
  return iGetString(StringName);
}

/**
 * Sets an image parameter for a @a PNames.
 *
 * Valid PNames are:
 *
 * Name                           | Description
 * ------------------------------ | -----------
 * @a ILU_FILTER                  | Image interpolation filter. Valid values are ILU_NEAREST, ILU_LINEAR. ILU_BILINEAR, ILU_SCALE_BOX, ILU_SCALE_TRIANGLE, ILU_SCALE_BELL, ILU_SCALE_BSPLINE, ILU_SCALE_LANCZOS3 or ILU_SCALE_MITCHELL.
 * @a ILU_PLACEMENT               | ALignment of image to use when enlarging the image's canvas. Can be ILU_LOWER_LEFT, ILU_LOWER_RIGHT, ILU_UPPER_LEFT, ILU_UPPER_RIGHT or ILU_CENTER.
 *
 * @ingroup ilu_state
 */
void ILAPIENTRY iluImageParameter(ILenum PName, ILenum Param) {
  iImageParameter(PName, Param);
}

/** 
 * Starts ILU and must be called prior to using any other ILU function.
 * @ingroup ilu_setup
 */ 
void ILAPIENTRY iluInit() {
  iInit();
}

/**
 * Inverts the alpha in the image.
 * Image must already have a valid alpha channel.
 * @ingroup ilu_colour
 * @todo Honour set region
 */
ILboolean ILAPIENTRY iluInvertAlpha() {
  return iInvertAlpha(iGetCurImage());
}

/**
 * Loads a file into a new image.
 * @ingroup ilu_util
 */
ILuint ILAPIENTRY iluLoadImage(ILconst_string FileName) {
  ILuint Id;
  ilGenImages(1, &Id);
  if (Id == 0)
    return 0;
  ilBindImage(Id);
  if (!ilLoadImage(FileName)) {
    ilDeleteImages(1, &Id);
    return 0;
  }
  return Id;
}

/**
 * Mirrors an image over its y axis.
 * @ingroup ilu_geometry
 */
ILboolean ILAPIENTRY iluMirror() {
  return iMirrorImage(iGetCurImage());
}


/** 
 * Inverts the colours in the image.
 * @ingroup ilu_colour
 */
ILboolean ILAPIENTRY iluNegative() {
  return iNegative(iGetCurImage());
}

/**
 * Adds some noise.
 * Very simple right now.
 * @todo This will probably use Perlin noise and parameters in the future.
 * @ingroup ilu_filter
 */
ILboolean ILAPIENTRY iluNoisify(ILclampf Tolerance) {
  return iNoisify(iGetCurImage(), Tolerance);
}

/**
 * Pixelizes an image.
 *
 * @todo Could probably be optimized a bit...too many nested for loops.
 * @ingroup ilu_filter
 */
ILboolean ILAPIENTRY iluPixelize(ILuint PixSize) {
  return iPixelize(iGetCurImage(), PixSize);
}

/**
 * Sets the currently active region.
 * If Points is NULL or n is 0, the region is unset.
 * @param Points Region points describing a polygon that encloses the region to set.
 *               Coordinates are relative to the boundaries size of the image bound
 *               when applying the region.
 * @param n Number of points. Must be at least 3 for a valid polygon.
 * @see iluRegioniv
 * @ingroup ilu_state
 */
void ILAPIENTRY iluRegionfv(ILUpointf *Points, ILuint n) {
  iRegionfv(Points, n);
}

/**
 * Sets the currently active region.
 * If Points is NULL or n is 0, the region is unset.
 * @param Points Region points describing a polygon that encloses the region to set.
 *               Coordinates are absolute pixel coordinates.
 * @param n Number of points. Must be at least 3 for a valid polygon.
 * @see iluRegionfv
 * @ingroup ilu_state
 */
void ILAPIENTRY iluRegioniv(ILUpointi *Points, ILuint n) {
  iRegioniv(Points, n);
}

/**
 * Replaces any occurence of the the color set by ilClearColour() with the 
 * given color where the euclidean distance between the colors is @a Tolerance
 * or lower.
 * @ingroup ilu_filter
 * @todo Honour set region.
 */
ILboolean ILAPIENTRY iluReplaceColour(ILubyte Red, ILubyte Green, ILubyte Blue, ILfloat Tolerance) {
  ILubyte ClearCol[4];
  ilGetClear(ClearCol, IL_RGBA, IL_UNSIGNED_BYTE);
  return iReplaceColour(iGetCurImage(), Red, Green, Blue, Tolerance, ClearCol);
}

/**
 * Simply rotates an image about the center by @a Angle degrees. 
 * The background where there is space left by the rotation will be 
 * filled with the colour set by ilClearColour().
 * @ingroup ilu_geometry
 */
ILboolean ILAPIENTRY iluRotate(ILfloat Angle) {
  return iRotate(iGetCurImage(), Angle);
}

/**
 * Rotates an image around a given axis.
 * @deprecated Not working. Never worked and I'm not planning to implement it.
 * @ingroup ilu_geometry
 */
ILboolean ILAPIENTRY iluRotate3D(ILfloat x, ILfloat y, ILfloat z, ILfloat Angle) {
  (void)x;
  (void)y;
  (void)z;
  (void)Angle;

  iSetError(ILU_ILLEGAL_OPERATION);
  return IL_FALSE;
}

/**
 * Modifies the color saturation of an image.
 * Applies a saturation consistent with the IL_LUMINANCE conversion values 
 * to red, green and blue.
 * All parameters go from -1.0 to 1.0. 
 * @ingroup ilu_colour
 */
ILboolean ILAPIENTRY iluSaturate1f(ILfloat Saturation) {
  return iSaturate4f(iGetCurImage(), 0.3086f, 0.6094f, 0.0820f, Saturation);
}

/**
 * Modifies the color saturation of an image.
 * Applies saturation to the colour components in the amounts specified.
 * All parameters go from -1.0 to 1.0. 
 * @ingroup ilu_colour
 */
ILboolean ILAPIENTRY iluSaturate4f(ILfloat r, ILfloat g, ILfloat b, ILfloat Saturation) {
  return iSaturate4f(iGetCurImage(), r,g,b, Saturation);
}

/**
 * Scales the image to the new dimensions specified, shrinking or enlarging 
 * the image, depending on the image's original dimensions. There are different 
 * filters that can be used to scale an image, and which filter to use can be 
 * specified via iluImageParameter. 
 * @ingroup ilu_geometry
 */
ILboolean ILAPIENTRY iluScale(ILuint Width, ILuint Height, ILuint Depth) {
  return iScale(iGetCurImage(), Width, Height, Depth);
}

/**
 * Scales the alpha components of the current bound image. Using 1.0f as the parameter
 * yields that the original alpha value.
 * @ingroup ilu_colour
 */
ILboolean ILAPIENTRY iluScaleAlpha(ILfloat scale) {
  return iScaleAlpha(iGetCurImage(), scale);
}

/**
 * Scales the individual colour components of the current bound image. Using 1.0f as
 * any of the parameters yields that colour component's original plane in the new image. 
 * @ingroup ilu_colour
 */
ILboolean ILAPIENTRY iluScaleColours(ILfloat r, ILfloat g, ILfloat b) {
  return iScaleColours(iGetCurImage(), r,g,b);
}

/**
 * Sets the current language to use for error messages.
 * @param Language Language to use, can be ILU_ENGLISH, ILU_ARABIC, ILU_DUTCH, ILU_JAPANESE, 
 *                 ILU_SPANISH, ILU_GERMAN or ILU_FRENCH.
 * @see iluErrorString
 * @ingroup ilu_state
 */
ILboolean ILAPIENTRY iluSetLanguage(ILenum Language) {
  return iSetLanguage(Language);
}

/**
 * Sharpens an image.
 * Can actually either sharpen or blur an image, depending on the value of Factor. 
 * iluBlurAvg and iluBlurGaussian are much faster for blurring, though. When Factor 
 * is 1.0, the image goes unchanged. When Factor is in the range 0.0 - 1.0, the current 
 * image is blurred. When Factor is in the range 1.0 - 2.5, the current image is sharpened.
 * To achieve a more pronounced sharpening/blurring effect, simply increase the number of 
 * iterations by increasing the value passed in Iter. 
 * @ingroup ilu_filter
 * @todo Honour set region.
 */
ILboolean ILAPIENTRY iluSharpen(ILfloat Factor, ILuint Iter) {
  return iSharpen(iGetCurImage(), Factor, Iter); 
}

/** 
 * "Swaps" the colour order of the current image. If the current image is in 
 * bgr(a) format, iluSwapColours will change the image to use rgb(a) format, or 
 * vice-versa. This can be helpful when you want to manipulate the image data yourself but 
 * only want to use a certain colour order. To determine the current colour order, call 
 * ilGetInteger with the IL_IMAGE_FORMAT parameter. 
 * @ingroup ilu_colour
 */
ILboolean ILAPIENTRY iluSwapColours() {
  return iSwapColours(iGetCurImage());
} 


ILboolean ILAPIENTRY iluWave(ILfloat Angle) {
  return iWave(iGetCurImage(), Angle);
}

/** @} */
