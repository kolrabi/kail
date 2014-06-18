//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-ILU/src/ilu_states.c
//
// Description: The state machine
//
//-----------------------------------------------------------------------------


#include "ilu_internal.h"
#include "ilu_states.h"


ILconst_string _iluVendor = IL_TEXT("kolrabi");
ILconst_string _iluVersion  = IL_TEXT("kolrabi's another Image Library Utilities (ILU) 1.9.0");// IL_TEXT(__DATE__));

ILconst_string iGetString(ILenum StringName) {
  switch (StringName)   {
    case ILU_VENDOR:
      return _iluVendor;
    //changed 2003-09-04
    case ILU_VERSION_NUM:
      return _iluVersion;
    default:
      iSetError(ILU_INVALID_PARAM);
      break;
  }
  return NULL;
}

void iGetIntegerv(ILenum Mode, ILint *Param) {  
  switch (Mode) {
    case ILU_VERSION_NUM:
      *Param = ILU_VERSION;
      break;

    case ILU_FILTER:
      *Param = iluFilter;
      break;

    default:
      iSetError(ILU_INVALID_ENUM);
  }
  return;
}



ILenum iluFilter = ILU_NEAREST;
ILenum iluPlacement = ILU_CENTER;

void iImageParameter(ILenum PName, ILenum Param) {
  iLockState(); 
  switch (PName)
  {
    case ILU_FILTER:
      switch (Param)
      {
        case ILU_NEAREST:
        case ILU_LINEAR:
        case ILU_BILINEAR:
        case ILU_SCALE_BOX:
        case ILU_SCALE_TRIANGLE:
        case ILU_SCALE_BELL:
        case ILU_SCALE_BSPLINE:
        case ILU_SCALE_LANCZOS3:
        case ILU_SCALE_MITCHELL:
          iluFilter = Param;
          break;
        default:
          iSetError(ILU_INVALID_ENUM);
      }
      break;

    case ILU_PLACEMENT:
      switch (Param)
      {
        case ILU_LOWER_LEFT:
        case ILU_LOWER_RIGHT:
        case ILU_UPPER_LEFT:
        case ILU_UPPER_RIGHT:
        case ILU_CENTER:
          iluPlacement = Param;
          break;
        default:
          iSetError(ILU_INVALID_ENUM);
      }
      break;

    default:
      iSetError(ILU_INVALID_ENUM);
  }
  iUnlockState(); 
}
