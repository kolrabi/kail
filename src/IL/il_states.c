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
#include "il_meta.h"
#include "il_pixel.h"

#include <stdlib.h>

// Global variables
static ILconst_string _ilVendor    = IL_TEXT("kolrabi");
static ILconst_string _ilVersion   = IL_TEXT("kolrabi's another Image Library (kaIL) 1.10.0");

static IL_STATE_STRUCT *iGetStateStruct() {
  return &iGetTLSData()->CurState;
}

extern ILchar *iLoadExtensions;
extern ILchar *iSaveExtensions;

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

  // Changed to the colour of the Universe
  //  (http://www.newscientist.com/news/news.jsp?id=ns99991775)
  //  *(http://www.space.com/scienceastronomy/universe_color_020308.html)*
  ilStates[ilCurrentPos].ClearColour[0] = 1.0f;
  ilStates[ilCurrentPos].ClearColour[1] = 0.972549f;
  ilStates[ilCurrentPos].ClearColour[2] = 0.90588f;
  ilStates[ilCurrentPos].ClearColour[3] = 0.0f;
  ilStates[ilCurrentPos].ClearIndex     = 0;

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

////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// String
///

//! Returns a constant string detailing aspects about this library.
ILconst_string iGetILString(ILimage *Image, ILenum StringName) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;
  ILimage *BaseImage = Image->BaseImage;

  switch (StringName) {
    case IL_VENDOR:                   return _ilVendor;
    case IL_VERSION_NUM:              return _ilVersion;
    case IL_LOAD_EXT:                 return iLoadExtensions;
    case IL_SAVE_EXT:                 return iSaveExtensions;

    case IL_TGA_ID_STRING:            
      iTrace("---- IL_TGA_ID_STRING is obsolete, use IL_META_DOCUMENT_NAME instead");
      return iGetMetaString(BaseImage, IL_META_DOCUMENT_NAME);

    case IL_TGA_AUTHNAME_STRING:
      iTrace("---- IL_TGA_AUTHNAME_STRING is obsolete, use IL_META_ARTIST instead");
      return iGetMetaString(BaseImage, IL_META_ARTIST);

    case IL_PNG_AUTHNAME_STRING:
      iTrace("---- IL_PNG_AUTHNAME_STRING is obsolete, use IL_META_ARTIST instead");
      return iGetMetaString(BaseImage, IL_META_ARTIST);

    case IL_TIF_AUTHNAME_STRING:
      iTrace("---- IL_TIF_AUTHNAME_STRING is obsolete, use IL_META_ARTIST instead");
      return iGetMetaString(BaseImage, IL_META_ARTIST);

    case IL_PNG_TITLE_STRING:
      iTrace("---- IL_PNG_TITLE_STRING is obsolete, use IL_META_DOCUMENT_NAME instead");
      return iGetMetaString(BaseImage, IL_META_DOCUMENT_NAME);

    case IL_TIF_DOCUMENTNAME_STRING:
      iTrace("---- IL_TIF_DOCUMENTNAME_STRING is obsolete, use IL_META_DOCUMENT_NAME instead");
      return iGetMetaString(BaseImage, IL_META_DOCUMENT_NAME);

    case IL_PNG_DESCRIPTION_STRING:
      iTrace("---- IL_PNG_DESCRIPTION_STRING is obsolete, use IL_META_IMAGE_DESCRIPTION instead");
      return iGetMetaString(BaseImage, IL_META_IMAGE_DESCRIPTION);

    case IL_TIF_DESCRIPTION_STRING:
      iTrace("---- IL_TIF_DESCRIPTION_STRING is obsolete, use IL_META_IMAGE_DESCRIPTION instead");
      return iGetMetaString(BaseImage, IL_META_IMAGE_DESCRIPTION);

    case IL_TGA_AUTHCOMMENT_STRING:
      iTrace("---- IL_TGA_AUTHCOMMENT_STRING is obsolete, use IL_META_USER_COMMENT instead");
      return iGetMetaString(BaseImage, IL_META_USER_COMMENT);

    case IL_TIF_HOSTCOMPUTER_STRING:
      iTrace("---- IL_TIF_HOSTCOMPUTER_STRING is obsolete, use IL_META_HOST_COMPUTER instead");
      return iGetMetaString(BaseImage, IL_META_HOST_COMPUTER);

    case IL_CHEAD_HEADER_STRING:      return (ILconst_string)ilStates[ilCurrentPos].ilCHeader;

    default:
      return iGetMetaString(BaseImage, StringName);
  }
}

// Returns format-specific strings, converted to UTF8
char *iGetString(ILimage *Image, ILenum StringName) {
#ifdef _UNICODE
  char *String = iMultiByteFromWide(iGetILString(Image, StringName));
#else
  char *String = iCharStrDup(iGetILString(Image, StringName));
#endif
  return String;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// Boolean
///

// Internal function that sets the Mode equal to Flag
ILboolean iAble(ILenum Mode, ILboolean Flag) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;

  switch (Mode) {
    case IL_ORIGIN_SET:       ilStates[ilCurrentPos].ilOriginSet      = Flag;     break;
    case IL_FORMAT_SET:       ilStates[ilCurrentPos].ilFormatSet      = Flag;     break;
    case IL_TYPE_SET:         ilStates[ilCurrentPos].ilTypeSet        = Flag;     break;
    case IL_FILE_OVERWRITE:   // deprecated: unused, IL_FILE_MODE is used instead
    case IL_FILE_MODE:        ilStates[ilCurrentPos].ilOverWriteFiles = Flag;     break;
    case IL_CONV_PAL:         ilStates[ilCurrentPos].ilAutoConvPal    = Flag;     break;
    case IL_DEFAULT_ON_FAIL:  ilStates[ilCurrentPos].ilDefaultOnFail  = Flag;     break;
    case IL_USE_KEY_COLOUR:   ilStates[ilCurrentPos].ilUseKeyColour   = Flag;     break;
    case IL_BLIT_BLEND:       ilStates[ilCurrentPos].ilBlitBlend      = Flag;     break;
    case IL_SAVE_INTERLACED:  ilStates[ilCurrentPos].ilInterlace      = Flag;     break;
    case IL_JPG_PROGRESSIVE:  ilStates[ilCurrentPos].ilJpgProgressive = Flag;     break;
    case IL_NVIDIA_COMPRESS:  ilStates[ilCurrentPos].ilUseNVidiaDXT   = Flag;     break;
    case IL_SQUISH_COMPRESS:  ilStates[ilCurrentPos].ilUseSquishDXT   = Flag;     break;

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
    case IL_ORIGIN_SET:       return ilStates[ilCurrentPos].ilOriginSet;
    case IL_FORMAT_SET:       return ilStates[ilCurrentPos].ilFormatSet;
    case IL_TYPE_SET:         return ilStates[ilCurrentPos].ilTypeSet;
    case IL_FILE_MODE: 
    case IL_FILE_OVERWRITE:   return ilStates[ilCurrentPos].ilOverWriteFiles;
    case IL_CONV_PAL:         return ilStates[ilCurrentPos].ilAutoConvPal;
    case IL_DEFAULT_ON_FAIL:  return ilStates[ilCurrentPos].ilDefaultOnFail;
    case IL_USE_KEY_COLOUR:   return ilStates[ilCurrentPos].ilUseKeyColour;
    case IL_BLIT_BLEND:       return ilStates[ilCurrentPos].ilBlitBlend;
    case IL_SAVE_INTERLACED:  return ilStates[ilCurrentPos].ilInterlace;
    case IL_JPG_PROGRESSIVE:  return ilStates[ilCurrentPos].ilJpgProgressive;
    case IL_NVIDIA_COMPRESS:  return ilStates[ilCurrentPos].ilUseNVidiaDXT;
    case IL_SQUISH_COMPRESS:  return ilStates[ilCurrentPos].ilUseSquishDXT;

    default:
      iSetError(IL_INVALID_ENUM);
  }

  return IL_FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// Integer
///

ILuint ILAPIENTRY iGetiv(ILimage *Image, ILenum Mode, ILint *Param, ILuint MaxCount) {
  IL_STATE_STRUCT *     StateStruct   = iGetStateStruct();
  IL_IMAGE_SELECTION *  Selection     = &iGetTLSData()->CurSel;
  IL_STATES *           ilStates      = StateStruct->ilStates;
  ILuint                ilCurrentPos  = StateStruct->ilCurrentPos;
  ILimage *             SubImage      = NULL;

  switch (Mode) {
    case IL_CUR_IMAGE:
      if (Image == NULL) {
        iSetError(IL_ILLEGAL_OPERATION);
        return 0;
      }
      if (Param) *Param = Selection->CurName;
      return 1;

    case IL_COMPRESS_MODE:        if (Param) *Param = ilStates[ilCurrentPos].ilCompression; return 1;
    case IL_FORMAT_MODE:          if (Param) *Param = ilStates[ilCurrentPos].ilFormatMode; return 1; 
    case IL_INTERLACE_MODE:       if (Param) *Param = ilStates[ilCurrentPos].ilInterlace; return 1;
    case IL_KEEP_DXTC_DATA:       if (Param) *Param = ilStates[ilCurrentPos].ilKeepDxtcData; return 1;
    case IL_ORIGIN_MODE:          if (Param) *Param = ilStates[ilCurrentPos].ilOriginMode; return 1;
    case IL_MAX_QUANT_INDICES:    if (Param) *Param = ilStates[ilCurrentPos].ilQuantMaxIndexs; return 1;
    case IL_NEU_QUANT_SAMPLE:     if (Param) *Param = ilStates[ilCurrentPos].ilNeuSample; return 1;
    case IL_QUANTIZATION_MODE:    if (Param) *Param = ilStates[ilCurrentPos].ilQuantMode; return 1;
    case IL_TYPE_MODE:            if (Param) *Param = ilStates[ilCurrentPos].ilTypeMode; return 1;
    case IL_VERSION_NUM:          if (Param) *Param = IL_VERSION; return 1;

    case IL_ACTIVE_IMAGE:         if (Param) *Param = Selection->CurFrame; return 1;
    case IL_ACTIVE_MIPMAP:        if (Param) *Param = Selection->CurMipmap; return 1;
    case IL_ACTIVE_LAYER:         if (Param) *Param = Selection->CurLayer; return 1;
    case IL_ACTIVE_FACE:          if (Param) *Param = Selection->CurFace; return 1;

    case IL_BMP_RLE:              if (Param) *Param = ilStates[ilCurrentPos].ilBmpRle; return 1;
    case IL_DXTC_FORMAT:          if (Param) *Param = ilStates[ilCurrentPos].ilDxtcFormat; return 1;
    case IL_JPG_QUALITY:          if (Param) *Param = ilStates[ilCurrentPos].ilJpgQuality; return 1;
    case IL_JPG_SAVE_FORMAT:      if (Param) *Param = ilStates[ilCurrentPos].ilJpgFormat; return 1;
    case IL_PCD_PICNUM:           if (Param) *Param = ilStates[ilCurrentPos].ilPcdPicNum; return 1;
    case IL_PNG_ALPHA_INDEX:      if (Param) *Param = ilStates[ilCurrentPos].ilPngAlphaIndex; return 1;
    case IL_PNG_INTERLACE:        if (Param) *Param = ilStates[ilCurrentPos].ilPngInterlace; return 1;
    case IL_SGI_RLE:              if (Param) *Param = ilStates[ilCurrentPos].ilSgiRle; return 1;
    case IL_TGA_CREATE_STAMP:     if (Param) *Param = ilStates[ilCurrentPos].ilTgaCreateStamp; return 1;
    case IL_TGA_RLE:              if (Param) *Param = ilStates[ilCurrentPos].ilTgaRle; return 1;
    case IL_VTF_COMP:             if (Param) *Param = ilStates[ilCurrentPos].ilVtfCompression; return 1;

    case IL_CONV_PAL:             if (Param) *Param = ilStates[ilCurrentPos].ilAutoConvPal; return 1;
    case IL_DEFAULT_ON_FAIL:      if (Param) *Param = ilStates[ilCurrentPos].ilDefaultOnFail; return 1;
    case IL_FILE_MODE:            if (Param) *Param = ilStates[ilCurrentPos].ilOverWriteFiles; return 1;
    case IL_FORMAT_SET:           if (Param) *Param = ilStates[ilCurrentPos].ilFormatSet; return 1;
    case IL_ORIGIN_SET:           if (Param) *Param = ilStates[ilCurrentPos].ilOriginSet; return 1;
    case IL_TYPE_SET:             if (Param) *Param = ilStates[ilCurrentPos].ilTypeSet; return 1;
    case IL_USE_KEY_COLOUR:       if (Param) *Param = ilStates[ilCurrentPos].ilUseKeyColour; return 1;
    case IL_BLIT_BLEND:           if (Param) *Param = ilStates[ilCurrentPos].ilBlitBlend; return 1;
    case IL_JPG_PROGRESSIVE:      if (Param) *Param = ilStates[ilCurrentPos].ilJpgProgressive; return 1;
    case IL_NVIDIA_COMPRESS:      if (Param) *Param = ilStates[ilCurrentPos].ilUseNVidiaDXT; return 1;
    case IL_SQUISH_COMPRESS:      if (Param) *Param = ilStates[ilCurrentPos].ilUseSquishDXT; return 1;
    case IL_IMAGE_SELECTION_MODE: if (Param) *Param = ilStates[ilCurrentPos].ilImageSelectionMode; return 1;

    case IL_DXTC_DATA_FORMAT:
      if (Image->DxtcData == NULL || Image->DxtcSize == 0) {
        *Param = IL_DXT_NO_COMP;
      } else {
        *Param = Image->DxtcFormat;
      }
      return 1;

    case IL_NUM_FACES: 
      *Param = 0;
      for (SubImage = Image->Faces; SubImage; SubImage = SubImage->Faces) (*Param)++;
      return 1;

    case IL_NUM_IMAGES: 
      *Param = 0;
      for (SubImage = Image->Next; SubImage; SubImage = SubImage->Next) (*Param)++;
      return 1;

    case IL_NUM_LAYERS: 
      *Param = 0;
      for (SubImage = Image->Layers; SubImage; SubImage = SubImage->Layers) (*Param)++;
      return 1;

    case IL_NUM_MIPMAPS:
      *Param = 0;
      for (SubImage = Image->Mipmaps; SubImage; SubImage = SubImage->Mipmaps) (*Param)++;
      return 1;

    //changed 20040610 to channel count (Bpp) times Bytes per channel
    case IL_IMAGE_BITS_PER_PIXEL:         *Param = (Image->Bpp << 3)*Image->Bpc; return 1;

    //changed 20040610 to channel count (Bpp) times Bytes per channel
    case IL_IMAGE_BYTES_PER_PIXEL:        *Param = Image->Bpp*Image->Bpc; return 1;

    case IL_IMAGE_BPC:          *Param = Image->Bpc; return 1;
    case IL_IMAGE_CHANNELS:     *Param = Image->Bpp; return 1;
    case IL_IMAGE_CUBEFLAGS:    *Param = Image->CubeFlags; return 1;
    case IL_IMAGE_DEPTH:        *Param = Image->Depth; return 1;
    case IL_IMAGE_DURATION:     *Param = Image->Duration; return 1;
    case IL_IMAGE_FORMAT:       *Param = Image->Format; return 1;
    case IL_IMAGE_HEIGHT:       *Param = Image->Height; return 1;
    case IL_IMAGE_SIZE_OF_DATA: *Param = Image->SizeOfData; return 1;
    case IL_IMAGE_OFFX:         *Param = Image->OffX; return 1;
    case IL_IMAGE_OFFY:         *Param = Image->OffY; return 1;
    case IL_IMAGE_ORIGIN:       *Param = Image->Origin; return 1;
    case IL_IMAGE_PLANESIZE:    *Param = Image->SizeOfPlane; return 1;
    case IL_IMAGE_TYPE:         *Param = Image->Type; return 1;
    case IL_IMAGE_WIDTH:        *Param = Image->Width; return 1;
    case IL_PALETTE_TYPE:       *Param = Image->Pal.PalType; return 1;
    case IL_PALETTE_BPP:        *Param = iGetBppPal(Image->Pal.PalType); return 1;
    case IL_PALETTE_BASE_TYPE:  *Param = iGetPalBaseType(Image->Pal.PalType); return 1;
    case IL_PALETTE_NUM_COLS:
      if (!Image->Pal.Palette || !Image->Pal.PalSize || Image->Pal.PalType == IL_PAL_NONE)
        *Param = 0;
      else 
        *Param = Image->Pal.PalSize / iGetBppPal(Image->Pal.PalType);
      return 1;

    case IL_IMAGE_METADATA_COUNT: 
      if (Image->BaseImage) {
        ILmeta *Exif = Image->BaseImage->MetaTags;
        *Param = 0;
        while(Exif) { (*Param)++; Exif = Exif->Next; }
        return 1;
      } else {
        iSetError(IL_ILLEGAL_OPERATION);
        return 0;
      }
  }

  if (MaxCount > 0 && iGetMetaLen(Mode) > MaxCount) {
    iSetError(IL_INTERNAL_ERROR);
    return 0;
  }
  return iGetMetaiv(Image, Mode, Param, MaxCount);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// Float
///

ILuint ILAPIENTRY iGetfv(ILimage *Image, ILenum Mode, ILfloat *Param, ILuint MaxCount) {
  return iGetMetafv(Image, Mode, Param, MaxCount);
}

void ILAPIENTRY iSetfv(ILimage *Image, ILenum Mode, const ILfloat *Param, ILuint Count) {
  (void)Count;
  iSetMetafv(Image, Mode, Param);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// Misc
///

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


////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// State stack
///

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
    if (ilStates[ilCurrentPos].ilCHeader)
      ifree(ilStates[ilCurrentPos].ilCHeader);

    ilStates[ilCurrentPos].ilCHeader  = iStrDup(ilStates[ilCurrentPos-1].ilCHeader);
  }
  if (Bits & IL_COLOUR_KEY_BIT) {
    ilStates[ilCurrentPos].ilUseKeyColour   = ilStates[ilCurrentPos-1].ilUseKeyColour;
    ilStates[ilCurrentPos].ilKeyColourRed   = ilStates[ilCurrentPos-1].ilKeyColourRed;
    ilStates[ilCurrentPos].ilKeyColourGreen = ilStates[ilCurrentPos-1].ilKeyColourGreen;
    ilStates[ilCurrentPos].ilKeyColourBlue  = ilStates[ilCurrentPos-1].ilKeyColourBlue;
    ilStates[ilCurrentPos].ilKeyColourAlpha = ilStates[ilCurrentPos-1].ilKeyColourAlpha;
  }

  if (Bits & IL_CLEAR_COLOUR_BIT) {
    memcpy(ilStates[ilCurrentPos].ClearColour, ilStates[ilCurrentPos-1].ClearColour, sizeof(ilStates[ilCurrentPos].ClearColour));
    ilStates[ilCurrentPos].ClearIndex = ilStates[ilCurrentPos-1].ClearIndex;
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


void iSetString(ILimage *Image, ILenum Mode, const char *String_) {
  ILchar *String;
  ILimage *BaseImage = Image->BaseImage;

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
      iTrace("---- IL_TGA_ID_STRING is obsolete, use IL_META_DOCUMENT_NAME instead");
      iSetMetaString(BaseImage, IL_META_DOCUMENT_NAME, String_); return;

    case IL_TGA_AUTHNAME_STRING:
      iTrace("---- IL_TGA_AUTHNAME_STRING is obsolete, use IL_META_ARTIST instead");
      iSetMetaString(BaseImage, IL_META_ARTIST, String_); return;

    case IL_PNG_AUTHNAME_STRING:
      iTrace("---- IL_PNG_AUTHNAME_STRING is obsolete, use IL_META_ARTIST instead");
      iSetMetaString(BaseImage, IL_META_ARTIST, String_); return;

    case IL_TIF_AUTHNAME_STRING:
      iTrace("---- IL_TIF_AUTHNAME_STRING is obsolete, use IL_META_ARTIST instead");
      iSetMetaString(BaseImage, IL_META_ARTIST, String_); return;

    case IL_PNG_TITLE_STRING:
      iTrace("---- IL_PNG_TITLE_STRING is obsolete, use IL_META_DOCUMENT_NAME instead");
      iSetMetaString(BaseImage, IL_META_DOCUMENT_NAME, String_); return;

    case IL_TIF_DOCUMENTNAME_STRING:
      iTrace("---- IL_TIF_DOCUMENTNAME_STRING is obsolete, use IL_META_DOCUMENT_NAME instead");
      iSetMetaString(BaseImage, IL_META_DOCUMENT_NAME, String_); return;

    case IL_PNG_DESCRIPTION_STRING:
      iTrace("---- IL_PNG_DESCRIPTION_STRING is obsolete, use IL_META_IMAGE_DESCRIPTION instead");
      iSetMetaString(BaseImage, IL_META_IMAGE_DESCRIPTION, String_); return;

    case IL_TIF_DESCRIPTION_STRING:
      iTrace("---- IL_TIF_DESCRIPTION_STRING is obsolete, use IL_META_IMAGE_DESCRIPTION instead");
      iSetMetaString(BaseImage, IL_META_IMAGE_DESCRIPTION, String_); return;

    case IL_TGA_AUTHCOMMENT_STRING:
      iTrace("---- IL_TGA_AUTHCOMMENT_STRING is obsolete, use IL_META_USER_COMMENT instead");
      iSetMetaString(BaseImage, IL_META_USER_COMMENT, String_); return;

    case IL_TIF_HOSTCOMPUTER_STRING:
      iTrace("---- IL_TIF_HOSTCOMPUTER_STRING is obsolete, use IL_META_HOST_COMPUTER instead");
      iSetMetaString(BaseImage, IL_META_HOST_COMPUTER, String_); return;

    case IL_CHEAD_HEADER_STRING:
      iTrace("---- IL_CHEAD_HEADER_STRING is obsolete, use IL_META_DOCUMENT_NAME instead");
      iSetMetaString(BaseImage, IL_META_DOCUMENT_NAME, String_); return;

    default:
      iSetMetaString(BaseImage, Mode, String_);
      //iSetError(IL_INVALID_ENUM);
  }

  ifree(String);
}

void ILAPIENTRY iSetiv(ILimage *CurImage, ILenum Mode, const ILint *Param, ILuint Count) {
  IL_STATE_STRUCT * StateStruct   = iGetStateStruct();
  IL_STATES *       ilStates      = StateStruct->ilStates;
  ILuint            ilCurrentPos  = StateStruct->ilCurrentPos;
  ILimage *         BaseImage     = CurImage->BaseImage;

  (void) Count;

  if (!Param) {
    iSetError(IL_INVALID_PARAM);
    return;
  }

  switch (Mode)
  {
    // Integer values
    case IL_FORMAT_MODE:      iFormatFunc(*Param);       return;
    case IL_ORIGIN_MODE:      iOriginFunc(*Param);       return;
    case IL_TYPE_MODE:        iTypeFunc(*Param);         return;
    case IL_KEEP_DXTC_DATA:   ilStates[ilCurrentPos].ilKeepDxtcData = !!*Param; return;

    case IL_MAX_QUANT_INDICES:
      if (*Param >= 2 && *Param <= 256) {
        ilStates[ilCurrentPos].ilQuantMaxIndexs = *Param;
        return;
      }
      break;
    case IL_NEU_QUANT_SAMPLE:
      if (*Param >= 1 && *Param <= 30) {
        ilStates[ilCurrentPos].ilNeuSample = *Param;
        return;
      }
      break;
    case IL_QUANTIZATION_MODE:
      if (*Param == IL_WU_QUANT || *Param == IL_NEU_QUANT) {
        ilStates[ilCurrentPos].ilQuantMode = *Param;
        return;
      }
      break;

    // Image specific values
    case IL_IMAGE_DURATION:
      if (CurImage == NULL) {
        iSetError(IL_ILLEGAL_OPERATION);
        break;
      }
      CurImage->Duration = *Param;
      return;
    case IL_IMAGE_OFFX:
      if (CurImage == NULL) {
        iSetError(IL_ILLEGAL_OPERATION);
        break;
      }
      CurImage->OffX = *Param;
      return;
    case IL_IMAGE_OFFY:
      if (CurImage == NULL) {
        iSetError(IL_ILLEGAL_OPERATION);
        break;
      }
      CurImage->OffY = *Param;
      return;
    case IL_IMAGE_CUBEFLAGS:
      if (CurImage == NULL) {
        iSetError(IL_ILLEGAL_OPERATION);
        break;
      }
      CurImage->CubeFlags = *Param;
      break;
 
    // Format specific values
    case IL_BMP_RLE:
      if (*Param == IL_FALSE || *Param == IL_TRUE) {
        ilStates[ilCurrentPos].ilBmpRle = *Param;
        return;
      }
      break;
    case IL_DXTC_FORMAT:
      if (*Param >= IL_DXT1 || *Param <= IL_DXT5 || *Param == IL_DXT1A) {
        ilStates[ilCurrentPos].ilDxtcFormat = *Param;
        return;
      }
      break;
    case IL_JPG_SAVE_FORMAT:
      if (*Param == IL_JFIF || *Param == IL_EXIF) {
        ilStates[ilCurrentPos].ilJpgFormat = *Param;
        return;
      }
      break;
    case IL_JPG_QUALITY:
      if (*Param >= 0 && *Param <= 99) {
        ilStates[ilCurrentPos].ilJpgQuality = *Param;
        return;
      }
      break;
    case IL_PNG_INTERLACE:
      if (*Param == IL_FALSE || *Param == IL_TRUE) {
        ilStates[ilCurrentPos].ilPngInterlace = *Param;
        return;
      }
      break;
    case IL_PCD_PICNUM:
      if (*Param >= 0 || *Param <= 2) {
        ilStates[ilCurrentPos].ilPcdPicNum = *Param;
        return;
      }
      break;
    case IL_PNG_ALPHA_INDEX:
      if (*Param >= -1 || *Param <= 255) {
        ilStates[ilCurrentPos].ilPngAlphaIndex=*Param;
        return;
      }
      break;
    case IL_SGI_RLE:
      if (*Param == IL_FALSE || *Param == IL_TRUE) {
        ilStates[ilCurrentPos].ilSgiRle = *Param;
        return;
      }
      break;
    case IL_TGA_CREATE_STAMP:
      if (*Param == IL_FALSE || *Param == IL_TRUE) {
        ilStates[ilCurrentPos].ilTgaCreateStamp = *Param;
        return;
      }
      break;
    case IL_TGA_RLE:
      if (*Param == IL_FALSE || *Param == IL_TRUE) {
        ilStates[ilCurrentPos].ilTgaRle = *Param;
        return;
      }
      break;
    case IL_VTF_COMP:
      if (*Param == IL_DXT1 || *Param == IL_DXT5 || *Param == IL_DXT3 || *Param == IL_DXT1A || *Param == IL_DXT_NO_COMP) {
        ilStates[ilCurrentPos].ilVtfCompression = *Param;
        return;
      }
      break;
    case IL_FILE_MODE: 
    case IL_FILE_OVERWRITE: // deprecated: unused, IL_FILE_MODE is used instead
      ilStates[ilCurrentPos].ilOverWriteFiles = !!*Param;
      return;

    case IL_IMAGE_SELECTION_MODE:
      if (*Param == IL_RELATIVE || *Param == IL_ABSOLUTE) {
        ilStates[ilCurrentPos].ilImageSelectionMode = *Param;
        return;
      }
      break;

    default:
      iSetMetaiv(BaseImage, Mode, Param);
  }
}

ILint iGetInt(ILenum Mode) {
  //like ilGetInteger(), but sets another error on failure

  //call ilGetIntegerv() for more robust code
  ILenum err;
  ILint r = -1;

  iGetiv(NULL, Mode, &r, 1);

  //check if an error occured, set another error
  err = ilGetError();
  if (r == -1 && err == IL_INVALID_ENUM) {
    iSetErrorReal(IL_INTERNAL_ERROR);
  } else {
    iSetErrorReal(err); //restore error
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

  *Red    = ilStates[ilCurrentPos].ilKeyColourRed;
  *Green  = ilStates[ilCurrentPos].ilKeyColourGreen;
  *Blue   = ilStates[ilCurrentPos].ilKeyColourBlue;
  *Alpha  = ilStates[ilCurrentPos].ilKeyColourAlpha; 
}


void iClearColour(ILclampf Red, ILclampf Green, ILclampf Blue, ILclampf Alpha) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;

  // Clamp to 0.0f - 1.0f.
  ilStates[ilCurrentPos].ClearColour[0] = IL_CLAMP(Red);
  ilStates[ilCurrentPos].ClearColour[1] = IL_CLAMP(Green);
  ilStates[ilCurrentPos].ClearColour[2] = IL_CLAMP(Blue);
  ilStates[ilCurrentPos].ClearColour[3] = IL_CLAMP(Alpha);
}

void iClearIndex(ILuint Index) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;

  // Clamp to 0.0f - 1.0f.
  ilStates[ilCurrentPos].ClearIndex = Index;
}


ILAPI void ILAPIENTRY iGetClear(void *Colours, ILenum Format, ILenum Type) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;

  const void *From = ilStates[ilCurrentPos].ClearColour;
  const void *To   = Colours;
  
  switch (Type) {
    case IL_BYTE:
    case IL_UNSIGNED_BYTE:
      switch (Format) {
        case IL_RGB:              iPixelConv3f(     ILfloat, From, 1.0f, ILubyte, To, IL_MAX_UNSIGNED_BYTE); break;
        case IL_RGBA:             iPixelConv4f(     ILfloat, From, 1.0f, ILubyte, To, IL_MAX_UNSIGNED_BYTE); break;
        case IL_BGR:              iPixelConv3Swapf( ILfloat, From, 1.0f, ILubyte, To, IL_MAX_UNSIGNED_BYTE); break;
        case IL_BGRA:             iPixelConv4Swapf( ILfloat, From, 1.0f, ILubyte, To, IL_MAX_UNSIGNED_BYTE); break;
        case IL_LUMINANCE:        iPixelConv3Lf(    ILfloat, From, 1.0f, ILubyte, To, IL_MAX_UNSIGNED_BYTE); break;
        case IL_LUMINANCE_ALPHA:  iPixelConv4LAf(   ILfloat, From, 1.0f, ILubyte, To, IL_MAX_UNSIGNED_BYTE); break;
        case IL_COLOUR_INDEX:     (*(ILubyte*)Colours) = ilStates[ilCurrentPos].ClearIndex; break;
        default:                  iSetError(IL_INTERNAL_ERROR); return;
      }
      break;
      
    case IL_SHORT:
    case IL_UNSIGNED_SHORT:
      switch (Format) {
        case IL_RGB:              iPixelConv3f(     ILfloat, From, 1.0f, ILushort, To, IL_MAX_UNSIGNED_SHORT); break;
        case IL_RGBA:             iPixelConv4f(     ILfloat, From, 1.0f, ILushort, To, IL_MAX_UNSIGNED_SHORT); break;
        case IL_BGR:              iPixelConv3Swapf( ILfloat, From, 1.0f, ILushort, To, IL_MAX_UNSIGNED_SHORT); break;
        case IL_BGRA:             iPixelConv4Swapf( ILfloat, From, 1.0f, ILushort, To, IL_MAX_UNSIGNED_SHORT); break;
        case IL_LUMINANCE:        iPixelConv3Lf(    ILfloat, From, 1.0f, ILushort, To, IL_MAX_UNSIGNED_SHORT); break;
        case IL_LUMINANCE_ALPHA:  iPixelConv4LAf(   ILfloat, From, 1.0f, ILushort, To, IL_MAX_UNSIGNED_SHORT); break;
        case IL_COLOUR_INDEX:     (*(ILushort*)Colours) = ilStates[ilCurrentPos].ClearIndex; break;
        default:                  iSetError(IL_INTERNAL_ERROR); return;
      }
      break;
      
    case IL_INT:
    case IL_UNSIGNED_INT:
      switch (Format) {
        case IL_RGB:              iPixelConv3f(     ILfloat, From, 1.0f, ILuint, To, IL_MAX_UNSIGNED_INT); break;
        case IL_RGBA:             iPixelConv4f(     ILfloat, From, 1.0f, ILuint, To, IL_MAX_UNSIGNED_INT); break;
        case IL_BGR:              iPixelConv3Swapf( ILfloat, From, 1.0f, ILuint, To, IL_MAX_UNSIGNED_INT); break;
        case IL_BGRA:             iPixelConv4Swapf( ILfloat, From, 1.0f, ILuint, To, IL_MAX_UNSIGNED_INT); break;
        case IL_LUMINANCE:        iPixelConv3Lf(    ILfloat, From, 1.0f, ILuint, To, IL_MAX_UNSIGNED_INT); break;
        case IL_LUMINANCE_ALPHA:  iPixelConv4LAf(   ILfloat, From, 1.0f, ILuint, To, IL_MAX_UNSIGNED_INT); break;
        case IL_COLOUR_INDEX:     (*(ILuint*)Colours) = ilStates[ilCurrentPos].ClearIndex; break;
        default:                  iSetError(IL_INTERNAL_ERROR); return;
      }
      break;
      
    case IL_FLOAT:
      switch (Format) {
        case IL_RGB:              iPixelConv3f(     ILfloat, From, 1.0f, ILfloat, To, 1.0f); break;
        case IL_RGBA:             iPixelConv4f(     ILfloat, From, 1.0f, ILfloat, To, 1.0f); break;
        case IL_BGR:              iPixelConv3Swapf( ILfloat, From, 1.0f, ILfloat, To, 1.0f); break;
        case IL_BGRA:             iPixelConv4Swapf( ILfloat, From, 1.0f, ILfloat, To, 1.0f); break;
        case IL_LUMINANCE:        iPixelConv3Lf(    ILfloat, From, 1.0f, ILfloat, To, 1.0f); break;
        case IL_LUMINANCE_ALPHA:  iPixelConv4LAf(   ILfloat, From, 1.0f, ILfloat, To, 1.0f); break;
        case IL_COLOUR_INDEX:     
        default:                  iSetError(IL_INTERNAL_ERROR); return;
      }        
      break;

    case IL_DOUBLE:
      switch (Format) {
        case IL_RGB:              iPixelConv3f(     ILfloat, From, 1.0f, ILdouble, To, 1.0); break;
        case IL_RGBA:             iPixelConv4f(     ILfloat, From, 1.0f, ILdouble, To, 1.0); break;
        case IL_BGR:              iPixelConv3Swapf( ILfloat, From, 1.0f, ILdouble, To, 1.0); break;
        case IL_BGRA:             iPixelConv4Swapf( ILfloat, From, 1.0f, ILdouble, To, 1.0); break;
        case IL_LUMINANCE:        iPixelConv3Lf(    ILfloat, From, 1.0f, ILdouble, To, 1.0); break;
        case IL_LUMINANCE_ALPHA:  iPixelConv4LAf(   ILfloat, From, 1.0f, ILdouble, To, 1.0); break;
        case IL_COLOUR_INDEX:     
        default:                  iSetError(IL_INTERNAL_ERROR); return;
      }        
     
    default:
      iSetError(IL_INTERNAL_ERROR);
      return;
  }
}
