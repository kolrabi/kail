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
#include "conv/il_convert.h"

#include <stdlib.h>

// Global variables
static ILconst_string _ilVendor    = IL_TEXT("kolrabi");
static ILconst_string _ilVersion   = IL_TEXT("kolrabi's another Image Library (kaIL) 1.11.0");

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

  ilStates[ilCurrentPos].ilStateFlags       = IL_STATE_FLAG_BLIT_BLEND_ALPHA;
  ilStates[ilCurrentPos].ilOriginMode       = IL_ORIGIN_LOWER_LEFT;
  ilStates[ilCurrentPos].ilFormatMode       = IL_BGRA;
  ilStates[ilCurrentPos].ilTypeMode         = IL_UNSIGNED_BYTE;
  ilStates[ilCurrentPos].ilKeyColourRed     = 0;
  ilStates[ilCurrentPos].ilKeyColourGreen   = 0;
  ilStates[ilCurrentPos].ilKeyColourBlue    = 0;
  ilStates[ilCurrentPos].ilKeyColourAlpha   = 0; 
  ilStates[ilCurrentPos].ilCompression      = IL_COMPRESS_ZLIB;

  // Changed to the colour of the Universe
  //  (http://www.newscientist.com/news/news.jsp?id=ns99991775)
  //  *(http://www.space.com/scienceastronomy/universe_color_020308.html)*
  ilStates[ilCurrentPos].ClearColour[0] = 1.0f;
  ilStates[ilCurrentPos].ClearColour[1] = 0.972549f;
  ilStates[ilCurrentPos].ClearColour[2] = 0.90588f;
  ilStates[ilCurrentPos].ClearColour[3] = 0.0f;
  ilStates[ilCurrentPos].ClearIndex     = 0;

  ilStates[ilCurrentPos].ilJpgQuality       = 99;
  ilStates[ilCurrentPos].ilJpgFormat        = IL_JFIF;
  ilStates[ilCurrentPos].ilDxtcFormat       = IL_DXT1;
  ilStates[ilCurrentPos].ilPcdPicNum        = 2;
  ilStates[ilCurrentPos].ilPngAlphaIndex    = -1;
  ilStates[ilCurrentPos].ilVtfCompression   = IL_DXT_NO_COMP;

  ilStates[ilCurrentPos].ilCHeader          = NULL;

  ilStates[ilCurrentPos].ilQuantMode        = IL_WU_QUANT;
  ilStates[ilCurrentPos].ilNeuSample        = 15;
  ilStates[ilCurrentPos].ilQuantMaxIndexs   = 256;

  ilStates[ilCurrentPos].ilImageSelectionMode = IL_RELATIVE;

  StateStruct->ilHints.MemVsSpeedHint = IL_FASTEST;
  StateStruct->ilHints.CompressHint   = IL_USE_COMPRESSION;

  // clear errors
  while (iGetError() != IL_NO_ERROR)
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

#define SETFLAG(x, b, v) (x) = ((x) & (~(ILuint)(b))) | ((v)?(b):0)
#define GETFLAG(x, b)    (!!((x) & (ILuint)(b)))

// Internal function that sets the Mode equal to Flag
ILboolean iAble(ILenum Mode, ILboolean Flag) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;

  switch (Mode) {
    case IL_ORIGIN_SET:       SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_ORIGIN_SET, Flag); break;
    case IL_FORMAT_SET:       SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_FORMAT_SET, Flag); break;
    case IL_TYPE_SET:         SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_TYPE_SET,   Flag); break;

    case IL_FILE_OVERWRITE:   // deprecated: unused, IL_FILE_MODE is used instead
    case IL_FILE_MODE:        SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_OVERWRITE_FILES, Flag); break;
    case IL_BLIT_BLEND:       SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_BLIT_BLEND_ALPHA, Flag); break;
    case IL_CONV_PAL:         SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_AUTO_CONVERT_PALETTE, Flag); break;
    case IL_DEFAULT_ON_FAIL:  SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_RETURN_DEFAULT_IMAGE_ON_FAIL, Flag); break;
    case IL_USE_KEY_COLOUR:   SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_USE_KEY_COLOUR, Flag); break;

    case IL_PNG_INTERLACE: 
    case IL_SAVE_INTERLACED:  SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_INTERLACE, Flag); break;
    case IL_KEEP_DXTC_DATA:   SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_KEEP_DXTC_DATA, Flag); break;
    case IL_NVIDIA_COMPRESS:  
    case IL_SQUISH_COMPRESS:  SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_USE_SQUISH_DXT, Flag); break;
    case IL_TGA_RLE:          SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_TGA_USE_RLE, Flag); break;
    case IL_BMP_RLE:          SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_BMP_USE_RLE, Flag); break;
    case IL_SGI_RLE:          SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_SGI_USE_RLE, Flag); break;
    case IL_JPG_PROGRESSIVE:  SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_JPG_PROGRESSIVE, Flag); break;

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
    case IL_ORIGIN_SET:       return GETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_ORIGIN_SET);
    case IL_FORMAT_SET:       return GETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_FORMAT_SET);
    case IL_TYPE_SET:         return GETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_TYPE_SET);

    case IL_FILE_OVERWRITE:   // deprecated: unused, IL_FILE_MODE is used instead
    case IL_FILE_MODE:        return GETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_OVERWRITE_FILES);
    case IL_BLIT_BLEND:       return GETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_BLIT_BLEND_ALPHA);
    case IL_CONV_PAL:         return GETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_AUTO_CONVERT_PALETTE);
    case IL_DEFAULT_ON_FAIL:  return GETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_RETURN_DEFAULT_IMAGE_ON_FAIL);
    case IL_USE_KEY_COLOUR:   return GETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_USE_KEY_COLOUR);

    case IL_PNG_INTERLACE:
    case IL_SAVE_INTERLACED:  return GETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_INTERLACE);
    case IL_KEEP_DXTC_DATA:   return GETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_KEEP_DXTC_DATA);
    case IL_NVIDIA_COMPRESS:  
    case IL_SQUISH_COMPRESS:  return GETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_USE_SQUISH_DXT);
    case IL_TGA_RLE:          return GETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_TGA_USE_RLE);
    case IL_BMP_RLE:          return GETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_BMP_USE_RLE);
    case IL_SGI_RLE:          return GETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_SGI_USE_RLE);
    case IL_JPG_PROGRESSIVE:  return GETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_JPG_PROGRESSIVE);

    default:
      iSetError(IL_INVALID_ENUM);
  }

  return IL_FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// Integer
///

ILuint ILAPIENTRY iGetiv(ILimage *Image, ILenum Mode, ILint *Param, ILint MaxCount) {
  IL_STATE_STRUCT *     StateStruct   = iGetStateStruct();
  IL_IMAGE_SELECTION *  Selection     = &iGetTLSData()->CurSel;
  IL_STATES *           ilStates      = StateStruct->ilStates;
  ILuint                ilCurrentPos  = StateStruct->ilCurrentPos;
  ILimage *             SubImage      = NULL;

  switch (Mode) {
    // bools
    case IL_ORIGIN_SET:       
    case IL_FORMAT_SET:       
    case IL_TYPE_SET:         

    case IL_FILE_OVERWRITE:   
    case IL_FILE_MODE:        
    case IL_BLIT_BLEND:       
    case IL_CONV_PAL:         
    case IL_DEFAULT_ON_FAIL:  
    case IL_USE_KEY_COLOUR:   

    case IL_KEEP_DXTC_DATA:
    case IL_PNG_INTERLACE:
    case IL_SAVE_INTERLACED:  
    case IL_NVIDIA_COMPRESS:  
    case IL_SQUISH_COMPRESS:  
    case IL_TGA_RLE:          
    case IL_BMP_RLE:
    case IL_SGI_RLE:          
    case IL_JPG_PROGRESSIVE:      if (Param) *Param = iIsEnabled(Mode); return 1;

    // real integers
    case IL_CUR_IMAGE:
      if (Image == NULL) {
        iSetError(IL_ILLEGAL_OPERATION);
        return 0;
      }
      if (Param) *Param = (ILint)Selection->CurName;
      return 1;

    case IL_COMPRESS_MODE:        if (Param) *Param = (ILint)ilStates[ilCurrentPos].ilCompression; return 1;
    case IL_FORMAT_MODE:          if (Param) *Param = (ILint)ilStates[ilCurrentPos].ilFormatMode; return 1; 
    case IL_ORIGIN_MODE:          if (Param) *Param = (ILint)ilStates[ilCurrentPos].ilOriginMode; return 1;
    case IL_MAX_QUANT_INDICES:    if (Param) *Param = (ILint)ilStates[ilCurrentPos].ilQuantMaxIndexs; return 1;
    case IL_NEU_QUANT_SAMPLE:     if (Param) *Param = (ILint)ilStates[ilCurrentPos].ilNeuSample; return 1;
    case IL_QUANTIZATION_MODE:    if (Param) *Param = (ILint)ilStates[ilCurrentPos].ilQuantMode; return 1;
    case IL_TYPE_MODE:            if (Param) *Param = (ILint)ilStates[ilCurrentPos].ilTypeMode; return 1;
    case IL_VERSION_NUM:          if (Param) *Param = (ILint)IL_VERSION; return 1;

    case IL_ACTIVE_IMAGE:         if (Param) *Param = (ILint)Selection->CurFrame; return 1;
    case IL_ACTIVE_MIPMAP:        if (Param) *Param = (ILint)Selection->CurMipmap; return 1;
    case IL_ACTIVE_LAYER:         if (Param) *Param = (ILint)Selection->CurLayer; return 1;
    case IL_ACTIVE_FACE:          if (Param) *Param = (ILint)Selection->CurFace; return 1;

    case IL_DXTC_FORMAT:          if (Param) *Param = (ILint)ilStates[ilCurrentPos].ilDxtcFormat; return 1;
    case IL_JPG_QUALITY:          if (Param) *Param = (ILint)ilStates[ilCurrentPos].ilJpgQuality; return 1;
    case IL_JPG_SAVE_FORMAT:      if (Param) *Param = (ILint)ilStates[ilCurrentPos].ilJpgFormat; return 1;
    case IL_PCD_PICNUM:           if (Param) *Param = (ILint)ilStates[ilCurrentPos].ilPcdPicNum; return 1;
    case IL_PNG_ALPHA_INDEX:      if (Param) *Param = (ILint)ilStates[ilCurrentPos].ilPngAlphaIndex; return 1;
    case IL_VTF_COMP:             if (Param) *Param = (ILint)ilStates[ilCurrentPos].ilVtfCompression; return 1;

    case IL_IMAGE_SELECTION_MODE: if (Param) *Param = (ILint)ilStates[ilCurrentPos].ilImageSelectionMode; return 1;

    case IL_DXTC_DATA_FORMAT:
      if (Image->DxtcData == NULL || Image->DxtcSize == 0) {
        *Param = (ILint)IL_DXT_NO_COMP;
      } else {
        *Param = (ILint)Image->DxtcFormat;
      }
      return 1;

    case IL_NUM_FACES: 
      *Param = 0;
      for (SubImage = Image->Faces; SubImage; SubImage = SubImage->Faces) (*Param)++;
      return 1;

    case IL_NUM_IMAGES: 
      *Param = 0;
      if (ilStates[ilCurrentPos].ilImageSelectionMode == IL_RELATIVE)
        for (SubImage = Image->Next; SubImage; SubImage = SubImage->Next) (*Param)++;
      else
        for (SubImage = Image->BaseImage; SubImage; SubImage = SubImage->Next) (*Param)++;
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
    case IL_IMAGE_BITS_PER_PIXEL:         *Param = (ILint)((Image->Bpp << 3)*Image->Bpc); return 1;

    //changed 20040610 to channel count (Bpp) times Bytes per channel
    case IL_IMAGE_BYTES_PER_PIXEL:        *Param = (ILint)(Image->Bpp*Image->Bpc); return 1;

    case IL_IMAGE_BPC:          *Param = (ILint)Image->Bpc; return 1;
    case IL_IMAGE_CHANNELS:     *Param = (ILint)Image->Bpp; return 1;
    case IL_IMAGE_CUBEFLAGS:    *Param = (ILint)Image->CubeFlags; return 1;
    case IL_IMAGE_DEPTH:        *Param = (ILint)Image->Depth; return 1;
    case IL_IMAGE_DURATION:     *Param = (ILint)Image->Duration; return 1;
    case IL_IMAGE_FORMAT:       *Param = (ILint)Image->Format; return 1;
    case IL_IMAGE_HEIGHT:       *Param = (ILint)Image->Height; return 1;
    case IL_IMAGE_SIZE_OF_DATA: *Param = (ILint)Image->SizeOfData; return 1;
    case IL_IMAGE_OFFX:         *Param = (ILint)Image->OffX; return 1;
    case IL_IMAGE_OFFY:         *Param = (ILint)Image->OffY; return 1;
    case IL_IMAGE_ORIGIN:       *Param = (ILint)Image->Origin; return 1;
    case IL_IMAGE_PLANESIZE:    *Param = (ILint)Image->SizeOfPlane; return 1;
    case IL_IMAGE_TYPE:         *Param = (ILint)Image->Type; return 1;
    case IL_IMAGE_WIDTH:        *Param = (ILint)Image->Width; return 1;
    case IL_PALETTE_TYPE:       *Param = (ILint)Image->Pal.PalType; return 1;
    case IL_PALETTE_BPP:        *Param = (ILint)iGetBppPal(Image->Pal.PalType); return 1;
    case IL_PALETTE_BASE_TYPE:  *Param = (ILint)iGetPalBaseType(Image->Pal.PalType); return 1;
    case IL_PALETTE_NUM_COLS:
      if (!Image->Pal.Palette || !Image->Pal.PalSize || Image->Pal.PalType == IL_PAL_NONE)
        *Param = 0;
      else 
        *Param = (ILint)(Image->Pal.PalSize / iGetBppPal(Image->Pal.PalType));
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
  return iGetMetaiv(Image, Mode, Param, MaxCount);
}

ILAPI ILint ILAPIENTRY iGetInteger(ILimage *Image, ILenum Mode)
{
  ILint Data = 0;
  iGetiv(Image, Mode, &Data, 1);
  return Data;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// Float
///

ILuint ILAPIENTRY iGetfv(ILimage *Image, ILenum Mode, ILfloat *Param, ILint MaxCount) {
  return iGetMetafv(Image, Mode, Param, MaxCount);
}

void ILAPIENTRY iSetfv(ILimage *Image, ILenum Mode, const ILfloat *Param) {
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
    SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_ORIGIN_SET, GETFLAG(ilStates[ilCurrentPos-1].ilOriginMode, IL_STATE_FLAG_ORIGIN_SET));
  }
  if (Bits & IL_FORMAT_BIT) {
    ilStates[ilCurrentPos].ilFormatMode = ilStates[ilCurrentPos-1].ilFormatMode;
    SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_FORMAT_SET, GETFLAG(ilStates[ilCurrentPos-1].ilOriginMode, IL_STATE_FLAG_FORMAT_SET));
  }
  if (Bits & IL_TYPE_BIT) {
    ilStates[ilCurrentPos].ilTypeMode = ilStates[ilCurrentPos-1].ilTypeMode;
    SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_TYPE_SET, GETFLAG(ilStates[ilCurrentPos-1].ilOriginMode, IL_STATE_FLAG_TYPE_SET));
  }
  if (Bits & IL_FILE_BIT) {
    SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_OVERWRITE_FILES, GETFLAG(ilStates[ilCurrentPos-1].ilOriginMode, IL_STATE_FLAG_OVERWRITE_FILES));
  }
  if (Bits & IL_PAL_BIT) {
    SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_AUTO_CONVERT_PALETTE, GETFLAG(ilStates[ilCurrentPos-1].ilOriginMode, IL_STATE_FLAG_AUTO_CONVERT_PALETTE));
  }
  if (Bits & IL_LOADFAIL_BIT) {
    SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_RETURN_DEFAULT_IMAGE_ON_FAIL, GETFLAG(ilStates[ilCurrentPos-1].ilOriginMode, IL_STATE_FLAG_RETURN_DEFAULT_IMAGE_ON_FAIL));
  }
  if (Bits & IL_COMPRESS_BIT) {
    ilStates[ilCurrentPos].ilCompression = ilStates[ilCurrentPos-1].ilCompression;
  }
  if (Bits & IL_FORMAT_SPECIFIC_BIT) {
    SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_INTERLACE, GETFLAG(ilStates[ilCurrentPos-1].ilOriginMode, IL_STATE_FLAG_INTERLACE));
    SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_TGA_USE_RLE, GETFLAG(ilStates[ilCurrentPos-1].ilOriginMode, IL_STATE_FLAG_TGA_USE_RLE));
    SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_BMP_USE_RLE, GETFLAG(ilStates[ilCurrentPos-1].ilOriginMode, IL_STATE_FLAG_BMP_USE_RLE));
    SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_SGI_USE_RLE, GETFLAG(ilStates[ilCurrentPos-1].ilOriginMode, IL_STATE_FLAG_SGI_USE_RLE));
    SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_JPG_PROGRESSIVE, GETFLAG(ilStates[ilCurrentPos-1].ilOriginMode, IL_STATE_FLAG_JPG_PROGRESSIVE));

    ilStates[ilCurrentPos].ilJpgQuality = ilStates[ilCurrentPos-1].ilJpgQuality;
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
    SETFLAG(ilStates[ilCurrentPos].ilStateFlags, IL_STATE_FLAG_USE_KEY_COLOUR, GETFLAG(ilStates[ilCurrentPos-1].ilOriginMode, IL_STATE_FLAG_USE_KEY_COLOUR));
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

  if (ilCurrentPos == 0) {
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


void iSetStringMB(ILimage *Image, ILenum Mode, const char *String_) {
  ILchar *String;
#ifdef _UNICODE
  String = iWideFromMultiByte(String_);
#else
  String = iCharStrDup(String_);
#endif
  iSetString(Image, Mode, String);
  ifree(String);
}


void iSetString(ILimage *Image, ILenum Mode, ILconst_string String_) {
  ILimage *BaseImage = Image->BaseImage;

  if (String_ == NULL) {
    iSetError(IL_INVALID_PARAM);
    return;
  }

  switch (Mode)
  {
    case IL_TGA_ID_STRING:            
      iTrace("---- IL_TGA_ID_STRING is obsolete, use IL_META_DOCUMENT_NAME instead");
      iSetMetaString(BaseImage, IL_META_DOCUMENT_NAME, String_); 
      return;

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
}

void ILAPIENTRY iSetiv(ILimage *CurImage, ILenum Mode, const ILint *Param) {
  IL_STATE_STRUCT * StateStruct   = iGetStateStruct();
  IL_STATES *       ilStates      = StateStruct->ilStates;
  ILuint            ilCurrentPos  = StateStruct->ilCurrentPos;
  ILimage *         BaseImage     = CurImage ? CurImage->BaseImage : NULL;

  if (!Param) {
    iSetError(IL_INVALID_PARAM);
    return;
  }

  switch (Mode)
  {
    // Integer values
    case IL_FORMAT_MODE:      iFormatFunc((ILenum)*Param);        return;
    case IL_ORIGIN_MODE:      iOriginFunc((ILenum)*Param);        return;
    case IL_TYPE_MODE:        iTypeFunc((ILenum)*Param);          return;

    case IL_MAX_QUANT_INDICES:
      if (*Param >= 2 && *Param <= 256) {
        ilStates[ilCurrentPos].ilQuantMaxIndexs = (ILuint)*Param;
        return;
      }
      break;
    case IL_NEU_QUANT_SAMPLE:
      if (*Param >= 1 && *Param <= 30) {
        ilStates[ilCurrentPos].ilNeuSample = (ILuint)*Param;
        return;
      }
      break;
    case IL_QUANTIZATION_MODE:
      if (*Param == IL_WU_QUANT || *Param == IL_NEU_QUANT) {
        ilStates[ilCurrentPos].ilQuantMode = (ILenum)*Param;
        return;
      }
      break;

    // Image specific values
    case IL_IMAGE_DURATION:
      if (CurImage == NULL) {
        iSetError(IL_ILLEGAL_OPERATION);
        break;
      }
      CurImage->Duration = (ILuint)*Param;
      return;
    case IL_IMAGE_OFFX:
      if (CurImage == NULL) {
        iSetError(IL_ILLEGAL_OPERATION);
        break;
      }
      CurImage->OffX = (ILuint)*Param;
      return;
    case IL_IMAGE_OFFY:
      if (CurImage == NULL) {
        iSetError(IL_ILLEGAL_OPERATION);
        break;
      }
      CurImage->OffY = (ILuint)*Param;
      return;

    case IL_IMAGE_CUBEFLAGS:
      if (CurImage == NULL) {
        iSetError(IL_ILLEGAL_OPERATION);
        break;
      }
      CurImage->CubeFlags = (ILenum)*Param;
      break;
 
    // Format specific values
    case IL_DXTC_FORMAT:
      if ((*Param >= IL_DXT1 && *Param <= IL_DXT5) || *Param == IL_DXT1A) {
        ilStates[ilCurrentPos].ilDxtcFormat = (ILenum)*Param;
        return;
      }
      break;
    case IL_JPG_SAVE_FORMAT:
      if (*Param == IL_JFIF || *Param == IL_EXIF) {
        ilStates[ilCurrentPos].ilJpgFormat = (ILenum)*Param;
        return;
      }
      break;
    case IL_JPG_QUALITY:
      if (*Param >= 0 && *Param <= 99) {
        ilStates[ilCurrentPos].ilJpgQuality = (ILuint)*Param;
        return;
      }
      break;
    case IL_PCD_PICNUM:
      if (*Param >= 0 && *Param <= 2) {
        ilStates[ilCurrentPos].ilPcdPicNum = (ILuint)*Param;
        return;
      }
      break;
    case IL_PNG_ALPHA_INDEX:
      if (*Param >= -1 && *Param <= 255) {
        ilStates[ilCurrentPos].ilPngAlphaIndex = *Param;
        return;
      }
      break;
    case IL_VTF_COMP:
      if (*Param == IL_DXT1 || *Param == IL_DXT5 || *Param == IL_DXT3 || *Param == IL_DXT1A || *Param == IL_DXT_NO_COMP) {
        ilStates[ilCurrentPos].ilVtfCompression = (ILenum)*Param;
        return;
      }
      break;

    case IL_IMAGE_SELECTION_MODE:
      if (*Param == IL_RELATIVE || *Param == IL_ABSOLUTE) {
        ilStates[ilCurrentPos].ilImageSelectionMode = (ILenum)*Param;
        return;
      }
      break;

    case IL_FILE_MODE: 
    case IL_FILE_OVERWRITE: // deprecated: unused, IL_FILE_MODE is used instead
    case IL_BMP_RLE:
    case IL_PNG_INTERLACE:
    case IL_SAVE_INTERLACED:
    case IL_SGI_RLE:
    case IL_TGA_RLE:
    case IL_KEEP_DXTC_DATA:   iAble(Mode, !!*Param);              return;

    default:
      if (CurImage == NULL) {
        iSetError(IL_ILLEGAL_OPERATION);
        break;
      }
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
  err = iGetError();
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


ILAPI void ILAPIENTRY iGetClear(void *Colour, ILenum Format, ILenum Type) {
  IL_STATE_STRUCT *StateStruct  = iGetStateStruct();
  IL_STATES *ilStates = StateStruct->ilStates;
  ILuint ilCurrentPos = StateStruct->ilCurrentPos;

  void * From     = ilStates[ilCurrentPos].ClearColour;
  ILuint FromSize = sizeof(ilStates[ilCurrentPos].ClearColour);

  void *Conv = iConvertBuffer(FromSize, IL_RGBA, Format, IL_FLOAT, Type, NULL, From);
  if (Conv) {
    ILuint Size = iGetBppFormat(Format) * iGetBpcType(Type);
    memcpy(Colour, From, Size);
    if (Conv != From)
      ifree(Conv);
  } else {
    iSetError(IL_INTERNAL_ERROR);
  }
}
