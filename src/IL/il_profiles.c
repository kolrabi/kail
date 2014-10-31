//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 01/23/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_profiles.c
//
// Description: Colour profile handler
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_LCMS

#ifdef LCMS_NODIRINCLUDE
  #include <lcms2.h>
#else
  #include <lcms/lcms2.h>
#endif
#endif//IL_NO_LCMS

ILboolean iApplyProfile(ILimage *Image, ILstring InProfile, ILstring OutProfile) {
#ifndef IL_NO_LCMS
  cmsHPROFILE   hInProfile, hOutProfile;
  cmsHTRANSFORM hTransform;
  ILubyte     * Temp;
  ILenum        Format=0;

#ifdef _UNICODE
  char AnsiName[512];
#endif//_UNICODE

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  switch (Image->Type)
  {
    case IL_BYTE:
    case IL_UNSIGNED_BYTE:
      switch (Image->Format)
      {
        case IL_LUMINANCE:
          Format = TYPE_GRAY_8;
          break;
        case IL_RGB:
          Format = TYPE_RGB_8;
          break;
        case IL_BGR:
          Format = TYPE_BGR_8;
          break;
        case IL_RGBA:
          Format = TYPE_RGBA_8;
          break;
        case IL_BGRA:
          Format = TYPE_BGRA_8;
          break;
        default:
          iSetError(IL_INTERNAL_ERROR);
          return IL_FALSE;
      }
      break;

    case IL_SHORT:
    case IL_UNSIGNED_SHORT:
      switch (Image->Format)
      {
        case IL_LUMINANCE:
          Format = TYPE_GRAY_16;
          break;
        case IL_RGB:
          Format = TYPE_RGB_16;
          break;
        case IL_BGR:
          Format = TYPE_BGR_16;
          break;
        case IL_RGBA:
          Format = TYPE_RGBA_16;
          break;
        case IL_BGRA:
          Format = TYPE_BGRA_16;
          break;
        default:
          iSetError(IL_INTERNAL_ERROR);
          return IL_FALSE;
      }
      break;

    // These aren't supported right now.
    case IL_INT:
    case IL_UNSIGNED_INT:
    case IL_FLOAT:
    case IL_DOUBLE:
      iSetError(IL_ILLEGAL_OPERATION);
      return IL_FALSE;
  }


  if (InProfile == NULL) {
    if (!Image->Profile || !Image->ProfileSize) {
      iSetError(IL_INVALID_PARAM);
      return IL_FALSE;
    }
    hInProfile = Image->Profile;
  }
  else {
#ifndef _UNICODE
    hInProfile = cmsOpenProfileFromFile(InProfile, "r");
#else
    wcstombs(AnsiName, InProfile, 512);
    hInProfile = cmsOpenProfileFromFile(AnsiName, "r");
#endif//_UNICODE
  }
#ifndef _UNICODE
  hOutProfile = cmsOpenProfileFromFile(OutProfile, "r");
#else
  wcstombs(AnsiName, OutProfile, 512);
  hOutProfile = cmsOpenProfileFromFile(AnsiName, "r");
#endif//_UNICODE

  hTransform = cmsCreateTransform(hInProfile, Format, hOutProfile, Format, INTENT_PERCEPTUAL, 0);

  Temp = (ILubyte*)ialloc(Image->SizeOfData);
  if (Temp == NULL) {
    return IL_FALSE;
  }

  cmsDoTransform(hTransform, Image->Data, Temp, Image->SizeOfData / 3);

  ifree(Image->Data);
  Image->Data = Temp;

  cmsDeleteTransform(hTransform);
  if (InProfile != NULL)
    cmsCloseProfile(hInProfile);
  cmsCloseProfile(hOutProfile);

#endif//IL_NO_LCMS

  return IL_TRUE;
}
