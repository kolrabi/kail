//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Last modified: 2014-05-30
//
// Filename: src/IL/il_api.c
//
//-----------------------------------------------------------------------------

/**
 * @file
 * @brief Contains public IL entry functions.
 *
 * Just calls the internal versions of the functions for now.
 *
 * @defgroup IL Image Library
 *              Loading, saving and converting images.
 * @ingroup IL 
 * @{
 * 
 * @defgroup state        Global State
 *                        General state and settings. 
 * @defgroup setup        Initialization / Deinitalization
 *                        Setting up the IL
 * @defgroup image_mgt    Image Management
 *                        Creating, deleting, copying, selecting images.
 * @defgroup image_manip  Image Manipulation
 *                        Perform operations on images.
 * @defgroup data         Image Data Handling
 *                        Operations on image data.
 * @defgroup file         Image File Operations
 *                        Loading, saving, determining type of image files.
 * @defgroup register     User Defined Image Types
 *                        Registering and unregistering loaders/savers for user
 *                        defined image types.
 */

#include "il_internal.h"
#include "il_stack.h"
#include "il_states.h"
#include "il_alloc.h"
#include "il_manip.h"
#include "il_register.h" 

/**
 * @def SIMPLE_PROC(img, f)
 * @internal
 */
#define SIMPLE_PROC(img, f) { \
  ILimage *img; \
  iLockState(); \
  img = iLockCurImage(); \
  iUnlockState(); \
  f; \
  iUnlockImage(img); }

/**
 * @def SIMPLE_FUNC(img, r, f)
 * @internal
 */
#define SIMPLE_FUNC(img, r, f) { \
  ILimage *img; r Result; \
  iLockState(); \
  img = iLockCurImage(); \
  iUnlockState(); \
  Result = f; \
  iUnlockImage(img); \
  return Result; }

/**
 * Sets the current face if the currently bound image is a cubemap.
 * @note If IL_IMAGE_SELECTION_MODE is set to IL_RELATIVE (default), the 
 * @a Number is the number of sub images down the chain, NOT the
 * absolute face index. To go back to the base image use ilBindImage.
 * @ingroup image_mgt
 */
ILboolean ILAPIENTRY ilActiveFace(ILuint Number) {
  // image selection is thread local, no lock necessary
  return iActiveFace(Number);
}

/**
 * Sets the current animation frame.
 * @note If IL_IMAGE_SELECTION_MODE is set to IL_RELATIVE (default), the 
 * @a Number is the number of sub images down the chain, NOT the
 * absolute frame index. To go back to the base image use ilBindImage.
 * @ingroup image_mgt
 */
ILboolean ILAPIENTRY ilActiveImage(ILuint Number) {
  // image selection is thread local, no lock necessary
  return iActiveImage(Number);
}

/**
 * Sets the current image layer.
 * @note If IL_IMAGE_SELECTION_MODE is set to IL_RELATIVE (default), the 
 * @a Number is the number of sub images down the chain, NOT the
 * absolute layer index. To go back to the base image use ilBindImage.
 * @ingroup image_mgt
 */
ILboolean ILAPIENTRY ilActiveLayer(ILuint Number) {
  // image selection is thread local, no lock necessary
  return iActiveLayer(Number);
}

/**
 * Sets the current mipmap level.
 * @note If IL_IMAGE_SELECTION_MODE is set to IL_RELATIVE (default), the 
 * @a Number is the number of sub images down the chain, NOT the
 * absolute mipmap level. To go back to the base image use ilBindImage.
 * @ingroup image_mgt
 */
ILboolean ILAPIENTRY ilActiveMipmap(ILuint Number) {
  // image selection is thread local, no lock necessary
  return iActiveMipmap(Number);
}

/**
 * Adds an opaque alpha channel to the currently bound image. 
 * If IL_USE_KEY_COLOUR is enabled, the colour set with ilKeyColour will 
 * be transparent.
 * @ingroup image_manip
 */
ILboolean ILAPIENTRY ilAddAlpha() {
  SIMPLE_FUNC(Image, ILboolean, iAddAlpha(Image));
}

/**
 * Loads a palette from a file and applies it to the currently bound image.
 * @ingroup image_manip
 */
ILboolean ILAPIENTRY ilApplyPal(ILconst_string FileName) {
  SIMPLE_FUNC(Image, ILboolean, iApplyPal(Image, FileName));  
}

/**
 * Apply a colour profile to the currently bound image.
 * Profile names must be the file names of valid ICC profiles.
 * @param InProfile Name of the image's current colour profile.
 * @param OutProfile Name of the colour profile to apply.
 * @note If the image library has been built without LCMS2 support,
 *       this will have no effect.
 * @ingroup image_manip
 */
ILboolean ILAPIENTRY ilApplyProfile(ILstring InProfile, ILstring OutProfile) {
  SIMPLE_FUNC(Image, ILboolean, iApplyProfile(Image, InProfile, OutProfile));  
}

/** 
 * Makes Image the current active image - similar to glBindTexture().
 *
 * This automatically resets the state to the first sub image, face 
 * (if applicable) and top level mipmap.
 * 
 * @param Image Name of image to bind.
 * @ingroup state
 */
void ILAPIENTRY ilBindImage(ILuint Image) {
  // image selection is thread local, no lock necessary
  iBindImage(Image);
}

/**
 * Blit a region of pixels from a @a Source image into the currently bound image.
 * @ingroup image_manip
 */ 
ILboolean ILAPIENTRY ilBlit(ILuint Source, ILint DestX,  ILint DestY,   ILint DestZ, 
                                           ILuint SrcX,  ILuint SrcY,   ILuint SrcZ,
                                           ILuint Width, ILuint Height, ILuint Depth)
{
  ILboolean Result;
  ILimage * Image;
  ILimage * SourceImage;

  if (iGetCurName() == Source) {
    // can't blit unto self
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }
  
  iLockState();
  Image       = iLockCurImage();
  SourceImage = iLockImage(Source);
  iUnlockState();

  Result      = iBlit(Image, SourceImage, DestX, DestY, DestZ, SrcX, SrcY, SrcZ, Width, Height, Depth);
  
  iUnlockImage(Image);
  iUnlockImage(SourceImage);

  return Result;
}

/**
 * Clamps data values of unsigned bytes from 16 to 235 for display on an
 * NTSC television.  Reasoning for this is given at
 *   http://msdn.microsoft.com/en-us/library/bb174608.aspx
 * @ingroup image_manip
 */
ILboolean ILAPIENTRY ilClampNTSC(void) {
  SIMPLE_FUNC(Image, ILboolean, iClampNTSC(Image)); 
}

/** 
 * Set a new clear colour to use by ilClearImage().
 * @ingroup state
 */
void ILAPIENTRY ilClearColour(ILclampf Red, ILclampf Green, ILclampf Blue, ILclampf Alpha) {
  // state is thread local, no lock necessary
  iClearColour(Red, Green, Blue, Alpha);
}

/** 
 * Set a new clear colour index to use by ilClearImage().
 * @ingroup state
 */
void ILAPIENTRY ilClearIndex(ILuint Index) {
  // state is thread local, no lock necessary
  iClearIndex(Index);
}

/**
 * Clears the current bound image to the values specified in ilClearColour().
 * @ingroup image_manip
 */
ILboolean ILAPIENTRY ilClearImage() {
  SIMPLE_FUNC(Image, ILboolean, iClearImage(Image)); 
}

/**
 * Removes all metadata from an image.
 */
void ILAPIENTRY ilClearMetadata() {
  SIMPLE_PROC(Image, iClearMetadata(Image));
}

/**
 * Creates a duplicate of the currently bound image.
 * @ingroup image_mgt
 */
ILuint ILAPIENTRY ilCloneCurImage() {
  ILimage *SrcImage;
  ILuint Result;

  iLockState();
  SrcImage = iLockCurImage();
  Result = iDuplicateImage(SrcImage);
  iUnlockImage(SrcImage);
  iUnlockState();
  return Result;
}


/**
 * Compresses data to a DXT format using different methods.
 * The data must be in unsigned byte RGBA or BGRA format.  Only DXT1, DXT3 and DXT5 are supported.
 * @ingroup data
 */
ILAPI ILubyte* ILAPIENTRY ilCompressDXT(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILenum DXTCFormat, ILuint *DXTCSize) {
  // doesn't change any global state, no lock
  return iCompressDXT(Data, Width, Height, Depth, DXTCFormat, DXTCSize);
}

/**
 * Converts the current image to the DestFormat format.
 * @param DestFormat An enum of the desired output format.  Any format values are accepted.
 * @param DestType An enum of the desired output type.  Any type values are accepted.
 * @exception IL_ILLEGAL_OPERATION  No currently bound image
 * @exception IL_INVALID_CONVERSION DestFormat or DestType was an invalid identifier.
 * @exception IL_OUT_OF_MEMORY      Could not allocate enough memory.
 * @return Boolean value of failure or success
 * @ingroup image_manip
 */
ILboolean ILAPIENTRY ilConvertImage(ILenum DestFormat, ILenum DestType) {
  SIMPLE_FUNC(Image, ILboolean, iConvertImages(Image, DestFormat, DestType)); 
}

/**
 * Converts the palette of current image to the DestFormat format.
 * @ingroup image_manip
 */
ILboolean ILAPIENTRY ilConvertPal(ILenum DestFormat) {
  SIMPLE_FUNC(Image, ILboolean, iConvertImagePal(Image, DestFormat));
}

/**
 * Copy the pixels of a region of the currently bound image to a buffer.
 * @param XOff    Left border of image subregion to copy in pixels.
 * @param YOff    Top border of image subregion to copy in pixels.
 * @param ZOff    Front border of image subregion to copy in slices.
 * @param Width   Width of region to copy in pixels.
 * @param Height  Height of region to copy in pixels.
 * @param Depth   Depth of region to copy in slices.
 * @param Format  Format of pixel data to get.
 * @param Type    Underlying pixel data type.
 * @param Data    Buffer to receive pixel data.
 * @ingroup image_manip
 */
ILuint ILAPIENTRY ilCopyPixels(ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth, ILenum Format, ILenum Type, void *Data) {
  SIMPLE_FUNC(Image, ILuint, iCopyPixels(Image, XOff, YOff, ZOff, Width, Height, Depth, Format, Type, Data));
}

/**
 * Copies everything from Src to the current bound image.
 * @param Src Name of source image from which to copy.
 * @ingroup image_mgt
 */
ILboolean ILAPIENTRY ilCopyImage(ILuint Src)
{
  ILimage * DestImage;
  ILimage * SrcImage;
  ILboolean Result;

  if (iGetCurName() == Src) {
    // can't copy unto self
    // iSetError(IL_INVALID_PARAM); // silently do nothing
    // return IL_FALSE;
    return IL_TRUE;
  }

  iLockState();
  DestImage = iLockCurImage();
  SrcImage  = iLockImage(Src);
  iUnlockState();

  Result    = iCopyImage(DestImage, SrcImage);
  iUnlockImage(DestImage);
  iUnlockImage(SrcImage);
  return Result;
}

/**
 * Creates sub images of the given type for the currently bound image.
 * The new sub images will be empty. Existing sub images of the type will be
 * replaced. The current image binding will not be changed.
 * @param Type    Sub image type, can be @a IL_SUB_NEXT to create animation frames
 *                after the current image, @a IL_SUB_MIPMAP to create mipmaps or
 *                @a IL_SUB_LAYER to create layers.
 * @param Num     The number of images to create.
 * @return        The number of images actually created.
 * @note The original version behaved a little differently, it only created one 
 *       sub image of the given type and the rest were added as frames in the 
 *       animation chain. I believe this was a bug and fixed it. However if your 
 *       program relied on that behaviour, it might be broken now. Be aware of that.
 * @ingroup image_manip
 */
ILuint ILAPIENTRY ilCreateSubImage(ILenum Type, ILuint Num) {
  SIMPLE_FUNC(Image, ILuint, iCreateSubImage(Image, Type, Num));
}

/**
 * Creates an ugly 64x64 black and yellow checkerboard image.
 * @ingroup image_manip 
 */
ILboolean ILAPIENTRY ilDefaultImage() {
  SIMPLE_FUNC(Image, ILboolean, iDefaultImage(Image));
}

/**
 * Delete one single image from the image stack.
 * @ingroup image_mgt
 */
void ILAPIENTRY ilDeleteImage(const ILuint Num) {
  ilDeleteImages(1, &Num);
}

/**
 * Deletes Num images from the image stack - similar to glDeleteTextures().
 * @ingroup image_mgt
 */
void ILAPIENTRY ilDeleteImages(ILsizei Num, const ILuint *Images) {
  iLockState();
  iDeleteImages(Num, Images);
  iUnlockState();
}

/**
 * Returns the size of the memory buffer needed to save the current
 * image into this Type. A return value of 0 is an error.
 * @ingroup file
 */
ILAPI ILuint ILAPIENTRY ilDetermineSize(ILenum Type) {
  SIMPLE_FUNC(Image, ILuint, iDetermineSize(Image, Type));
}

/**
 * Determines the type of a file from its contents or if
 * necessary from its extension. Uses globally set io functions (ilSetRead/ilSetReadF)
 * @ingroup file
 */
ILenum ILAPIENTRY ilDetermineType(ILconst_string FileName) {
  SIMPLE_FUNC(Image, ILenum, iDetermineType(Image, FileName));
}

/**
 * Determines the type of a file from its contents.
 * Uses globally set io functions (ilSetRead/ilSetReadF)
 * @ingroup file
 */
ILenum ILAPIENTRY ilDetermineTypeF(ILHANDLE File) {
  ILimage *Image;
  ILenum Result;

  iLockState();
  Image = iLockCurImage();
  iUnlockState();

  iSetInputFile(Image, File);
  Result = iDetermineTypeFuncs(Image);
  iUnlockImage(Image);
  return Result;
}

/**
 * Determines the type of a file from its contents starting from the
 * current position of the current file.
 * Uses globally set io functions (ilSetRead/ilSetReadF)
 * @ingroup file
 */
ILenum ILAPIENTRY ilDetermineTypeFuncs() {
  SIMPLE_FUNC(Image, ILenum, iDetermineTypeFuncs(Image));
}

/**
 * Determines the type of a lump in memory from its contents.
 * Uses globally set io functions (ilSetRead/ilSetReadF)
 * @ingroup file
 */
ILenum ILAPIENTRY ilDetermineTypeL(const void *Lump, ILuint Size) {
  ILimage *Image;
  ILenum Result;

  iLockState();
  Image = iLockCurImage();
  iUnlockState();

  iSetInputLump(Image, (void*)Lump, Size);
  Result = iDetermineTypeFuncs(Image);
  iUnlockImage(Image);
  return Result;
}

/**
 * Disables a mode.
 * @param  Mode Mode to disable
 * @return      IL_TRUE if successful.
 * @see         ilEnable for a list of valid modes.
 * @see         ilIsEnabled
 * @ingroup state
 */
ILboolean ILAPIENTRY ilDisable(ILenum Mode)
{
  return iAble(Mode, IL_FALSE);
}

/**
 * Decompresses stored DXTC data into the currently bound image and all of its mipmaps.
 * @ingroup data
 */
ILAPI ILboolean ILAPIENTRY ilDxtcDataToImage() {
  SIMPLE_FUNC(Image, ILboolean, iDxtcDataToImage(Image));
}

/**
 * Decompresses stored DXTC data into the currently bound image.
 * @ingroup data
 */
ILAPI ILboolean ILAPIENTRY ilDxtcDataToSurface() {
  SIMPLE_FUNC(Image, ILboolean, iDxtcDataToSurface(Image));
}

/**
 * Enables a mode.
 * Valid modes are:
 *
 * Mode                   | Default       | Description
 * ---------------------- | ------------- | ---------------------------------------------
 * @a IL_BLIT_BLEND       | @a IL_FALSE   | When using ilBlit do alpha blending.
 * @a IL_ORIGIN_SET       | @a IL_FALSE   | Flip image on load to match @a IL_ORIGIN_MODE.
 * @a IL_FORMAT_SET       | @a IL_FALSE   | Convert image format on load to match the format set by ilFormatFunc().
 * @a IL_TYPE_SET         | @a IL_FALSE   | Convert image data type on load to match @a IL_TYPE_MODE.
 * @a IL_CONV_PAL         | @a IL_FALSE   | Convert images that use palettes on load to 24 bit RGBA.
 * @a IL_SQUISH_COMPRESS  | @a IL_FALSE   | Use libsquish for compressing DXT formats if available.
 * 
 * @param  Mode Mode to enable
 * @return      IL_TRUE if successful.
 * @see         ilIsEnabled
 * @ingroup state
 */
ILboolean ILAPIENTRY ilEnable(ILenum Mode) {
  return iAble(Mode, IL_TRUE);
}

/**
 * Enumerate over all metadata.
 */
ILAPI ILboolean ILAPIENTRY ilEnumMetadata(ILuint Index, ILenum *IFD, ILenum *ID) {
  SIMPLE_FUNC(Image, ILboolean, iEnumMetadata(Image, Index, IFD, ID));
}

/**
 * Flips the stored DXTC data of the currently bound image vertically.
 * @ingroup data
 */
void ILAPIENTRY ilFlipSurfaceDxtcData() {
  SIMPLE_PROC(Image, iFlipSurfaceDxtcData(Image));
}

/**
 * Set the default image format to use. Default value IL_BGRA.
 * The current value can be retrieved by calling @a ilGetInteger with 
 * the parameter @a IL_FORMAT_MODE.
 * @param  Mode New default format.
 * @return      IL_TRUE if successful, on failure IL_FALSE is returned and 
 *              an error is set.
 * @ingroup state
 */
ILboolean ILAPIENTRY ilFormatFunc(ILenum Mode)
{
  return iFormatFunc(Mode);
}

/** 
 * Create one single new image on the image stack.
 * @ingroup image_mgt
 */
ILuint ILAPIENTRY ilGenImage(void) {
  ILuint Image;
  ilGenImages(1, &Image);
  return Image;
}

/** 
 * Creates Num images and puts their index in Images - similar to glGenTextures().
 * @ingroup image_mgt
 */
void ILAPIENTRY ilGenImages(ILsizei Num, ILuint *Images) {
  iGenImages(Num, Images);
}

/**
 * Extract the alpha channel from the currently bound image.
 * @param Type Data type to use.
 * @ingroup data
 */
ILubyte* ILAPIENTRY ilGetAlpha(ILenum Type) {
  SIMPLE_FUNC(Image, ILubyte *, iGetAlpha(Image, Type));
}

/**
 * Sets @a Param equal to the current value of the @a Mode.
 * @ingroup state
 */
void ILAPIENTRY ilGetBooleanv(ILenum Mode, ILboolean *Param)
{
  ILimage *Image;
  ILint Temp;

  if (Param == NULL) {
    iSetError(IL_INVALID_PARAM);
    return;
  }

  iLockState();
  Image = iLockCurImage();
  iUnlockState();

  iGetiv(Image, Mode, &Temp, 1);
  iUnlockImage(Image);  

  *Param = Temp ? IL_TRUE : IL_FALSE;
}


/**
 * Returns the current value of the @a Mode.
 * @ingroup state
 */
ILboolean ILAPIENTRY ilGetBoolean(ILenum Mode) {
  ILboolean Result;
  ilGetBooleanv(Mode, &Result);
  return Result;
}

/**
 * Returns a pointer to the current image's data.
 * The pointer to the image data returned by this function is only valid until any
 * operations are done on the image.  After any operations, this function should be
 * called again.  The pointer can be cast to other types for images that have more
 * than one byte per channel for easier access to data.
 * @exception IL_ILLEGAL_OPERATION No currently bound image
 * @return ILubyte pointer to image data.
 * @ingroup data
 */
ILubyte* ILAPIENTRY ilGetData(void) {
  SIMPLE_FUNC(Image, ILubyte *, iGetData(Image));
}


/**
 * Compress image data using internal DXTC compression and
 * copy the data into the supplied buffer and returns the number 
 * of bytes written. If Buffer is NULL no data is written and only 
 * the minimum size of the Buffer is returned.
 * @ingroup data
 */
ILuint ILAPIENTRY ilGetDXTCData(void *Buffer, ILuint BufferSize, ILenum DXTCFormat) {
  SIMPLE_FUNC(Image, ILuint, iGetDXTCData(Image, Buffer, BufferSize, DXTCFormat));
}


/** 
 * Gets the last error on the error stack
 * @return  An enum describing the last error.
 * @ingroup state
 */
ILenum ILAPIENTRY ilGetError(void) {
  return iGetError();
}

/**
 * Gets a current value from an image.
 */
ILfloat ILAPIENTRY ilGetFloatImage(ILuint ImageName, ILenum Mode) {
  ILimage *Image;
  ILfloat Result = 0.0f;

  iLockState();
  Image = iLockImage(ImageName);
  iUnlockState();

  if (!Image) {
    iSetError(IL_INVALID_PARAM);
    return 0;
  }

  iGetfv(Image, Mode, &Result, 1);
  iUnlockImage(Image);
  return Result;
}

/**
 * Gets the current value(s) of the @a Mode.
 * @param Mode Which value(s) to get
 * @param Param Where to store the value(s), can be NULL. If not NULL, it must
 *              point to an array of ILints that is large enough to contain all
 *              values.
 * @return Number of float values.
 * @see ilGetFloat
 * @ingroup state
 */
ILuint ILAPIENTRY ilGetFloatv(ILenum Mode, ILfloat *Param) {
  SIMPLE_FUNC(Image, ILuint, iGetfv(Image, Mode, Param, -1));
}

/**
 * Returns a current float value.
 *
 * Valid @a Modes are:
 *
 * Mode                   | R/W | Default               | Description
 * ---------------------- | --- | --------------------- | ---------------------------------------------
 * IL_META_EXPOSURE_TIME
 * IL_META_FSTOP
 * IL_META_SHUTTER_SPEED
 * IL_META_APERTURE
 * IL_META_BRIGHTNESS
 * IL_META_EXPOSURE_BIAS
 * IL_META_MAX_APERTURE
 * IL_META_SUBJECT_DISTANCE
 * IL_META_FOCAL_LENGTH
 * IL_META_FLASH_ENERGY
 * IL_META_X_RESOLUTION
 * IL_META_Y_RESOLUTION
 * IL_META_GPS_LATITUDE
 * IL_META_GPS_LONGITUDE
 * IL_META_GPS_ALTITUDE
 * IL_META_GPS_DOP
 * IL_META_GPS_SPEED
 * IL_META_GPS_TRACK
 * IL_META_GPS_IMAGE_DIRECTION
 * IL_META_GPS_DEST_LATITUDE
 * IL_META_GPS_DEST_LONGITUDE
 * IL_META_GPS_DEST_ALTITUDE
 * IL_META_GPS_DEST_BEARING
 * IL_META_GPS_DEST_DISTANCE
 *
 * @see ilGetIntegerImage for image specific modes.
 * @ingroup state
 */
ILfloat ILAPIENTRY ilGetFloat(ILenum Mode) {
  ILimage *Image;
  ILfloat Result = 0;

  iLockState();
  Image = iLockCurImage();
  iUnlockState();

  if (!Image) {
    iSetError(IL_INVALID_PARAM);
    return 0;
  }

  iGetfv(Image, Mode, &Result, 1);
  iUnlockImage(Image);
  return Result;  
}

/**
 * Gets the current value(s) of the @a Mode.
 * @param Mode Which value(s) to get
 * @param Param Where to store the value(s), can be NULL. If not NULL, it must
 *              point to an array of ILints that is large enough to contain all
 *              values.
 * @return Number of integer values.
 * @see ilGetInteger
 * @ingroup state
 */
ILuint ILAPIENTRY ilGetIntegerv(ILenum Mode, ILint *Param) {
  SIMPLE_FUNC(Image, ILuint, iGetiv(Image, Mode, Param, -1));
}

/**
 * Returns the current value of the @a Mode.
 *
 * Valid @a Modes are:
 *
 * Mode                   | R/W | Default               | Description
 * ---------------------- | --- | --------------------- | ---------------------------------------------
 * IL_CUR_IMAGE           | R   | 0                     | The name of the currently bound image set by ilBindImage().
 * IL_FORMAT_MODE         | RW  | IL_BGRA               | The format to convert loaded images into if IL_FORMAT_SET is enabled.
 * IL_KEEP_DXTC_DATA      | RW  | IL_FALSE              | When loading DXTC compressed images, keep a copy of the original data around.
 * IL_ORIGIN_MODE         | RW  | IL_ORIGIN_LOWER_LEFT  | Specify the origin of the image, can be @a IL_ORIGIN_LOWER_LEFT or @a IL_ORIGIN_UPPER_LEFT.
 * IL_MAX_QUANT_INDICES   | RW  | 256                   | Maximum number of colour indices to use when quantizing images.
 * IL_NEU_QUANT_SAMPLE    | RW  | 15                    | Number of samples to use when quantizing with NeuQuant.
 * IL_QUANTIZATION_MODE   | RW  | IL_WU_QUANT           | Quantizer to use, can be @a IL_WU_QUANT or @a IL_NEU_QUANT.
 * IL_TYPE_MODE           | RW  | IL_UNSIGNED_BYTE      | The type to convert loaded images into if IL_TYPE_SET is enabled.
 * IL_VERSION_NUM         | R   | IL_VERSION            | The version of the image library.
 * IL_ACTIVE_IMAGE        | R   | 0                     | The currently active sub image, set by ilActiveImage().
 * IL_ACTIVE_MIPMAP       | R   | 0                     | The currently active mipmap, set by ilActiveMipmap().
 * IL_ACTIVE_LAYER        | R   | 0                     | The currently active layer, set by ilActiveLayer().
 * IL_BMP_RLE             | RW  | IL_FALSE              | Use RLE when writing BMPs.
 * IL_DXTC_FORMAT         | RW  | IL_DXT1               | DXTC format to use when compressing, can be IL_DXT1, IL_DXT3, IL_DXT4, IL_DXT5, IL_DXT_NO_COMP.
 * IL_JPG_QUALITY         | RW  | 99                    | JPEG compression quality used when writing JPEG files.
 * IL_PCD_PICNUM          | RW  | 2                     | Select picture resolution for Kodak Photo CD files.
 * IL_PNG_ALPHA_INDEX     | RW  | -1                    | Define a colour index as transparent, saved in the tRNS chunk.
 * IL_PNG_INTERLACE       | RW  | IL_FALSE              | Use interlacing when writing PNG files.
 * IL_SGI_RLE             | RW  | IL_FALSE              | Use RLE when writing SGI files.
 * IL_TGA_RLE             | RW  | IL_FALSE              | Use RLE when writing Targa files.
 * IL_VTF_COMP            | RW  | IL_DXT_NO_COMP        | Compression to use when writing VTF files, can be IL_DXT_NO_COMP, IL_DXT1, IL_DXT3, IL_DXT4, IL_DXT5.
 *
 * @see ilGetIntegerImage for image specific modes.
 * @ingroup state
 */
ILint ILAPIENTRY ilGetInteger(ILenum Mode) {
  ILimage *Image;
  ILint Result = 0;

  iLockState();
  Image = iLockCurImage();
  iUnlockState();

  if (!Image) {
    iSetError(IL_INVALID_PARAM);
    return 0;
  }

  iGetiv(Image, Mode, &Result, 1);
  iUnlockImage(Image);
  return Result;
}

/**
 * Get a value about a specific image.
 * The @a Modes listed here can also be retrieved for the currently bound
 * image by calling ilGetInteger().
 *
 * Valid Modes are:
 * 
 * Mode                       | Description
 * -------------------------- | -----------------------------------
 * IL_DXTC_DATA_FORMAT        | Format of the retained compressed DXTC data (if IL_KEEP_DXTC_DATA is enabled on load).
 * IL_IMAGE_BITS_PER_PIXEL    | Bits per pixel.
 * IL_IMAGE_BYTES_PER_PIXEL   | Bytes per pixel.
 * IL_IMAGE_BPC               | Bytes per channel.
 * IL_IMAGE_CHANNELS          | Image colour channel count.
 * IL_IMAGE_CUBEFLAGS         | Cubemap face of image if it is a cubemap.
 * IL_IMAGE_DEPTH             | Depth of image in pixels (number 2d images along the Z axis).
 * IL_IMAGE_DURATION          | Duration of the image in an animation in milliseconds.
 * IL_IMAGE_FORMAT            | Pixel format of image.
 * IL_IMAGE_HEIGHT            | Height of image in pixels.
 * IL_IMAGE_SIZE_OF_DATA      | Total number of bytes in image data buffer.
 * IL_IMAGE_OFFX              | X offset of image in pixels.
 * IL_IMAGE_OFFY              | Y offset of image in pixels.
 * IL_IMAGE_ORIGIN            | Origin of image.
 * IL_IMAGE_PLANESIZE         | Size of one 2d image plane in bytes.
 * IL_IMAGE_TYPE              | Data type of bytes in data buffer.
 * IL_IMAGE_WIDTH             | Width of image in pixels.
 * IL_NUM_FACES               | Number of faces (== 5 for cubemaps).
 * IL_NUM_IMAGES              | Number of following sub images (eg. in an animation).
 * IL_NUM_LAYERS              | Number of layers in image.
 * IL_NUM_MIPMAPS             | Number of mipmaps contained in image.
 * IL_PALETTE_TYPE            | Type of palette data if any.
 * IL_PALETTE_BPP             | Bytes pro palette entry.
 * IL_PALETTE_NUM_COLS        | Total number of palette entries.
 * IL_PALETTE_BASE_TYPE       | Pixel format for all palette entries.
 *
 * @ingroup image_mgt
 */
ILint ILAPIENTRY ilGetIntegerImage(ILuint ImageName, ILenum Mode) {
  ILimage *Image;
  ILint Result;

  iLockState();
  Image = iLockImage(ImageName);
  iUnlockState();

  if (!Image) {
    iSetError(IL_INVALID_PARAM);
    return 0;
  }

  iGetiv(Image, Mode, &Result, 1);
  iUnlockImage(Image);
  return Result;
}

/**
 * Get the current position into the memory lump (if any is set) 
 * of the currently bound image.
 * @ingroup file
 */
ILuint64 ILAPIENTRY ilGetLumpPos() {
  SIMPLE_FUNC(Image, ILuint64, iGetLumpPos(Image));
}

/**
 * Retrieve image metadata.
 */
ILboolean ILAPIENTRY ilGetMetadata(ILenum Category, ILenum ID, ILenum *Type, ILuint *Count, ILuint *Size, void **Data) {
  SIMPLE_FUNC(Image, ILboolean, iGetMetadata(Image, Category, ID, Type, Count, Size, Data));
}

/**
 * Returns a pointer to the current image's palette data.
 * The pointer to the image palette data returned by this function is only valid until
 * any operations are done on the image.  After any operations, this function should be
 * called again.
 * @exception IL_ILLEGAL_OPERATION No currently bound image
 * @return ILubyte pointer to image palette data.
 * @ingroup data
 */
ILubyte* ILAPIENTRY ilGetPalette(void) {
  SIMPLE_FUNC(Image, ILubyte*, iGetPalette(Image));
}

/**
 * Returns a constant string detailing aspects about this library.
 *
 * Valid StringNames are:
 *
 * Name                           | R/W | Description
 * ------------------------------ | --- | -----------
 * @a IL_VENDOR                   | R   | The name of the vendor of this version of the IL implementation.
 * @a IL_VERSION_NUM              | R   | Current version string of the IL implementation.
 * @a IL_LOAD_EXT                 | R   | A string containing extensions of all files that can be loaded.
 * @a IL_SAVE_EXT                 | R   | A string containing extensions of all files that can be saved.
 * @a IL_TGA_ID_STRING            | RW  | Identifier string to be used when writing Targa image files. (obsolete: use IL_META_DOCUMENT_NAME)
 * @a IL_TGA_AUTHNAME_STRING      | RW  | Author name to be used when writing Targa image files. (obsolete: use IL_META_ARTIST)
 * @a IL_TGA_AUTHCOMMENT_STRING   | RW  | Author comment to be used when writing Targa image files.  (obsolete: use IL_META_USER_COMMENT)
 * @a IL_PNG_AUTHNAME_STRING      | RW  | Author name to be used when writing PNG image files. (obsolete: use IL_META_ARTIST)
 * @a IL_PNG_TITLE_STRING         | RW  | Image title to be used when writing PNG image files. (obsolete: use IL_META_DOCUMENT_NAME)
 * @a IL_PNG_DESCRIPTION_STRING   | RW  | Image description to be used when writing PNG image files. (obsolete: use IL_META_IMAGE_DESCRIPTION)
 * @a IL_TIF_DESCRIPTION_STRING   | RW  | Image description to be used when writing TIFF image files. (obsolete: use IL_META_IMAGE_DESCRIPTION)
 * @a IL_TIF_HOSTCOMPUTER_STRING  | RW  | Name of host computer to be used when writing TIFF image files. (obsolete: use IL_META_HOST_COMPUTER)
 * @a IL_TIF_DOCUMENTNAME_STRING  | RW  | Document name to be used when writing TIFF image files. (obsolete: use IL_META_DOCUMENT_NAME)
 * @a IL_TIF_AUTHNAME_STRING      | RW  | Author name to be used when writing TIFF image files. (obsolete: use IL_META_ARTIST)
 * @a IL_CHEAD_HEADER_STRING      | RW  | Variable name to use when writing C headers.
 * @a IL_META_ARTIST              | RW  | Author name meta data to be used when writing images.
 * @a IL_META_DOCUMENT_NAME       | RW  | Document name meta data to be used when writing images.
 * @a IL_META_HOST_COMPUTER       | RW  | Host computer name meta data to be used when writing images.
 * @a IL_META_USER_COMMENT        | RW  | User comment about the image.
 * @a IL_META_IMAGE_DESCRIPTION   | RW  | String describing the image contents.
 *
 * Strings marked with RW can also be set using ilSetString();
 * 
 * @param  StringName String to get.
 * @note   If it belongs to the current image the returned string is valid only
 *         as long as the image exists!
 * @ingroup state
 */
ILconst_string ILAPIENTRY ilGetString(ILenum StringName) {
  SIMPLE_FUNC(Image, ILconst_string, iGetILString(Image, StringName));
}

/**
 * Specifies implementation-dependent performance hints. These are only recommendations
 * for the image library and it is free to ignore them.
 * 
 * Valid @a Targets are:
 * 
 * Target                 | Default           | Description
 * ---------------------- | ----------------- | ---------------------------------------------
 * @a IL_MEM_SPEED_HINT   | IL_FASTEST        | Preference between speed and memory usage. Can be @a IL_LESS_MEM or @a IL_FASTEST.
 * @a IL_USE_COMPRESSION  | IL_NO_COMPRESSION | Whether to use compression when writing formats that support it optionally. Can be @a IL_USE_COMPRESSION or @a IL_NO_COMPRESSION.
 * @ingroup state
 */
void ILAPIENTRY ilHint(ILenum Target, ILenum Mode) {
  iHint(Target, Mode);
}

/**
 * Convert the DXTC data of the currently bound image and its 
 * mipmaps into image data.
 * @ingroup data
 */
ILboolean ILAPIENTRY ilImageToDxtcData(ILenum Format) {
  SIMPLE_FUNC(Image, ILboolean, iImageToDxtcData(Image, Format));
}

/**
 * Initialize the image library.
 * This must be called before calling any other IL functions or their behaviour
 * is undefined.
 * @ingroup setup
 */
void ILAPIENTRY ilInit(void) {
  iInitIL();
}

/**
 * Invert the alpha channel in the DXTC data of the currently bound image.
 * @ingroup data
 */
ILAPI ILboolean ILAPIENTRY ilInvertSurfaceDxtcDataAlpha() {
  SIMPLE_FUNC(Image, ILboolean, iInvertSurfaceDxtcDataAlpha(Image));
}

/**
 * Checks whether a @a Mode is not enabled.
 * @see ilEnable
 * @see ilDisable
 * @ingroup state
 */
ILboolean ILAPIENTRY ilIsDisabled(ILenum Mode) {
  return !iIsEnabled(Mode);
}

/**
 * Checks whether a @a Mode is enabled.
 * @see ilEnable
 * @see ilDisable
 * @ingroup state
 */
ILboolean ILAPIENTRY ilIsEnabled(ILenum Mode) {
  return iIsEnabled(Mode);
}

/**
 * Checks whether a given @a Image name is valid.
 * @ingroup image_mgt
 */
ILboolean ILAPIENTRY ilIsImage(ILuint Image) {
  return iIsImage(Image);
}

/**
 * Check if a file is of a given type. Uses globally set io functions (ilSetRead/ilSetReadF)
 * @ingroup file
 */
ILboolean ILAPIENTRY ilIsValid(ILenum Type, ILconst_string FileName) {
  ILimage *Image;
  ILboolean Result = IL_FALSE;
  SIO io;

  if (FileName == NULL) {
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }

  iLockState();
  Image = iLockCurImage();
  iUnlockState();

  io = Image->io;

  // If we can open the file, determine file type from contents
  // This is more reliable than the file name extension 
  if (SIOopenRO(&io, FileName)) {
    Result = iIsValidIO(Type, &io);
    SIOclose(&io);
  }
  iUnlockImage(Image);
  return Result;
}

/**
 * Check if a file is of a given type. Uses globally set io functions (ilSetRead/ilSetReadF)
 * @ingroup file
 */
ILboolean ILAPIENTRY ilIsValidF(ILenum Type, ILHANDLE File) {
  ILimage *Image;
  ILboolean Result = IL_FALSE;
  SIO io;

  if (File == NULL) {
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }

  iLockState();
  Image = iLockCurImage();
  iUnlockState();

  io = Image->io;

  io.handle = File;
  Result = iIsValidIO(Type, &io);
  iUnlockImage(Image);
  return Result;
}

/**
 * Check if a memory lump is of a given type. Uses globally set io functions (ilSetRead/ilSetReadF)
 * @ingroup file
 */
ILboolean ILAPIENTRY ilIsValidL(ILenum Type, void *Lump, ILuint Size) {
  SIO io;

  if (Lump == NULL) {
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }

  iSetInputLumpIO(&io, Lump, Size);
  return iIsValidIO(Type, &io);
}

/**
 * Set the current color key.
 * @ingroup state
 */
void ILAPIENTRY ilKeyColour(ILclampf Red, ILclampf Green, ILclampf Blue, ILclampf Alpha) {
  iKeyColour(Red, Green, Blue, Alpha);
}

/**
 * Attempts to load an image from a file.  The file format is specified 
 * by the user.
 * @param Type Format of this file. Acceptable values are IL_BLP, IL_BMP, IL_CUT, IL_DCX, IL_DDS,
 * IL_DICOM, IL_DOOM, IL_DOOM_FLAT, IL_DPX, IL_EXR, IL_FITS, IL_FTX, IL_GIF, IL_HDR, IL_ICO, IL_ICNS,
 * IL_IFF, IL_IWI, IL_JP2, IL_JPG, IL_LIF, IL_MDL, IL_MNG, IL_MP3, IL_PCD, IL_PCX, IL_PIX, IL_PNG,
 * IL_PNM, IL_PSD, IL_PSP, IL_PXR, IL_ROT, IL_SGI, IL_SUN, IL_TEXTURE, IL_TGA, IL_TIF, IL_TPL,
 * IL_UTX, IL_VTF, IL_WAL, IL_WBMP, IL_XPM, IL_RAW, IL_JASC_PAL and IL_TYPE_UNKNOWN.
 * If IL_TYPE_UNKNOWN is specified, ilLoad will try to determine the type of the file and load it.
 * @param FileName Ansi or Unicode string, depending on the compiled version of DevIL, that gives
 *        the filename of the file to load.
 * @return Boolean value of failure or success.  Returns IL_FALSE if all three loading methods
 *        have been tried and failed.
 * @ingroup file
 */
ILboolean ILAPIENTRY ilLoad(ILenum Type, ILconst_string FileName) {
  SIMPLE_FUNC(Image, ILboolean, iLoad(Image, Type, FileName));
}


/**
 * Loads raw data from a file.
 * @ingroup file
 */
ILboolean ILAPIENTRY ilLoadData(ILconst_string FileName, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp) {
  SIMPLE_FUNC(Image, ILboolean, iLoadData(Image, FileName, Width, Height, Depth, Bpp));
}

/**
 * Loads raw data from an already-opened file.
 * @ingroup file
 */
ILboolean ILAPIENTRY ilLoadDataF(ILHANDLE File, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp) {
  SIMPLE_FUNC(Image, ILboolean, iLoadDataF(Image, File, Width, Height, Depth, Bpp));
}

/**
 * Loads raw data from a memory "lump"
 * @ingroup file
 */
ILboolean ILAPIENTRY ilLoadDataL(void *Lump, ILuint Size, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp) {
  ILimage *Image;
  ILboolean Result;

  iLockState();
  Image = iLockCurImage();
  iUnlockState();

  iSetInputLump(Image, Lump, Size);
  Result = iLoadDataInternal(Image, Width, Height, Depth, Bpp);
  iUnlockImage(Image);
  return Result;
}


/**
 * Attempts to load an image from a file stream.  The file format is 
 * specified by the user.
 * @param Type Format of this file.  Acceptable values are IL_BLP, IL_BMP, IL_CUT, IL_DCX, IL_DDS,
 * IL_DICOM, IL_DOOM, IL_DOOM_FLAT, IL_DPX, IL_EXR, IL_FITS, IL_FTX, IL_GIF, IL_HDR, IL_ICO, IL_ICNS,
 * IL_IFF, IL_IWI, IL_JP2, IL_JPG, IL_LIF, IL_MDL, IL_MNG, IL_MP3, IL_PCD, IL_PCX, IL_PIX, IL_PNG,
 * IL_PNM, IL_PSD, IL_PSP, IL_PXR, IL_ROT, IL_SGI, IL_SUN, IL_TEXTURE, IL_TGA, IL_TIF, IL_TPL,
 * IL_UTX, IL_VTF, IL_WAL, IL_WBMP, IL_XPM, IL_RAW, IL_JASC_PAL and IL_TYPE_UNKNOWN.
 * If IL_TYPE_UNKNOWN is specified, ilLoadF will try to determine the type of the file and load it.
 * @param File File stream to load from. The caller is responsible for closing the handle.
 * @return Boolean value of failure or success.  Returns IL_FALSE if loading fails.
 * @ingroup file
 */
ILboolean ILAPIENTRY ilLoadF(ILenum Type, ILHANDLE File) {
  ILimage *Image;
  ILboolean Result;

  if (File == NULL) {
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }

  iLockState();
  Image = iLockCurImage();
  iUnlockState();

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  iSetInputFile(Image, File);
  SIOseek(&Image->io, 0, IL_SEEK_SET);

  if (Type == IL_TYPE_UNKNOWN)
    Type = iDetermineTypeFuncs(Image);
 
  Result = iLoadFuncs2(Image, Type);
  iUnlockImage(Image);

  return Result;
}

/**
 * Attempts to load an image using the currently set IO functions. 
 * The file format is specified by the user.
 * @param Type Format of this file.  Acceptable values are IL_BLP, IL_BMP, IL_CUT, IL_DCX, IL_DDS,
 * IL_DICOM, IL_DOOM, IL_DOOM_FLAT, IL_DPX, IL_EXR, IL_FITS, IL_FTX, IL_GIF, IL_HDR, IL_ICO, IL_ICNS,
 * IL_IFF, IL_IWI, IL_JP2, IL_JPG, IL_LIF, IL_MDL, IL_MNG, IL_MP3, IL_PCD, IL_PCX, IL_PIX, IL_PNG,
 * IL_PNM, IL_PSD, IL_PSP, IL_PXR, IL_ROT, IL_SGI, IL_SUN, IL_TEXTURE, IL_TGA, IL_TIF, IL_TPL,
 * IL_UTX, IL_VTF, IL_WAL, IL_WBMP, IL_XPM, IL_RAW, IL_JASC_PAL and IL_TYPE_UNKNOWN.
 * If IL_TYPE_UNKNOWN is specified, ilLoadFuncs fails.
 * @return Boolean value of failure or success.  Returns IL_FALSE if loading fails.
 * @ingroup file
 */
ILboolean ILAPIENTRY ilLoadFuncs(ILenum Type) {
  ILimage *Image;
  ILboolean Result;

  iLockState();
  Image = iLockCurImage();
  iUnlockState();

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (Type == IL_TYPE_UNKNOWN)
    Type = iDetermineTypeFuncs(Image);

  Result = iLoadFuncs2(Image, Type);
  iUnlockImage(Image);

  return Result;
}


/**
 * Attempts to load an image from a file with various different methods 
 * before failing - very generic.
 * The ilLoadImage function allows a general interface to the specific internal file-loading
 * routines.  First, it finds the extension and checks to see if any user-registered functions
 * (registered through ilRegisterLoad) match the extension. If nothing matches, it takes the
 * extension and determines which function to call based on it. Lastly, it attempts to identify
 * the image based on various image header verification functions, such as ilIsValidPngF.
 * If all this checking fails, IL_FALSE is returned with no modification to the current bound image.
 * @param FileName Ansi or Unicode string, depending on the compiled version of DevIL, that gives
 *        the filename of the file to load.
 * @return Boolean value of failure or success.  Returns IL_FALSE if all three loading methods
 *        have been tried and failed.
 * @ingroup file
 */
ILboolean ILAPIENTRY ilLoadImage(ILconst_string FileName) {
  ILimage *Image;
  ILenum type;
  ILboolean Result;

  if (FileName == NULL || iStrLen(FileName) < 1) {
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }

  iLockState();
  Image = iLockCurImage();
  iUnlockState();

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (Image->io.openReadOnly == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    iUnlockImage(Image);
    return IL_FALSE;
  }
 
  type = iDetermineType(Image, FileName);
  Result = IL_FALSE;

  if (type != IL_TYPE_UNKNOWN) {
    Result = iLoad(Image, type, FileName);
  } else if (!iLoadRegistered(FileName)) {
    iSetError(IL_INVALID_EXTENSION);
    Result = IL_FALSE;
  }
  iUnlockImage(Image);
  return Result;
}

/**
 * Attempts to load an image from a memory buffer.  The file format is specified by the user.
 * @param Type Format of this file.  Acceptable values are IL_BLP, IL_BMP, IL_CUT, IL_DCX, IL_DDS,
 * IL_DICOM, IL_DOOM, IL_DOOM_FLAT, IL_DPX, IL_EXR, IL_FITS, IL_FTX, IL_GIF, IL_HDR, IL_ICO, IL_ICNS,
 * IL_IFF, IL_IWI, IL_JP2, IL_JPG, IL_LIF, IL_MDL, IL_MNG, IL_MP3, IL_PCD, IL_PCX, IL_PIX, IL_PNG,
 * IL_PNM, IL_PSD, IL_PSP, IL_PXR, IL_ROT, IL_SGI, IL_SUN, IL_TEXTURE, IL_TGA, IL_TIF, IL_TPL,
 * IL_UTX, IL_VTF, IL_WAL, IL_WBMP, IL_XPM, IL_RAW, IL_JASC_PAL and IL_TYPE_UNKNOWN.
 * If IL_TYPE_UNKNOWN is specified, ilLoadL will try to determine the type of the file and load it.
 * @param Lump The buffer where the file data is located
 * @param Size Size of the buffer
 * @return Boolean value of failure or success.  Returns IL_FALSE if loading fails.
 * @ingroup file
 */
ILboolean ILAPIENTRY ilLoadL(ILenum Type, const void *Lump, ILuint Size) {
  ILimage *Image;
  ILboolean Result;

  if (Lump == NULL || Size == 0) {
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }

  iLockState();
  Image = iLockCurImage();
  iUnlockState();

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  iSetInputLump(Image, (void*)Lump, Size);
  Result = iLoadFuncs2(Image, Type); 
  iUnlockImage(Image);
  return Result;
}

/**
 * Loads a palette from FileName into the current image's palette.
 * @ingroup file
 */
ILboolean ILAPIENTRY ilLoadPal(ILconst_string FileName) {
  SIMPLE_FUNC(Image, ILboolean, iLoadPal(Image, FileName));
} 

/**
 * Adds an alpha channel if not present and sets it to the given value.
 * @ingroup image_manip
 * @note this is the same as ilSetAlpha
 */
void ILAPIENTRY ilModAlpha(ILdouble AlphaValue) {
  SIMPLE_PROC(Image, iSetAlpha(Image, AlphaValue));
}

/** 
 * Sets the default origin to be used.
 * @param Mode The default origin to use. Valid values are:
 *        - IL_ORIGIN_LOWER_LEFT
 *        - IL_ORIGIN_UPPER_LEFT
 * @ingroup state
 */
ILboolean ILAPIENTRY ilOriginFunc(ILenum Mode) {
  return iOriginFunc(Mode);
}

/**
 * Overlays the image found in Src on top of the current bound image 
 * at the coords specified.
 * @ingroup image_manip
 */
ILboolean ILAPIENTRY ilOverlayImage(ILuint Source, ILint XCoord, ILint YCoord, ILint ZCoord) {
  ILimage * DestImage;
  ILimage * SrcImage;
  ILboolean Result;

  if (iGetCurName() == Source) {
    // can't overly unto self
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }

  iLockState();
  DestImage = iLockCurImage();
  SrcImage  = iLockImage(Source);
  iUnlockState();

  Result    = iOverlayImage(DestImage, SrcImage, XCoord, YCoord, ZCoord);
  iUnlockImage(DestImage);
  iUnlockImage(SrcImage);
  return Result;
}

/**
 * Pops the last entry off the state stack into the current states.
 * @ingroup state
 */
void ILAPIENTRY ilPopAttrib(void) {
  iPopAttrib();
}

/** 
 * Pushes the states indicated by Bits onto the state stack.
 * States not indicated by Bits will be set to their default values.
 * Bits can be a combination of:
 *
 * - IL_ORIGIN_BIT
 * - IL_FORMAT_BIT
 * - IL_TYPE_BIT
 * - IL_FILE_BIT
 * - IL_PAL_BIT
 * - IL_FORMAT_SPECIFIC_BIT
 *
 * @todo Create a version of ilPushAttrib() and ilPopAttrib() that behaves 
 *       more like OpenGL
 * @ingroup state
 */
void ILAPIENTRY ilPushAttrib(ILuint Bits) {
  iPushAttrib(Bits);
}

/**
 * Sets the pixel data format of the currently bound image (no conversion).
 * This is supposed to be called by any loaders registered by ilRegisterLoad.
 * @param Format The format to use, valid values are:
 *               - IL_COLOUR_INDEX
 *               - IL_RGB
 *               - IL_RGBA
 *               - IL_BGR
 *               - IL_BGRA
 *               - IL_LUMINANCE
 *               - IL_LUMINANCE_ALPHA
 * @ingroup register
 */
void ILAPIENTRY ilRegisterFormat(ILenum Format) {
  SIMPLE_PROC(Image, iRegisterFormat(Image, Format));
}

/**
 * Register a file loading function for a file extension.
 * @ingroup register
 */
ILboolean ILAPIENTRY ilRegisterLoad(ILconst_string Ext, IL_LOADPROC Load) {
  ILboolean Result;

  iLockState();
  Result = iRegisterLoad(Ext, Load);
  iUnlockState();
  return Result;
}

/**
 * Preallocates mipmap images for the currently bound image.
 * @ingroup register
 */
ILboolean ILAPIENTRY ilRegisterMipNum(ILuint Num) {
  SIMPLE_FUNC(Image, ILboolean, iRegisterMipNum(Image, Num));
}

/**
 * Preallocates cubemap faces for the currently bound image.
 * @ingroup register
 */
ILboolean ILAPIENTRY ilRegisterNumFaces(ILuint Num) {
  SIMPLE_FUNC(Image, ILboolean, iRegisterNumFaces(Image, Num));
}

/**
 * Preallocates following animation frames for currently bound image.
 * @ingroup register
 */
ILboolean ILAPIENTRY ilRegisterNumImages(ILuint Num) {
  SIMPLE_FUNC(Image, ILboolean, iRegisterNumImages(Image, Num));
}

/**
 * Sets the origin of the currently bound image (no conversion).
 * @param Origin The image of the image, valid values are:
 *               - IL_ORIGIN_LOWER_LEFT
 *               - IL_ORIGIN_UPPER_LEFT
 * @ingroup register
 */
void ILAPIENTRY ilRegisterOrigin(ILenum Origin) {
  SIMPLE_PROC(Image, iRegisterOrigin(Image, Origin));
}

/**
 * Sets the palette data of the currently bound image.
 * @ingroup register
 */
void ILAPIENTRY ilRegisterPal(void *Pal, ILuint Size, ILenum Type) {
  SIMPLE_PROC(Image, iRegisterPal(Image, Pal, Size, Type));
}

/**
 * Register a file saving function for a file extension.
 * @ingroup register
 */
ILboolean ILAPIENTRY ilRegisterSave(ILconst_string Ext, IL_SAVEPROC Save) {
  ILboolean Result;

  iLockState();
  Result = iRegisterSave(Ext, Save);
  iUnlockState();
  return Result;
}

/**
 * Sets the image pixel data type (no conversion).
 * @param Type The data type to use, valid values are:
 *               - IL_BYTE
 *               - IL_UNSIGNED_BYTE
 *               - IL_SHORT
 *               - IL_UNSIGNED_SHORT
 *               - IL_INT
 *               - IL_UNSIGNED_INT
 *               - IL_FLOAT
 *               - IL_DOUBLE
 * @ingroup register
 */
void ILAPIENTRY ilRegisterType(ILenum Type) {
  SIMPLE_PROC(Image, iRegisterType(Image, Type));
}

/**
 * Unregisters a load extension - doesn't have to be called.
 * @ingroup register
 */
ILboolean ILAPIENTRY ilRemoveLoad(ILconst_string Ext) {
  ILboolean Result;

  iLockState();
  Result = iRemoveLoad(Ext);
  iUnlockState();
  return Result;
}

/**
 * Unregisters a save extension - doesn't have to be called.
 * @ingroup register
 */
ILboolean ILAPIENTRY ilRemoveSave(ILconst_string Ext) {
  ILboolean Result;

  iLockState();
  Result = iRemoveSave(Ext);
  iUnlockState();
  return Result;
}

/**
 * Reset the file reading functions of the currently bound image to the
 * default.
 * @ingroup file
 */
void ILAPIENTRY ilResetRead() {
  SIMPLE_PROC(Image, iResetRead(Image));
}

/**
 * Set memory allocation/deallocation functions back to default.
 * @deprecated Use ilSetMemory(NULL, NULL) instead.
 */
void ILAPIENTRY ilResetMemory() {
  ilSetMemory(NULL, NULL);
}

/**
 * Reset the file saving functions of the currently bound image to the
 * default.
 * @ingroup file
 */
void ILAPIENTRY ilResetWrite() {
  SIMPLE_PROC(Image, iResetWrite(Image));
}

/**
 * Attempts to save an image to a file.  The file format is specified by the user.
 * @param Type Format of this file.  Acceptable values are IL_BMP, IL_CHEAD, IL_DDS, IL_EXR,
 * IL_HDR, IL_JP2, IL_JPG, IL_PCX, IL_PNG, IL_PNM, IL_PSD, IL_RAW, IL_SGI, IL_TGA, IL_TIF,
 * IL_VTF, IL_WBMP and IL_JASC_PAL.
 * @param FileName Ansi or Unicode string, depending on the compiled version of DevIL, that gives
 *        the filename to save to.
 * @return Boolean value of failure or success.  Returns IL_FALSE if saving failed.
 * @ingroup file
 */
ILboolean ILAPIENTRY ilSave(ILenum Type, ILconst_string FileName) {
  SIMPLE_FUNC(Image, ILboolean, iSave(Image, Type, FileName));
}

/**
 * Save the current image to FileName as raw data.
 * @ingroup file
 */
ILboolean ILAPIENTRY ilSaveData(ILconst_string FileName) {
  SIMPLE_FUNC(Image, ILboolean, iSaveData(Image, FileName));  
}

/**
 * Attempts to save an image to a file stream.  The file format is specified by the user.
 * @param Type Format of this file.  Acceptable values are all supported image and palette type values, eg.
 * IL_BMP, IL_CHEAD, IL_DDS, IL_EXR, etc. If the given type is a palette type only the
 * palette is saved.
 * @param File File stream to save to.
 * @return Boolean value of failure or success.  Returns IL_FALSE if saving failed.
 * @ingroup file
 */
ILuint ILAPIENTRY ilSaveF(ILenum Type, ILHANDLE File) {
  ILboolean Result;
  ILimage *Image;

  iLockState();
  Image = iLockCurImage();
  iUnlockState();

  iSetOutputFile(Image, File);
  Result = iSaveFuncs2(Image, Type);
  iUnlockImage(Image);
  return Result;
}

/**
 * Attempts to load an image using the currently set IO functions. 
 * The file format is specified by the user.
 * @param type The image/palette file type to save.
 * @ingroup file
 * @see ilSetWrite
 */
ILAPI ILboolean ILAPIENTRY ilSaveFuncs(ILenum type) {
  SIMPLE_FUNC(Image, ILboolean, iSaveFuncs2(Image, type));
}

/**
 * Saves the current image based on the extension given in FileName.
 * @param FileName Ansi or Unicode string, depending on the compiled version of DevIL, that gives
 *        the filename to save to.
 * @return Boolean value of failure or success.  Returns IL_FALSE if saving failed.
 * @ingroup file
 */
ILboolean ILAPIENTRY ilSaveImage(ILconst_string FileName) {
  SIMPLE_FUNC(Image, ILboolean, iSaveImage(Image, FileName)); 
}

/**
 * Attempts to save an image to a memory buffer.  
 * The file format is specified by the user.
 * @param Type Format of this image file.  Acceptable values are IL_BMP, IL_CHEAD, IL_DDS, IL_EXR,
 * IL_HDR, IL_JP2, IL_JPG, IL_PCX, IL_PNG, IL_PNM, IL_PSD, IL_RAW, IL_SGI, IL_TGA, IL_TIF,
 * IL_VTF, IL_WBMP and IL_JASC_PAL.
 * @param Lump Memory buffer to save to
 * @param Size Size of the memory buffer
 * @return The number of bytes written to the lump, or 0 in case of failure
 * @ingroup file
 */
ILuint ILAPIENTRY ilSaveL(ILenum Type, void *Lump, ILuint Size) {
  ILimage *Image;
  ILint64 pos1, pos2;
  ILboolean bRet;

  iLockState();
  Image = iLockCurImage();
  iUnlockState();

  iSetOutputLump(Image, Lump, Size);
  pos1 = SIOtell(&Image->io);
  bRet = iSaveFuncs2(Image, Type);
  pos2 = SIOtell(&Image->io);

  iUnlockImage(Image);

  if (bRet)
    return (ILuint)(pos2-pos1);  // Return the number of bytes written.
  else
    return 0;  // Error occurred
}

/**
 * Saves a palette from the current image's palette to a file.
 * @ingroup file
 */
ILboolean ILAPIENTRY ilSavePal(ILconst_string FileName) {
  SIMPLE_FUNC(Image, ILboolean, iSavePal(Image, FileName));
} 

/**
 * Adds an alpha channel if not present and sets it to the given value.
 * @ingroup image_manip
 * @note this is the same as ilModAlpha
 */
ILboolean ILAPIENTRY ilSetAlpha(ILdouble AlphaValue) {
  SIMPLE_FUNC(Image, ILboolean, iSetAlpha(Image, AlphaValue));
}

/**
 * Uploads Data of the same size to replace the current image's data.
 * @param Data New image data to update the currently bound image
 * @exception IL_ILLEGAL_OPERATION No currently bound image
 * @exception IL_INVALID_PARAM Data was NULL.
 * @return Boolean value of failure or success
 * @ingroup image_manip
 */
ILboolean ILAPIENTRY ilSetData(void *Data) {
  SIMPLE_FUNC(Image, ILboolean, iSetData(Image, Data));
}

/** 
 * Set the duration of the currently bound image.
 * Only useful if the image is part of an animation chain.
 * @ingroup data
 */
ILboolean ILAPIENTRY ilSetDuration(ILuint Duration) {
  SIMPLE_FUNC(Image, ILboolean, iSetDuration(Image, Duration));
}

/**
 * Sets a parameter value for a @a Mode
 * @see ilGetFloat for a list of valid @a Modes.
 * @ingroup state
 */
void ILAPIENTRY ilSetFloat(ILenum Mode, ILfloat Param) {
  ILimage *Image;

  iLockState();
  Image = iLockCurImage();
  iSetfv(Image, Mode, &Param);
  iUnlockImage(Image);
  iUnlockState();
}

/**
 * Sets a parameter value for a @a Mode
 * @see ilGetFloat for a list of valid @a Modes.
 * @ingroup state
 */
void ILAPIENTRY ilSetFloatv(ILenum Mode, ILfloat *Param) {
  ILimage *Image;

  iLockState();
  Image = iLockCurImage();
  iSetfv(Image, Mode, Param);
  iUnlockImage(Image);
  iUnlockState();
}

/**
 * Sets a parameter value for a @a Mode
 * @see ilGetInteger for a list of valid @a Modes.
 * @ingroup state
 */
void ILAPIENTRY ilSetInteger(ILenum Mode, ILint Param) {
  ILimage *Image;

  iLockState();
  Image = iLockCurImage();
  iSetiv(Image, Mode, &Param);
  iUnlockImage(Image);
  iUnlockState();
}

/**
 * Sets a parameter value for a @a Mode
 * @see ilGetInteger for a list of valid @a Modes.
 * @ingroup state
 */
void ILAPIENTRY ilSetIntegerv(ILenum Mode, ILint *Param) {
  ILimage *Image;

  iLockState();
  Image = iLockCurImage();
  iSetiv(Image, Mode, Param);
  iUnlockImage(Image);
  iUnlockState();
}

/**
 * Sets the memory allocation and deallocation functions.
 *
 * When changing the @a freeFunc all allocated memory up to that point
 * will still be freed by the function that was set when that memory was 
 * allocated. This means the correct function will be called for every
 * allocated object.
 * 
 * @param  mallocFunc The function to call to allocate memory or NULL to reset to the default.
 * @param  freeFunc   The function to call to free memory or NULL to reset to the default.
 * @ingroup setup
 */
void ILAPIENTRY ilSetMemory(mAlloc mallocFunc, mFree freeFunc) {
  iLockState();
  iSetMemory(mallocFunc, freeFunc);
  iUnlockState();
}

/** 
 * Set an image meta tag.
 */
ILAPI ILboolean ILAPIENTRY ilSetMetadata(ILenum IFD, ILenum ID, ILenum Type, ILuint Count, ILuint Size, const void *Data) {
  SIMPLE_FUNC(Image, ILboolean, iSetMetadata(Image, IFD, ID, Type, Count, Size, Data));
}

/**
 * Set the pixels in a region of the currently bound image.
 * @ingroup image_manip
 */
void ILAPIENTRY ilSetPixels(ILint XOff, ILint YOff, ILint ZOff, ILuint Width, ILuint Height, ILuint Depth, ILenum Format, ILenum Type, void *Data) {
  SIMPLE_PROC(Image, iSetPixels(Image, XOff, YOff, ZOff, Width, Height, Depth, Format, Type, Data));
}

/** 
 * Allows you to override the default file-reading functions.
 * @ingroup file
 */
ILboolean ILAPIENTRY ilSetRead(fOpenProc aOpen, fCloseProc aClose, fEofProc aEof, fGetcProc aGetc, 
  fReadProc aRead, fSeekProc aSeek, fTellProc aTell)
{
  ILimage *Image;

  iLockState();
  Image = iLockCurImage();
  iUnlockState();

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (!aEof || !aGetc || !aRead || !aSeek || !aTell) {
    iSetError(IL_INVALID_VALUE);
    iUnlockImage(Image);
    return IL_FALSE;
  }

  iSetRead(Image, aOpen, aClose, aEof, aGetc, aRead, aSeek, aTell);
  iUnlockImage(Image);
  return IL_TRUE;
}

/**
 * Sets a string detailing aspects about this library.
 * @param StringName Name of string to set.
 * @param String     New string value, will be automatically converted to
 *                   ILchar if necessary.
 * @ingroup state
 */
void ILAPIENTRY ilSetString(ILenum StringName, ILconst_string String) {
  SIMPLE_PROC(Image, iSetString(Image, StringName, String));
}

/**
 * Allows you to override the default file-writing functions.
 * @ingroup file
 */
ILboolean ILAPIENTRY ilSetWrite(fOpenProc Open, fCloseProc Close, fPutcProc Putc, fSeekProc Seek, 
  fTellProc Tell, fWriteProc Write)
{
  ILimage *Image;

  iLockState();
  Image = iLockCurImage();
  iUnlockState();

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (!Putc || !Write || !Seek || !Tell) {
    iSetError(IL_INVALID_VALUE);
    iUnlockImage(Image);
    return IL_FALSE;
  }

  iSetWrite(Image, Open, Close, Putc, Seek, Tell, Write);
  iUnlockImage(Image);
  return IL_TRUE;
}

/**
 * Shuts down the image library.
 * @ingroup setup
 */
void ILAPIENTRY ilShutDown(void) {
  iShutDownIL();
}

/**
 * Compresses the currently bound image with DXTC and store the
 * result in its internal DXTC data buffer.
 * @ingroup data
 */
ILAPI ILboolean ILAPIENTRY ilSurfaceToDxtcData(ILenum Format) {
  SIMPLE_FUNC(Image, ILboolean, iSurfaceToDxtcData(Image, Format));
}

/**
 * Changes the current bound image to use these new dimensions (current data is destroyed).
 * @param Width Specifies the new image width.  This cannot be 0.
 * @param Height Specifies the new image height.  This cannot be 0.
 * @param Depth Specifies the new image depth.  This cannot be 0.
 * @param Bpp Number of channels (ex. 3 for RGB)
 * @param Format Enum of the desired format.  Any format values are accepted.
 * @param Type Enum of the desired type.  Any type values are accepted.
 * @param Data Specifies data that should be copied to the new image. If this parameter is NULL, no data is copied, and the new image data consists of undefined values.
 * @exception IL_ILLEGAL_OPERATION No currently bound image.
 * @exception IL_INVALID_PARAM One of the parameters is incorrect, such as one of the dimensions being 0.
 * @exception IL_OUT_OF_MEMORY Could not allocate enough memory.
 * @return Boolean value of failure or success
 * @ingroup image_manip
 */
ILboolean ILAPIENTRY ilTexImage(ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, void *Data) {
  SIMPLE_FUNC(Image, ILboolean, iTexImage(Image, Width, Height, Depth, Bpp, Format, Type, Data));
}

/**
 * Sets the currently bound image data. Like ilTexImage but from 
 * DXTC compressed data.
 * @param Width Specifies the new image width.  This cannot be 0.
 * @param Height Specifies the new image height.  This cannot be 0.
 * @param Depth Specifies the new image depth.  This cannot be 0.
 * @param DxtFormat Describes the DXT compression data format.
 * @param Data The DTX compressed pixel data.
 * @ingroup image_manip
 */
ILAPI ILboolean ILAPIENTRY ilTexImageDxtc(ILuint Width, ILuint Height, ILuint Depth, ILenum DxtFormat, const ILubyte* Data) {
  SIMPLE_FUNC(Image, ILboolean, iTexImageDxtc(Image, Width, Height, Depth, DxtFormat, Data));
}

/**
 * Get the image type for a given file name by its extension.
 * @ingroup file
 */
ILenum ILAPIENTRY ilTypeFromExt(ILconst_string FileName) {
  return iTypeFromExt(FileName);
}

/**
 * Sets the default type to be used.
 * @ingroup state
 */
ILboolean ILAPIENTRY ilTypeFunc(ILenum Mode) {
  return iTypeFunc(Mode);
}

/** @} */
