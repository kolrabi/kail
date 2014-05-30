//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Last modified: 2014-05-30
//
// Filename: src-IL/src/il_api.c
//
//-----------------------------------------------------------------------------

/**
 * @file
 * @brief Contains public IL entry functions.
 *
 * Just calls the internal versions of the functions for now.
 *
 * @defgroup state Global State
 * @defgroup setup Initialization / Deinitalization
 * @defgroup image Image Functions
 */

#include "il_internal.h"
#include "il_stack.h"
#include "il_states.h"
#include "il_alloc.h"

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
  iBindImage(Image);
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
 * Enables a mode.
 * Valid modes are:
 *
 * Mode                   | Default       | Description
 * ---------------------- | ------------- | ---------------------------------------------
 * @a IL_ORIGIN_SET       | @a IL_FALSE   | Flip image on load to match @a IL_ORIGIN_MODE.
 * @a IL_FORMAT_SET       | @a IL_FALSE   | Convert image format on load to match the format set by ilFormatFunc().
 * @a IL_TYPE_SET         | @a IL_FALSE   | Convert image data type on load to match @a IL_TYPE_MODE.
 * @a IL_CONV_PAL         | @a IL_FALSE   | Convert images that use palettes on load to 24 bit RGBA.
 * @a IL_NVIDIA_COMPRESS  | @a IL_FALSE   | Use NVIDIA texture tools for compressing DXT formats if available.
 * @a IL_SQUISH_COMPRESS  | @a IL_FALSE   | Use libsquish for compressing DXT formats if available.
 * 
 * @param  Mode Mode to enable
 * @return      IL_TRUE if successful.
 * @see         ilIsEnabled
 * @ingroup state
 */
ILboolean ILAPIENTRY ilEnable(ILenum Mode)
{
  return iAble(Mode, IL_TRUE);
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
 * Sets @a Param equal to the current value of the @a Mode.
 * @ingroup state
 */
void ILAPIENTRY ilGetBooleanv(ILenum Mode, ILboolean *Param)
{
  if (Param == NULL) {
    iSetError(IL_INVALID_PARAM);
    return;
  }
  *Param = iGetInteger(Mode);
}


/**
 * Returns the current value of the @a Mode.
 * @ingroup state
 */
ILboolean ILAPIENTRY ilGetBoolean(ILenum Mode) {
  return iGetInteger(Mode);
}


/** 
 * Gets the last error on the error stack
 * @return  An enum describing the last error;
 */
ILenum ILAPIENTRY ilGetError(void) {
  return iGetError();
}


/**
 * Sets @a Param equal to the current value of the @a Mode.
 * @see ilGetInteger
 * @ingroup state
 */
void ILAPIENTRY ilGetIntegerv(ILenum Mode, ILint *Param) {
  if (Param == NULL) {
    iSetError(IL_INVALID_PARAM);
    return;
  }
  *Param = iGetInteger(Mode);
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
 * IL_TGA_CREATE_STAMP    | RW
 * IL_TGA_RLE             | RW  | IL_FALSE              | Use RLE when writing Targa files.
 * IL_VTF_COMP            | RW  | IL_DXT_NO_COMP        | Compression to use when writing VTF files, can be IL_DXT_NO_COMP, IL_DXT1, IL_DXT3, IL_DXT4, IL_DXT5.
 *
 * @see ilGetIntegerImage for image specific modes.
 * @ingroup state
 */
ILint ILAPIENTRY ilGetInteger(ILenum Mode) {
  return iGetInteger(Mode);
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
 * @ingroup image
 */
ILint ILAPIENTRY ilGetIntegerImage(ILuint Image, ILenum Mode) {
  ILimage *image = iGetImage(Image);
  if (!image) {
    iSetError(IL_INVALID_PARAM);
    return 0;
  }

  return iGetIntegerImage(image, Mode);
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
 * @a IL_TGA_ID_STRING            | RW  | Identifier string to be used when writing Targa image files.
 * @a IL_TGA_AUTHNAME_STRING      | RW  | Author name to be used when writing Targa image files.
 * @a IL_TGA_AUTHCOMMENT_STRING   | RW  | Author comment to be used when writing Targa image files.
 * @a IL_PNG_AUTHNAME_STRING      | RW  | Author name to be used when writing PNG image files.
 * @a IL_PNG_TITLE_STRING         | RW  | Image title to be used when writing PNG image files.
 * @a IL_PNG_DESCRIPTION_STRING   | RW  | Image description to be used when writing PNG image files.
 * @a IL_TIF_DESCRIPTION_STRING   | RW  | Image description to be used when writing TIFF image files.
 * @a IL_TIF_HOSTCOMPUTER_STRING  | RW  | Name of host computer to be used when writing TIFF image files.
 * @a IL_TIF_DOCUMENTNAME_STRING  | RW  | Document name to be used when writing TIFF image files.
 * @a IL_TIF_AUTHNAME_STRING      | RW  | Author name to be used when writing TIFF image files.
 * @a IL_CHEAD_HEADER_STRING      | RW  | Variable name to use when writing C headers.
 *
 * Strings marked with RW can also be set using ilSetString();
 * 
 * @param  StringName String to get.
 * @ingroup state
 */
ILconst_string ILAPIENTRY ilGetString(ILenum StringName) {
  return iGetILString(StringName);
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
 * Initialize the image library.
 * This must be called before calling any other IL functions or their behaviour
 * is undefined.
 * @ingroup setup
 */
void ILAPIENTRY ilInit(void) {
  iInitIL();
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
 * @ingroup image
 */
ILboolean ILAPIENTRY ilIsImage(ILuint Image) {
  return iIsImage(Image);
}

/**
 * Sets a parameter value for a @a Mode
 * @see ilGetInteger for a list of valid @a Modes.
 * @ingroup state
 */
void ILAPIENTRY ilSetInteger(ILenum Mode, ILint Param) {
  iSetInteger(Mode, Param);
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
  iSetMemory(mallocFunc, freeFunc);
}

/**
 * Sets a string detailing aspects about this library.
 * @param StringName Name of string to set.
 * @param String     New string value, will be automatically converted to
 *                   ILchar if necessary.
 * @ingroup state
 */
void ILAPIENTRY ilSetString(ILenum StringName, const char *String) {
  return iSetString(StringName, String);
}

/**
 * Shuts down the image library.
 * @ingroup setup
 */
void ILAPIENTRY ilShutDown(void) {
  iShutDownIL();
}