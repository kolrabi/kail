//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 2014-05-28 by Björn Paetzel
//
// Filename: src/IL/il_io.c
//
// Description: Determines image types and loads/saves images
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#include "il_register.h"
#include "il_pal.h"
#include <string.h>

#include "il_formats.h"

ILboolean iDefaultImage(ILimage *Image);

// Returns a widened version of a string.
// Make sure to free this after it is used.  Code help from
//  https://buildsecurityin.us-cert.gov/daisy/bsi-rules/home/g1/769-BSI.html
wchar_t * ILAPIENTRY iWideFromMultiByte(const char *Multi)
{
  ILsizei Length;
  wchar_t *Temp;

  if (Multi == NULL)
    return NULL;

  Length = mbstowcs(NULL, Multi, 0) + 1; // note error return of -1 is possible
  if (Length == 0) {
    iSetError(IL_INVALID_PARAM);
    return NULL;
  }
  if (Length > ULONG_MAX/sizeof(wchar_t)) {
    iSetError(IL_INTERNAL_ERROR);
    return NULL;
  }
  Temp = (wchar_t*)ialloc(Length * sizeof(wchar_t));
  mbstowcs(Temp, Multi, Length); 

  return Temp;
}

char * ILAPIENTRY iMultiByteFromWide(const wchar_t *Wide)
{
  ILsizei Length;
  char  * Temp;

  if (Wide == NULL)
    return NULL;

  Length = wcstombs(NULL, Wide, 0) + 1; // note error return of -1 is possible
  if (Length == 0) {
    iSetError(IL_INVALID_PARAM);
    return NULL;
  }

  Temp = (char*)ialloc(Length);
  wcstombs(Temp, Wide, Length);

  return Temp;
}

ILenum ILAPIENTRY iTypeFromExt(ILconst_string FileName) {
  ILenum    Type;
  ILstring  Ext;

  if (FileName == NULL || iStrLen(FileName) < 1) {
    iSetError(IL_INVALID_PARAM);
    return IL_TYPE_UNKNOWN;
  }

  Ext = iGetExtension(FileName);
  //added 2003-08-31: fix sf bug 789535
  if (Ext == NULL) {
    return IL_TYPE_UNKNOWN;
  }

  else if (!iStrCmp(Ext, IL_TEXT("wdp")) || !iStrCmp(Ext, IL_TEXT("hdp")))
    Type = IL_WDP;
  else
    Type = iIdentifyFormatExt(Ext);

  return Type;
}


ILenum iDetermineType(ILimage *Image, ILconst_string FileName) {
  ILenum Type = IL_TYPE_UNKNOWN;

  // TODO check Image

  if (FileName != NULL) {
    // If we can open the file, determine file type from contents
    // This is more reliable than the file name extension 

    if (SIOopenRO(&Image->io, FileName)) {
      Type = iDetermineTypeFuncs(Image);
      SIOclose(&Image->io);
    }

    if (Type == IL_TYPE_UNKNOWN)
      Type = iTypeFromExt(FileName);
  }

  return Type;
}

ILenum iDetermineTypeFuncs(ILimage *Image) {
  return iIdentifyFormat(&Image->io);
}

ILboolean iIsValidIO(ILenum Type, SIO* io) {
  const ILformat *format = iGetFormat(Type);

  if (io == NULL) {
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }

  if (format) {
    return format->Validate && format->Validate(io);
  }

  iSetError(IL_INVALID_ENUM);
  return IL_FALSE;
}

ILboolean ILAPIENTRY iLoad(ILimage *Image, ILenum Type, ILconst_string FileName) {
  if (FileName == NULL || iStrLen(FileName) < 1) {
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }

  if ( Image == NULL
    || Image->io.openReadOnly == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (SIOopenRO(&Image->io, FileName)) {
    ILboolean bRet;
    if (Type == IL_TYPE_UNKNOWN)
      Type = iDetermineTypeFuncs(Image);
    if (Type == IL_TYPE_UNKNOWN)
      Type = iTypeFromExt(FileName);

    bRet = iLoadFuncs2(Image, Type);

    SIOclose(&Image->io);

    if (Type == IL_CUT) {
      // Attempt to load the palette
      size_t fnLen = iStrLen(FileName);
      if (fnLen > 4) {
        if (FileName[fnLen-4] == '.'
        &&  FileName[fnLen-3] == 'c'
        &&  FileName[fnLen-2] == 'u'
        &&  FileName[fnLen-1] == 't') {
          ILchar* palFN = (ILchar*) ialloc(sizeof(ILchar)*fnLen+2);
          iStrCpy(palFN, FileName);
          iStrCpy(&palFN[fnLen-3], IL_TEXT("pal"));
          iLoad(Image, IL_HALO_PAL, palFN);
          ifree(palFN);
        }
      }
    } 

    return bRet;
  } else {
    iTrace("***** Could not open file "IL_SFMT, FileName);
    iSetError(IL_COULD_NOT_OPEN_FILE);
    return IL_FALSE;
  }
}

ILboolean ILAPIENTRY iLoadFuncs2(ILimage* Image, ILenum type) {
  const ILformat *format = iGetFormat(type);
  ILboolean bRet = IL_FALSE;

  if (type == IL_TYPE_UNKNOWN) {
    iSetError(IL_INVALID_ENUM);
  } else if (format) {
    if (format->Load != NULL) {
      // clear old meta data first
      ILmeta *Meta = Image->MetaTags;
      while(Meta) {
        ILmeta *NextMeta = Meta->Next;
        ifree(Meta->Data);
        ifree(Meta->String);
        ifree(Meta);
        Meta = NextMeta;
      }
      Image->MetaTags = NULL;

      bRet = format->Load(Image) && iFixImages(Image, Image);
    }
  } else {
    iSetError(IL_FORMAT_NOT_SUPPORTED);
  }

  if (!bRet && iGetInt(IL_DEFAULT_ON_FAIL)) {
    iDefaultImage(Image);
  }

  return bRet;
}

ILboolean ILAPIENTRY iSaveFuncs2(ILimage* image, ILenum type) {
  ILboolean bRet = IL_FALSE;
  const ILformat *format = iGetFormat(type);

  if (format) {
    return format->Save != NULL && format->Save(image);
  } else {

  // Try registered procedures
  // @todo: must be ported to use Image->io
  /*if (iRegisterSave(FileName))
    return IL_TRUE;*/

    iSetError(IL_FORMAT_NOT_SUPPORTED);
  }

  return bRet;
}

ILboolean ILAPIENTRY iSave(ILimage *Image, ILenum Type, ILconst_string FileName) {
  ILboolean bRet = IL_FALSE;
  const ILformat *format;

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (Image->io.openWrite == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (FileName == NULL || iStrLen(FileName) < 1) {
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }

  if (Type == IL_TYPE_UNKNOWN) {
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }

  format = iGetFormat(Type);
  if (!format || !format->Save) {
    iTrace("**** Unknown format %04x", Type);
    iSetError(IL_FORMAT_NOT_SUPPORTED);
    return IL_FALSE;
  }

  if (iGetInt(IL_FILE_MODE) == IL_FALSE) {
    if (iFileExists(FileName)) {
      iSetError(IL_FILE_ALREADY_EXISTS);
      return IL_FALSE;
    }
  }

  if (SIOopenWR(&Image->io, FileName)) {
    bRet = iSaveFuncs2(Image, Type);
    SIOclose(&Image->io);
  } else {
    iSetError(IL_COULD_NOT_OPEN_FILE);
  }
  return bRet;
}

ILboolean ILAPIENTRY iSaveImage(ILimage *Image, ILconst_string FileName) {
  ILenum Type;
  ILboolean bRet = IL_FALSE;
  const ILformat *format;

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (FileName == NULL || iStrLen(FileName) < 1) {
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }

  if (iSaveRegistered(FileName)) {
    return IL_TRUE;
  }

  if (Image->io.openWrite == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  Type = iTypeFromExt(FileName);

  if (Type == IL_TYPE_UNKNOWN) {
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }

  format = iGetFormat(Type);
  if (!format || !format->Save) {
    iSetError(IL_FORMAT_NOT_SUPPORTED);
    return IL_FALSE;
  }

  if (iGetInt(IL_FILE_MODE) == IL_FALSE) {
    if (iFileExists(FileName)) {
      iSetError(IL_FILE_ALREADY_EXISTS);
      return IL_FALSE;
    }
  }

  if (SIOopenWR(&Image->io, FileName)) {
    bRet = iSaveFuncs2(Image, Type);
    SIOclose(&Image->io);
  } else {
    iSetError(IL_COULD_NOT_OPEN_FILE);
  }
  return bRet;
}

