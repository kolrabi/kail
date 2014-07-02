//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_states.c
//
// Description: IL_STATE_STRUCT machine
//
//
// 20040223 XIX : now has a ilPngAlphaIndex member, so we can spit out png files with a transparent index, set to -1 for none
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_states.h"
#include "il_stack.h"
//#include <malloc.h>
#include <stdlib.h>

ILint iMetaToInt(ILimage *, ILenum, ILenum);

// Global variables
static ILconst_string _ilVendor    = IL_TEXT("kolrabi");
static ILconst_string _ilVersion   = IL_TEXT("kolrabi's another Image Library (kaIL) 1.9.0");

static IL_STATE_STRUCT *iGetStateStruct() {
  return &iGetTLSData()->CurState;
}

extern ILchar *_ilLoadExt;
extern ILchar *_ilSaveExt;

//! Set all states to their defaults.
void ilDefaultStates() {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;

  ilStates[ilCurrentPos].ilOriginSet = IL_FALSE;
  ilStates[ilCurrentPos].ilOriginMode = IL_ORIGIN_LOWER_LEFT;
  ilStates[ilCurrentPos].ilFormatSet = IL_FALSE;
  ilStates[ilCurrentPos].ilFormatMode = IL_BGRA;
  ilStates[ilCurrentPos].ilTypeSet = IL_FALSE;
  ilStates[ilCurrentPos].ilTypeMode = IL_UNSIGNED_BYTE;
  ilStates[ilCurrentPos].ilOverWriteFiles = IL_FALSE;
  ilStates[ilCurrentPos].ilAutoConvPal = IL_FALSE;
  ilStates[ilCurrentPos].ilDefaultOnFail = IL_FALSE;
  ilStates[ilCurrentPos].ilUseKeyColour = IL_FALSE;
  ilStates[ilCurrentPos].ilKeyColourRed = 0;
  ilStates[ilCurrentPos].ilKeyColourGreen = 0;
  ilStates[ilCurrentPos].ilKeyColourBlue = 0;
  ilStates[ilCurrentPos].ilKeyColourAlpha = 0; 
  ilStates[ilCurrentPos].ilBlitBlend = IL_TRUE;
  ilStates[ilCurrentPos].ilCompression = IL_COMPRESS_ZLIB;
  ilStates[ilCurrentPos].ilInterlace = IL_FALSE;

  ilStates[ilCurrentPos].ilTgaCreateStamp = IL_FALSE;
  ilStates[ilCurrentPos].ilJpgQuality = 99;
  ilStates[ilCurrentPos].ilPngInterlace = IL_FALSE;
  ilStates[ilCurrentPos].ilTgaRle = IL_FALSE;
  ilStates[ilCurrentPos].ilBmpRle = IL_FALSE;
  ilStates[ilCurrentPos].ilSgiRle = IL_FALSE;
  ilStates[ilCurrentPos].ilJpgFormat = IL_JFIF;
  ilStates[ilCurrentPos].ilJpgProgressive = IL_FALSE;
  ilStates[ilCurrentPos].ilDxtcFormat = IL_DXT1;
  ilStates[ilCurrentPos].ilPcdPicNum = 2;
  ilStates[ilCurrentPos].ilPngAlphaIndex = -1;
  ilStates[ilCurrentPos].ilVtfCompression = IL_DXT_NO_COMP;

  ilStates[ilCurrentPos].ilTgaId = NULL;
  ilStates[ilCurrentPos].ilTgaAuthName = NULL;
  ilStates[ilCurrentPos].ilTgaAuthComment = NULL;
  ilStates[ilCurrentPos].ilPngAuthName = NULL;
  ilStates[ilCurrentPos].ilPngTitle = NULL;
  ilStates[ilCurrentPos].ilPngDescription = NULL;

  //2003-09-01: added tiff strings
  ilStates[ilCurrentPos].ilTifDescription = NULL;
  ilStates[ilCurrentPos].ilTifHostComputer = NULL;
  ilStates[ilCurrentPos].ilTifDocumentName = NULL;
  ilStates[ilCurrentPos].ilTifAuthName = NULL;
  ilStates[ilCurrentPos].ilCHeader = NULL;

  ilStates[ilCurrentPos].ilQuantMode = IL_WU_QUANT;
  ilStates[ilCurrentPos].ilNeuSample = 15;
  ilStates[ilCurrentPos].ilQuantMaxIndexs = 256;

  ilStates[ilCurrentPos].ilKeepDxtcData = IL_FALSE;
  ilStates[ilCurrentPos].ilUseNVidiaDXT = IL_FALSE;
  ilStates[ilCurrentPos].ilUseSquishDXT = IL_FALSE;

  ilStates[ilCurrentPos].ilImageSelectionMode = IL_RELATIVE;

  StateStruct->ilHints.MemVsSpeedHint = IL_FASTEST;
  StateStruct->ilHints.CompressHint   = IL_USE_COMPRESSION;

  // clear errors
  while (ilGetError() != IL_NO_ERROR)
    ;

  return;
}


//! Returns a constant string detailing aspects about this library.
ILconst_string iGetILString(ILenum StringName) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;

  switch (StringName) {
    case IL_VENDOR:
      return (ILconst_string)_ilVendor;
    case IL_VERSION_NUM: //changed 2003-08-30: IL_VERSION changes                 //switch define ;-)
      return (ILconst_string)_ilVersion;
    case IL_LOAD_EXT:
      return (ILconst_string)_ilLoadExt;
    case IL_SAVE_EXT:
      return (ILconst_string)_ilSaveExt;
    case IL_TGA_ID_STRING:
      return (ILconst_string)ilStates[ilCurrentPos].ilTgaId;
    case IL_TGA_AUTHNAME_STRING:
      return (ILconst_string)ilStates[ilCurrentPos].ilTgaAuthName;
    case IL_TGA_AUTHCOMMENT_STRING:
      return (ILconst_string)ilStates[ilCurrentPos].ilTgaAuthComment;
    case IL_PNG_AUTHNAME_STRING:
      return (ILconst_string)ilStates[ilCurrentPos].ilPngAuthName;
    case IL_PNG_TITLE_STRING:
      return (ILconst_string)ilStates[ilCurrentPos].ilPngTitle;
    case IL_PNG_DESCRIPTION_STRING:
      return (ILconst_string)ilStates[ilCurrentPos].ilPngDescription;
    //2003-08-31: added tif strings
    case IL_TIF_DESCRIPTION_STRING:
      return (ILconst_string)ilStates[ilCurrentPos].ilTifDescription;
    case IL_TIF_HOSTCOMPUTER_STRING:
      return (ILconst_string)ilStates[ilCurrentPos].ilTifHostComputer;
    case IL_TIF_DOCUMENTNAME_STRING:
      return (ILconst_string)ilStates[ilCurrentPos].ilTifDocumentName;
    case IL_TIF_AUTHNAME_STRING:
      return (ILconst_string)ilStates[ilCurrentPos].ilTifAuthName;
    case IL_CHEAD_HEADER_STRING:
      return (ILconst_string)ilStates[ilCurrentPos].ilCHeader;
    default:
      iSetError(IL_INVALID_ENUM);
      break;
  }
  return NULL;
}


// Clips a string to a certain length and returns a new string.
char *iClipString(ILconst_string String_, ILuint MaxLen) {
  ILuint  Length;

#ifdef _UNICODE
  char *Clipped = iMultiByteFromWide(String_);
#else
  char *Clipped = iCharStrDup(String_);
#endif

  if (Clipped == NULL)
    return NULL;

  Length = iCharStrLen(Clipped);
  if (Length >= MaxLen)
    Clipped[MaxLen] = 0;
  return Clipped;
}


// Returns format-specific strings, converted to UTF8 truncated to MaxLen (not counting the terminating NUL).
char *iGetString(ILenum StringName) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;

  switch (StringName)   {
    case IL_TGA_ID_STRING:
      return iClipString(ilStates[ilCurrentPos].ilTgaId, 254);
    case IL_TGA_AUTHNAME_STRING:
      return iClipString(ilStates[ilCurrentPos].ilTgaAuthName, 40);
    case IL_TGA_AUTHCOMMENT_STRING:
      return iClipString(ilStates[ilCurrentPos].ilTgaAuthComment, 80);
    case IL_PNG_AUTHNAME_STRING:
      return iClipString(ilStates[ilCurrentPos].ilPngAuthName, 255);
    case IL_PNG_TITLE_STRING:
      return iClipString(ilStates[ilCurrentPos].ilPngTitle, 255);
    case IL_PNG_DESCRIPTION_STRING:
      return iClipString(ilStates[ilCurrentPos].ilPngDescription, 255);

    //changed 2003-08-31...here was a serious copy and paste bug ;-)
    case IL_TIF_DESCRIPTION_STRING:
      return iClipString(ilStates[ilCurrentPos].ilTifDescription, 255);
    case IL_TIF_HOSTCOMPUTER_STRING:
      return iClipString(ilStates[ilCurrentPos].ilTifHostComputer, 255);
    case IL_TIF_DOCUMENTNAME_STRING:
      return iClipString(ilStates[ilCurrentPos].ilTifDocumentName, 255);
    case IL_TIF_AUTHNAME_STRING:
      return iClipString(ilStates[ilCurrentPos].ilTifAuthName, 255);
    case IL_CHEAD_HEADER_STRING:
      return iClipString(ilStates[ilCurrentPos].ilCHeader, 32);
    default:
      iSetError(IL_INVALID_ENUM);
  }
  return NULL;
}

// Internal function that sets the Mode equal to Flag
ILboolean iAble(ILenum Mode, ILboolean Flag) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;

  switch (Mode) {
    case IL_ORIGIN_SET:
      ilStates[ilCurrentPos].ilOriginSet = Flag;
      break;
    case IL_FORMAT_SET:
      ilStates[ilCurrentPos].ilFormatSet = Flag;
      break;
    case IL_TYPE_SET:
      ilStates[ilCurrentPos].ilTypeSet = Flag;
      break;
    case IL_FILE_MODE: 
    case IL_FILE_OVERWRITE: // deprecated: unused, IL_FILE_MODE is used instead
      ilStates[ilCurrentPos].ilOverWriteFiles = Flag;
      break;
    case IL_CONV_PAL:
      ilStates[ilCurrentPos].ilAutoConvPal = Flag;
      break;
    case IL_DEFAULT_ON_FAIL:
      ilStates[ilCurrentPos].ilDefaultOnFail = Flag;
      break;
    case IL_USE_KEY_COLOUR:
      ilStates[ilCurrentPos].ilUseKeyColour = Flag;
      break;
    case IL_BLIT_BLEND:
      ilStates[ilCurrentPos].ilBlitBlend = Flag;
      break;
    case IL_SAVE_INTERLACED:
      ilStates[ilCurrentPos].ilInterlace = Flag;
      break;
    case IL_JPG_PROGRESSIVE:
      ilStates[ilCurrentPos].ilJpgProgressive = Flag;
      break;
    case IL_NVIDIA_COMPRESS:
      ilStates[ilCurrentPos].ilUseNVidiaDXT = Flag;
      break;
    case IL_SQUISH_COMPRESS:
      ilStates[ilCurrentPos].ilUseSquishDXT = Flag;
      break;

    default:
      iSetError(IL_INVALID_ENUM);
      return IL_FALSE;
  }

  return IL_TRUE;
}


//! Checks whether the mode is enabled.
ILboolean iIsEnabled(ILenum Mode) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;

  switch (Mode)   {
    case IL_ORIGIN_SET:
      return ilStates[ilCurrentPos].ilOriginSet;
    case IL_FORMAT_SET:
      return ilStates[ilCurrentPos].ilFormatSet;
    case IL_TYPE_SET:
      return ilStates[ilCurrentPos].ilTypeSet;
    case IL_FILE_MODE: 
    case IL_FILE_OVERWRITE:
      return ilStates[ilCurrentPos].ilOverWriteFiles;
    case IL_CONV_PAL:
      return ilStates[ilCurrentPos].ilAutoConvPal;
    case IL_DEFAULT_ON_FAIL:
      return ilStates[ilCurrentPos].ilDefaultOnFail;
    case IL_USE_KEY_COLOUR:
      return ilStates[ilCurrentPos].ilUseKeyColour;
    case IL_BLIT_BLEND:
      return ilStates[ilCurrentPos].ilBlitBlend;
    case IL_SAVE_INTERLACED:
      return ilStates[ilCurrentPos].ilInterlace;
    case IL_JPG_PROGRESSIVE:
      return ilStates[ilCurrentPos].ilJpgProgressive;
    case IL_NVIDIA_COMPRESS:
      return ilStates[ilCurrentPos].ilUseNVidiaDXT;
    case IL_SQUISH_COMPRESS:
      return ilStates[ilCurrentPos].ilUseSquishDXT;

    default:
      iSetError(IL_INVALID_ENUM);
  }

  return IL_FALSE;
}


//! Internal function to figure out where we are in an image chain.
ILuint iGetActiveNum(ILenum Type) {
  IL_TLS_DATA *TLSData = iGetTLSData();

  switch (Type) {
    case IL_ACTIVE_IMAGE:
      return TLSData->CurSel.CurFrame;

    case IL_ACTIVE_MIPMAP:
      return TLSData->CurSel.CurMipmap;

    case IL_ACTIVE_LAYER:
      return TLSData->CurSel.CurLayer;

    case IL_ACTIVE_FACE:
      return TLSData->CurSel.CurFace;
  }

  //@TODO: Any error needed here?

  return 0;
}

ILboolean ILAPIENTRY iGetIntegerImageV(ILimage *Image, ILenum Mode, ILint *Param) {
  ILimage *SubImage;
  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  switch (Mode) {
    case IL_DXTC_DATA_FORMAT:
      if (Image->DxtcData == NULL || Image->DxtcSize == 0) {
        *Param = IL_DXT_NO_COMP;
      } else {
        *Param = Image->DxtcFormat;
      }
      return IL_TRUE;

    case IL_NUM_FACES: 
      *Param = 0;
      for (SubImage = Image->Faces; SubImage; SubImage = SubImage->Faces) (*Param)++;
      return IL_TRUE;

    case IL_NUM_IMAGES: 
      *Param = 0;
      for (SubImage = Image->Next; SubImage; SubImage = SubImage->Next) (*Param)++;
      return IL_TRUE;

    case IL_NUM_LAYERS: 
      *Param = 0;
      for (SubImage = Image->Layers; SubImage; SubImage = SubImage->Layers) (*Param)++;
      return IL_TRUE;

    case IL_NUM_MIPMAPS:
      *Param = 0;
      for (SubImage = Image->Mipmaps; SubImage; SubImage = SubImage->Mipmaps) (*Param)++;
      return IL_TRUE;

    //changed 20040610 to channel count (Bpp) times Bytes per channel
    case IL_IMAGE_BITS_PER_PIXEL:         *Param = (Image->Bpp << 3)*Image->Bpc; return IL_TRUE;

    //changed 20040610 to channel count (Bpp) times Bytes per channel
    case IL_IMAGE_BYTES_PER_PIXEL:        *Param = Image->Bpp*Image->Bpc; return IL_TRUE;

    case IL_IMAGE_BPC:          *Param = Image->Bpc; return IL_TRUE;
    case IL_IMAGE_CHANNELS:     *Param = Image->Bpp; return IL_TRUE;
    case IL_IMAGE_CUBEFLAGS:    *Param = Image->CubeFlags; return IL_TRUE;
    case IL_IMAGE_DEPTH:        *Param = Image->Depth; return IL_TRUE;
    case IL_IMAGE_DURATION:     *Param = Image->Duration; return IL_TRUE;
    case IL_IMAGE_FORMAT:       *Param = Image->Format; return IL_TRUE;
    case IL_IMAGE_HEIGHT:       *Param = Image->Height; return IL_TRUE;
    case IL_IMAGE_SIZE_OF_DATA: *Param = Image->SizeOfData; return IL_TRUE;
    case IL_IMAGE_OFFX:         *Param = Image->OffX; return IL_TRUE;
    case IL_IMAGE_OFFY:         *Param = Image->OffY; return IL_TRUE;
    case IL_IMAGE_ORIGIN:       *Param = Image->Origin; return IL_TRUE;
    case IL_IMAGE_PLANESIZE:    *Param = Image->SizeOfPlane; return IL_TRUE;
    case IL_IMAGE_TYPE:         *Param = Image->Type; return IL_TRUE;
    case IL_IMAGE_WIDTH:        *Param = Image->Width; return IL_TRUE;
    case IL_PALETTE_TYPE:       *Param = Image->Pal.PalType; return IL_TRUE;
    case IL_PALETTE_BPP:        *Param = iGetBppPal(Image->Pal.PalType); return IL_TRUE;
    case IL_PALETTE_BASE_TYPE:  *Param = iGetPalBaseType(Image->Pal.PalType); return IL_TRUE;
    case IL_PALETTE_NUM_COLS:
      if (!Image->Pal.Palette || !Image->Pal.PalSize || Image->Pal.PalType == IL_PAL_NONE)
        *Param = 0;
      else 
        *Param = Image->Pal.PalSize / iGetBppPal(Image->Pal.PalType);
      return IL_TRUE;

    case IL_IMAGE_METADATA_COUNT: {
        ILexif *Exif = Image->ExifTags;
        *Param = 0;
        while(Exif) { (*Param)++; Exif = Exif->Next; }
        return IL_TRUE;
      } 

    case IL_META_EXIF_VERSION:      *Param = iMetaToInt(Image, IL_TIFF_IFD_EXIF, 0x9000); return IL_TRUE;
    case IL_META_EXPOSURE_PROGRAM:  *Param = iMetaToInt(Image, IL_TIFF_IFD_EXIF, 0x8822); return IL_TRUE;
    case IL_META_METERING_MODE:     *Param = iMetaToInt(Image, IL_TIFF_IFD_EXIF, 0x9207); return IL_TRUE;
    case IL_META_LIGHT_SOURCE:      *Param = iMetaToInt(Image, IL_TIFF_IFD_EXIF, 0x9208); return IL_TRUE;
    case IL_META_FLASH:             *Param = iMetaToInt(Image, IL_TIFF_IFD_EXIF, 0x9209); return IL_TRUE;
    case IL_META_GPS_VERSION:       *Param = iMetaToInt(Image, IL_TIFF_IFD_GPS,  0x0000); return IL_TRUE;
    case IL_META_GPS_DIFFERENTIAL:  *Param = iMetaToInt(Image, IL_TIFF_IFD_GPS,  0x001E); return IL_TRUE;

    default:
      iSetError(IL_INVALID_ENUM);
    }
  return IL_FALSE;
}

//@TODO rename to ilGetImageIntegerv for 1.6.9 and make it public
//! Sets Param equal to the current value of the Mode
ILint ILAPIENTRY iGetIntegerImage(ILimage *Image, ILenum Mode)
{
    ILint Param = 0;

    switch (Mode) {
      // TODO: block modes that have multiple values
      case IL_META_EXPOSURE_TIME:
        iSetError(IL_INVALID_ENUM);
        return 0;
    }

    iGetIntegerImageV(Image, Mode, &Param);
    return Param;
}

ILboolean iGetIntegerV(ILenum Mode, ILint *Param) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_IMAGE_SELECTION *Selection = &iGetTLSData()->CurSel;
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;
  ILimage *CurImage = iGetSelectedImage(Selection);
  ILimage *BaseImage = iGetBaseImage();

  if (!Param) {
    iSetError(IL_INVALID_VALUE);
    return IL_FALSE;
  }

  switch (Mode) {
    case IL_CUR_IMAGE:
      if (CurImage == NULL) {
        iSetError(IL_ILLEGAL_OPERATION);
        return IL_FALSE;
      }
      *Param = Selection->CurName;
      return IL_TRUE;

    case IL_COMPRESS_MODE:        *Param = ilStates[ilCurrentPos].ilCompression; return IL_TRUE;
    case IL_FORMAT_MODE:          *Param = ilStates[ilCurrentPos].ilFormatMode; return IL_TRUE; 
    case IL_INTERLACE_MODE:       *Param = ilStates[ilCurrentPos].ilInterlace; return IL_TRUE;
    case IL_KEEP_DXTC_DATA:       *Param = ilStates[ilCurrentPos].ilKeepDxtcData; return IL_TRUE;
    case IL_ORIGIN_MODE:          *Param = ilStates[ilCurrentPos].ilOriginMode; return IL_TRUE;
    case IL_MAX_QUANT_INDICES:    *Param = ilStates[ilCurrentPos].ilQuantMaxIndexs; return IL_TRUE;
    case IL_NEU_QUANT_SAMPLE:     *Param = ilStates[ilCurrentPos].ilNeuSample; return IL_TRUE;
    case IL_QUANTIZATION_MODE:    *Param = ilStates[ilCurrentPos].ilQuantMode; return IL_TRUE;
    case IL_TYPE_MODE:            *Param = ilStates[ilCurrentPos].ilTypeMode; return IL_TRUE;
    case IL_VERSION_NUM:          *Param = IL_VERSION; return IL_TRUE;

    case IL_ACTIVE_IMAGE:
    case IL_ACTIVE_MIPMAP:
    case IL_ACTIVE_LAYER:         *Param = iGetActiveNum(Mode); return IL_TRUE;

    case IL_BMP_RLE:              *Param = ilStates[ilCurrentPos].ilBmpRle; return IL_TRUE;
    case IL_DXTC_FORMAT:          *Param = ilStates[ilCurrentPos].ilDxtcFormat; return IL_TRUE;
    case IL_JPG_QUALITY:          *Param = ilStates[ilCurrentPos].ilJpgQuality; return IL_TRUE;
    case IL_JPG_SAVE_FORMAT:      *Param = ilStates[ilCurrentPos].ilJpgFormat; return IL_TRUE;
    case IL_PCD_PICNUM:           *Param = ilStates[ilCurrentPos].ilPcdPicNum; return IL_TRUE;
    case IL_PNG_ALPHA_INDEX:      *Param = ilStates[ilCurrentPos].ilPngAlphaIndex; return IL_TRUE;
    case IL_PNG_INTERLACE:        *Param = ilStates[ilCurrentPos].ilPngInterlace; return IL_TRUE;
    case IL_SGI_RLE:              *Param = ilStates[ilCurrentPos].ilSgiRle; return IL_TRUE;
    case IL_TGA_CREATE_STAMP:     *Param = ilStates[ilCurrentPos].ilTgaCreateStamp; return IL_TRUE;
    case IL_TGA_RLE:              *Param = ilStates[ilCurrentPos].ilTgaRle; return IL_TRUE;
    case IL_VTF_COMP:             *Param = ilStates[ilCurrentPos].ilVtfCompression; return IL_TRUE;

    case IL_CONV_PAL:             *Param = ilStates[ilCurrentPos].ilAutoConvPal; return IL_TRUE;
    case IL_DEFAULT_ON_FAIL:      *Param = ilStates[ilCurrentPos].ilDefaultOnFail; return IL_TRUE;
    case IL_FILE_MODE:            *Param = ilStates[ilCurrentPos].ilOverWriteFiles; return IL_TRUE;
    case IL_FORMAT_SET:           *Param = ilStates[ilCurrentPos].ilFormatSet; return IL_TRUE;
    case IL_ORIGIN_SET:           *Param = ilStates[ilCurrentPos].ilOriginSet; return IL_TRUE;
    case IL_TYPE_SET:             *Param = ilStates[ilCurrentPos].ilTypeSet; return IL_TRUE;
    case IL_USE_KEY_COLOUR:       *Param = ilStates[ilCurrentPos].ilUseKeyColour; return IL_TRUE;
    case IL_BLIT_BLEND:           *Param = ilStates[ilCurrentPos].ilBlitBlend; return IL_TRUE;
    case IL_JPG_PROGRESSIVE:      *Param = ilStates[ilCurrentPos].ilJpgProgressive; return IL_TRUE;
    case IL_NVIDIA_COMPRESS:      *Param = ilStates[ilCurrentPos].ilUseNVidiaDXT; return IL_TRUE;
    case IL_SQUISH_COMPRESS:      *Param = ilStates[ilCurrentPos].ilUseSquishDXT; return IL_TRUE;
    case IL_IMAGE_SELECTION_MODE: *Param = ilStates[ilCurrentPos].ilImageSelectionMode; return IL_TRUE;

    case IL_IMAGE_METADATA_COUNT:
    case IL_META_EXIF_VERSION:
    case IL_META_EXPOSURE_PROGRAM:
    case IL_META_METERING_MODE:
    case IL_META_LIGHT_SOURCE:
    case IL_META_FLASH:
    case IL_META_GPS_VERSION:
    case IL_META_GPS_DIFFERENTIAL:
      return iGetIntegerImageV(BaseImage, Mode, Param);
  }

  return iGetIntegerImageV(CurImage, Mode, Param);
}


ILint iGetInteger(ILenum Mode) {
  ILint Param = 0;

  switch (Mode) {
    case IL_META_EXPOSURE_TIME:
      // TODO: block modes that have multiple values
      iSetError(IL_INVALID_ENUM);
      return 0;
  }

  iGetIntegerV(Mode, &Param);
  return Param;
}

ILboolean iOriginFunc(ILenum Mode) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;

  switch (Mode) {
    case IL_ORIGIN_LOWER_LEFT:
    case IL_ORIGIN_UPPER_LEFT:
      ilStates[ilCurrentPos].ilOriginMode = Mode;
      break;
    default:
      iSetError(IL_INVALID_PARAM);
      return IL_FALSE;
  }
  return IL_TRUE;
}


ILboolean iFormatFunc(ILenum Mode) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;

  switch (Mode)
  {
    //case IL_COLOUR_INDEX:
    case IL_RGB:
    case IL_RGBA:
    case IL_BGR:
    case IL_BGRA:
    case IL_LUMINANCE:
    case IL_LUMINANCE_ALPHA:
      ilStates[ilCurrentPos].ilFormatMode = Mode;
      break;
    default:
      iSetError(IL_INVALID_PARAM);
      return IL_FALSE;
  }
  return IL_TRUE;
}

ILboolean iTypeFunc(ILenum Mode) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;

  switch (Mode) {
    case IL_BYTE:
    case IL_UNSIGNED_BYTE:
    case IL_SHORT:
    case IL_UNSIGNED_SHORT:
    case IL_INT:
    case IL_UNSIGNED_INT:
    case IL_FLOAT:
    case IL_DOUBLE:
      ilStates[ilCurrentPos].ilTypeMode = Mode;
      break;
    default:
      iSetError(IL_INVALID_PARAM);
      return IL_FALSE;
  }
  return IL_TRUE;
}


ILboolean ILAPIENTRY ilCompressFunc(ILenum Mode) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;

  switch (Mode)
  {
    case IL_COMPRESS_NONE:
    case IL_COMPRESS_RLE:
    //case IL_COMPRESS_LZO:
    case IL_COMPRESS_ZLIB:
      ilStates[ilCurrentPos].ilCompression = Mode;
      break;
    default:
      iSetError(IL_INVALID_PARAM);
      return IL_FALSE;
  }
  return IL_TRUE;
}

void iPushAttrib(ILuint Bits) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos ;

  // Should we check here to see if ilCurrentPos is negative?

  if (StateStruct->ilCurrentPos >= IL_ATTRIB_STACK_MAX - 1) {
    StateStruct->ilCurrentPos = IL_ATTRIB_STACK_MAX - 1;
    iSetError(IL_STACK_OVERFLOW);
    return;
  }

  StateStruct->ilCurrentPos++;

  //  memcpy(&ilStates[ilCurrentPos], &ilStates[ilCurrentPos - 1], sizeof(IL_STATES));

  ilDefaultStates();

  // FIXME: colour key data

  ilCurrentPos = StateStruct->ilCurrentPos;

  if (Bits & IL_ORIGIN_BIT) {
    ilStates[ilCurrentPos].ilOriginMode = ilStates[ilCurrentPos-1].ilOriginMode;
    ilStates[ilCurrentPos].ilOriginSet  = ilStates[ilCurrentPos-1].ilOriginSet;
  }
  if (Bits & IL_FORMAT_BIT) {
    ilStates[ilCurrentPos].ilFormatMode = ilStates[ilCurrentPos-1].ilFormatMode;
    ilStates[ilCurrentPos].ilFormatSet  = ilStates[ilCurrentPos-1].ilFormatSet;
  }
  if (Bits & IL_TYPE_BIT) {
    ilStates[ilCurrentPos].ilTypeMode = ilStates[ilCurrentPos-1].ilTypeMode;
    ilStates[ilCurrentPos].ilTypeSet  = ilStates[ilCurrentPos-1].ilTypeSet;
  }
  if (Bits & IL_FILE_BIT) {
    ilStates[ilCurrentPos].ilOverWriteFiles = ilStates[ilCurrentPos-1].ilOverWriteFiles;
  }
  if (Bits & IL_PAL_BIT) {
    ilStates[ilCurrentPos].ilAutoConvPal = ilStates[ilCurrentPos-1].ilAutoConvPal;
  }
  if (Bits & IL_LOADFAIL_BIT) {
    ilStates[ilCurrentPos].ilDefaultOnFail = ilStates[ilCurrentPos-1].ilDefaultOnFail;
  }
  if (Bits & IL_COMPRESS_BIT) {
    ilStates[ilCurrentPos].ilCompression = ilStates[ilCurrentPos-1].ilCompression;
  }
  if (Bits & IL_FORMAT_SPECIFIC_BIT) {
    ilStates[ilCurrentPos].ilTgaCreateStamp = ilStates[ilCurrentPos-1].ilTgaCreateStamp;
    ilStates[ilCurrentPos].ilJpgQuality = ilStates[ilCurrentPos-1].ilJpgQuality;
    ilStates[ilCurrentPos].ilPngInterlace = ilStates[ilCurrentPos-1].ilPngInterlace;
    ilStates[ilCurrentPos].ilTgaRle = ilStates[ilCurrentPos-1].ilTgaRle;
    ilStates[ilCurrentPos].ilBmpRle = ilStates[ilCurrentPos-1].ilBmpRle;
    ilStates[ilCurrentPos].ilSgiRle = ilStates[ilCurrentPos-1].ilSgiRle;
    ilStates[ilCurrentPos].ilJpgFormat = ilStates[ilCurrentPos-1].ilJpgFormat;
    ilStates[ilCurrentPos].ilDxtcFormat = ilStates[ilCurrentPos-1].ilDxtcFormat;
    ilStates[ilCurrentPos].ilPcdPicNum = ilStates[ilCurrentPos-1].ilPcdPicNum;

    ilStates[ilCurrentPos].ilPngAlphaIndex = ilStates[ilCurrentPos-1].ilPngAlphaIndex;

    // Strings
    if (ilStates[ilCurrentPos].ilTgaId)
      ifree(ilStates[ilCurrentPos].ilTgaId);
    if (ilStates[ilCurrentPos].ilTgaAuthName)
      ifree(ilStates[ilCurrentPos].ilTgaAuthName);
    if (ilStates[ilCurrentPos].ilTgaAuthComment)
      ifree(ilStates[ilCurrentPos].ilTgaAuthComment);
    if (ilStates[ilCurrentPos].ilPngAuthName)
      ifree(ilStates[ilCurrentPos].ilPngAuthName);
    if (ilStates[ilCurrentPos].ilPngTitle)
      ifree(ilStates[ilCurrentPos].ilPngTitle);
    if (ilStates[ilCurrentPos].ilPngDescription)
      ifree(ilStates[ilCurrentPos].ilPngDescription);

    //2003-09-01: added tif strings
    if (ilStates[ilCurrentPos].ilTifDescription)
      ifree(ilStates[ilCurrentPos].ilTifDescription);
    if (ilStates[ilCurrentPos].ilTifHostComputer)
      ifree(ilStates[ilCurrentPos].ilTifHostComputer);
    if (ilStates[ilCurrentPos].ilTifDocumentName)
      ifree(ilStates[ilCurrentPos].ilTifDocumentName);
    if (ilStates[ilCurrentPos].ilTifAuthName)
      ifree(ilStates[ilCurrentPos].ilTifAuthName);

    if (ilStates[ilCurrentPos].ilCHeader)
      ifree(ilStates[ilCurrentPos].ilCHeader);

    ilStates[ilCurrentPos].ilTgaId = iStrDup(ilStates[ilCurrentPos-1].ilTgaId);
    ilStates[ilCurrentPos].ilTgaAuthName = iStrDup(ilStates[ilCurrentPos-1].ilTgaAuthName);
    ilStates[ilCurrentPos].ilTgaAuthComment = iStrDup(ilStates[ilCurrentPos-1].ilTgaAuthComment);
    ilStates[ilCurrentPos].ilPngAuthName = iStrDup(ilStates[ilCurrentPos-1].ilPngAuthName);
    ilStates[ilCurrentPos].ilPngTitle = iStrDup(ilStates[ilCurrentPos-1].ilPngTitle);
    ilStates[ilCurrentPos].ilPngDescription = iStrDup(ilStates[ilCurrentPos-1].ilPngDescription);

    //2003-09-01: added tif strings
    ilStates[ilCurrentPos].ilTifDescription = iStrDup(ilStates[ilCurrentPos-1].ilTifDescription);
    ilStates[ilCurrentPos].ilTifHostComputer = iStrDup(ilStates[ilCurrentPos-1].ilTifHostComputer);
    ilStates[ilCurrentPos].ilTifDocumentName = iStrDup(ilStates[ilCurrentPos-1].ilTifDocumentName);
    ilStates[ilCurrentPos].ilTifAuthName = iStrDup(ilStates[ilCurrentPos-1].ilTifAuthName);

    ilStates[ilCurrentPos].ilCHeader = iStrDup(ilStates[ilCurrentPos-1].ilCHeader);
  }

  ilStates[ilCurrentPos].ilImageSelectionMode = ilStates[ilCurrentPos-1].ilImageSelectionMode;
}


// @TODO:  Find out how this affects strings!!!
void iPopAttrib() {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;

  if (ilCurrentPos <= 0) {
    ilCurrentPos = 0;
    iSetError(IL_STACK_UNDERFLOW);
    return;
  }

  // Should we check here to see if ilCurrentPos is too large?
  StateStruct->ilCurrentPos--;

  return;
}


//! Specifies implementation-dependent performance hints
void iHint(ILenum Target, ILenum Mode) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  switch (Target)   {
    case IL_MEM_SPEED_HINT:
      switch (Mode) {
        case IL_FASTEST:
          StateStruct->ilHints.MemVsSpeedHint = Mode;
          break;
        case IL_LESS_MEM:
          StateStruct->ilHints.MemVsSpeedHint = Mode;
          break;
        case IL_DONT_CARE:
          StateStruct->ilHints.MemVsSpeedHint = IL_FASTEST;
          break;
        default:
          iSetError(IL_INVALID_ENUM);
          return;
      }
      break;

    case IL_COMPRESSION_HINT:
      switch (Mode)
      {
        case IL_USE_COMPRESSION:
          StateStruct->ilHints.CompressHint = Mode;
          break;
        case IL_NO_COMPRESSION:
          StateStruct->ilHints.CompressHint = Mode;
          break;
        case IL_DONT_CARE:
          StateStruct->ilHints.CompressHint = IL_NO_COMPRESSION;
          break;
        default:
          iSetError(IL_INVALID_ENUM);
          return;
      }
      break;

      
    default:
      iSetError(IL_INVALID_ENUM);
      return;
  }

  return;
}


ILenum iGetHint(ILenum Target) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();

  switch (Target)
  {
    case IL_MEM_SPEED_HINT:
      return StateStruct->ilHints.MemVsSpeedHint;
    case IL_COMPRESSION_HINT:
      return StateStruct->ilHints.CompressHint;
    default:
      iSetError(IL_INTERNAL_ERROR);
      return 0;
  }
}


void iSetString(ILenum Mode, const char *String_) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;
  ILchar *String;

  if (String_ == NULL) {
    iSetError(IL_INVALID_PARAM);
    return;
  }

#ifdef _UNICODE
  String = iWideFromMultiByte(String_);
#else
  String = iCharStrDup(String_);
#endif

  if (String == NULL) {
    iSetError(IL_INTERNAL_ERROR);
    return;
  }

  switch (Mode)
  {
    case IL_TGA_ID_STRING:
      if (ilStates[ilCurrentPos].ilTgaId)
        ifree(ilStates[ilCurrentPos].ilTgaId);
      ilStates[ilCurrentPos].ilTgaId = String;
      break;
    case IL_TGA_AUTHNAME_STRING:
      if (ilStates[ilCurrentPos].ilTgaAuthName)
        ifree(ilStates[ilCurrentPos].ilTgaAuthName);
      ilStates[ilCurrentPos].ilTgaAuthName = String;
      break;
    case IL_TGA_AUTHCOMMENT_STRING:
      if (ilStates[ilCurrentPos].ilTgaAuthComment)
        ifree(ilStates[ilCurrentPos].ilTgaAuthComment);
      ilStates[ilCurrentPos].ilTgaAuthComment = String;
      break;
    case IL_PNG_AUTHNAME_STRING:
      if (ilStates[ilCurrentPos].ilPngAuthName)
        ifree(ilStates[ilCurrentPos].ilPngAuthName);
      ilStates[ilCurrentPos].ilPngAuthName = String;
      break;
    case IL_PNG_TITLE_STRING:
      if (ilStates[ilCurrentPos].ilPngTitle)
        ifree(ilStates[ilCurrentPos].ilPngTitle);
      ilStates[ilCurrentPos].ilPngTitle = String;
      break;
    case IL_PNG_DESCRIPTION_STRING:
      if (ilStates[ilCurrentPos].ilPngDescription)
        ifree(ilStates[ilCurrentPos].ilPngDescription);
      ilStates[ilCurrentPos].ilPngDescription = String;
      break;

    //2003-09-01: added tif strings
    case IL_TIF_DESCRIPTION_STRING:
      if (ilStates[ilCurrentPos].ilTifDescription)
        ifree(ilStates[ilCurrentPos].ilTifDescription);
      ilStates[ilCurrentPos].ilTifDescription = String;
      break;
    case IL_TIF_HOSTCOMPUTER_STRING:
      if (ilStates[ilCurrentPos].ilTifHostComputer)
        ifree(ilStates[ilCurrentPos].ilTifHostComputer);
      ilStates[ilCurrentPos].ilTifHostComputer = String;
      break;
    case IL_TIF_DOCUMENTNAME_STRING:
            if (ilStates[ilCurrentPos].ilTifDocumentName)
        ifree(ilStates[ilCurrentPos].ilTifDocumentName);
      ilStates[ilCurrentPos].ilTifDocumentName = String;
      break;
    case IL_TIF_AUTHNAME_STRING:
      if (ilStates[ilCurrentPos].ilTifAuthName)
        ifree(ilStates[ilCurrentPos].ilTifAuthName);
      ilStates[ilCurrentPos].ilTifAuthName = String;
      break;

    case IL_CHEAD_HEADER_STRING:
      if (ilStates[ilCurrentPos].ilCHeader)
        ifree(ilStates[ilCurrentPos].ilCHeader);
      ilStates[ilCurrentPos].ilCHeader = String;
      break;

    default:
      iSetError(IL_INVALID_ENUM);
  }

  return;
}


void iSetInteger(ILimage *CurImage , ILenum Mode, ILint Param) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;

  switch (Mode)
  {
    // Integer values
    case IL_FORMAT_MODE:
      ilFormatFunc(Param);
      return;
    case IL_KEEP_DXTC_DATA:
      if (Param == IL_FALSE || Param == IL_TRUE) {
        ilStates[ilCurrentPos].ilKeepDxtcData = Param;
        return;
      }
      break;
    case IL_MAX_QUANT_INDICES:
      if (Param >= 2 && Param <= 256) {
        ilStates[ilCurrentPos].ilQuantMaxIndexs = Param;
        return;
      }
      break;
    case IL_NEU_QUANT_SAMPLE:
      if (Param >= 1 && Param <= 30) {
        ilStates[ilCurrentPos].ilNeuSample = Param;
        return;
      }
      break;
    case IL_ORIGIN_MODE:
      ilOriginFunc(Param);
      return;
    case IL_QUANTIZATION_MODE:
      if (Param == IL_WU_QUANT || Param == IL_NEU_QUANT) {
        ilStates[ilCurrentPos].ilQuantMode = Param;
        return;
      }
      break;
    case IL_TYPE_MODE:
      ilTypeFunc(Param);
      return;

    // Image specific values
    case IL_IMAGE_DURATION:
      if (CurImage == NULL) {
        iSetError(IL_ILLEGAL_OPERATION);
        break;
      }
      CurImage->Duration = Param;
      return;
    case IL_IMAGE_OFFX:
      if (CurImage == NULL) {
        iSetError(IL_ILLEGAL_OPERATION);
        break;
      }
      CurImage->OffX = Param;
      return;
    case IL_IMAGE_OFFY:
      if (CurImage == NULL) {
        iSetError(IL_ILLEGAL_OPERATION);
        break;
      }
      CurImage->OffY = Param;
      return;
    case IL_IMAGE_CUBEFLAGS:
      if (CurImage == NULL) {
        iSetError(IL_ILLEGAL_OPERATION);
        break;
      }
      CurImage->CubeFlags = Param;
      break;
 
    // Format specific values
    case IL_BMP_RLE:
      if (Param == IL_FALSE || Param == IL_TRUE) {
        ilStates[ilCurrentPos].ilBmpRle = Param;
        return;
      }
      break;
    case IL_DXTC_FORMAT:
      if (Param >= IL_DXT1 || Param <= IL_DXT5 || Param == IL_DXT1A) {
        ilStates[ilCurrentPos].ilDxtcFormat = Param;
        return;
      }
      break;
    case IL_JPG_SAVE_FORMAT:
      if (Param == IL_JFIF || Param == IL_EXIF) {
        ilStates[ilCurrentPos].ilJpgFormat = Param;
        return;
      }
      break;
    case IL_JPG_QUALITY:
      if (Param >= 0 && Param <= 99) {
        ilStates[ilCurrentPos].ilJpgQuality = Param;
        return;
      }
      break;
    case IL_PNG_INTERLACE:
      if (Param == IL_FALSE || Param == IL_TRUE) {
        ilStates[ilCurrentPos].ilPngInterlace = Param;
        return;
      }
      break;
    case IL_PCD_PICNUM:
      if (Param >= 0 || Param <= 2) {
        ilStates[ilCurrentPos].ilPcdPicNum = Param;
        return;
      }
      break;
    case IL_PNG_ALPHA_INDEX:
      if (Param >= -1 || Param <= 255) {
        ilStates[ilCurrentPos].ilPngAlphaIndex=Param;
        return;
      }
      break;
    case IL_SGI_RLE:
      if (Param == IL_FALSE || Param == IL_TRUE) {
        ilStates[ilCurrentPos].ilSgiRle = Param;
        return;
      }
      break;
    case IL_TGA_CREATE_STAMP:
      if (Param == IL_FALSE || Param == IL_TRUE) {
        ilStates[ilCurrentPos].ilTgaCreateStamp = Param;
        return;
      }
      break;
    case IL_TGA_RLE:
      if (Param == IL_FALSE || Param == IL_TRUE) {
        ilStates[ilCurrentPos].ilTgaRle = Param;
        return;
      }
      break;
    case IL_VTF_COMP:
      if (Param == IL_DXT1 || Param == IL_DXT5 || Param == IL_DXT3 || Param == IL_DXT1A || Param == IL_DXT_NO_COMP) {
        ilStates[ilCurrentPos].ilVtfCompression = Param;
        return;
      }
      break;
    case IL_FILE_MODE: 
    case IL_FILE_OVERWRITE: // deprecated: unused, IL_FILE_MODE is used instead
      ilStates[ilCurrentPos].ilOverWriteFiles = !!Param;
      return;

    case IL_IMAGE_SELECTION_MODE:
      if (Param == IL_RELATIVE || Param == IL_ABSOLUTE) {
        ilStates[ilCurrentPos].ilImageSelectionMode = Param;
        return;
      }
      break;


    default:
      iSetError(IL_INVALID_ENUM);
      return;
  }

  iSetError(IL_INVALID_PARAM);  // Parameter not in valid bounds.
  return;
}



ILint iGetInt(ILenum Mode) {
  //like ilGetInteger(), but sets another error on failure

  //call ilGetIntegerv() for more robust code
  ILenum err;
  ILint r = -1;

  ilGetIntegerv(Mode, &r);

  //check if an error occured, set another error
  err = ilGetError();
  if (r == -1 && err == IL_INVALID_ENUM) {
    iSetError(IL_INTERNAL_ERROR);
  } else {
    iSetError(err); //restore error
  }

  return r;
}

void iKeyColour(ILclampf Red, ILclampf Green, ILclampf Blue, ILclampf Alpha) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;

  ilStates[ilCurrentPos].ilKeyColourRed = Red;
  ilStates[ilCurrentPos].ilKeyColourGreen = Green;
  ilStates[ilCurrentPos].ilKeyColourBlue = Blue;
  ilStates[ilCurrentPos].ilKeyColourAlpha = Alpha; 
}


void iGetKeyColour(ILclampf *Red, ILclampf *Green, ILclampf *Blue, ILclampf *Alpha) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;

  *Red = ilStates[ilCurrentPos].ilKeyColourRed;
  *Green = ilStates[ilCurrentPos].ilKeyColourGreen;
  *Blue = ilStates[ilCurrentPos].ilKeyColourBlue;
  *Alpha = ilStates[ilCurrentPos].ilKeyColourAlpha; 
}
