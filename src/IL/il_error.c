//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 06/02/2007
//
// Filename: src-IL/src/il_error.c
//
// Description: The error functions
//
//-----------------------------------------------------------------------------


#include "il_internal.h"

// Sets the current error
//  If you go past the stack size for this, it cycles the errors, almost like
//  a LRU algo.
ILAPI void ILAPIENTRY iSetErrorReal(ILenum Error) {
  IL_ERROR_STACK *ErrorStack = &iGetTLSData()->CurError;

  ErrorStack->ilErrorPlace++;
  if (ErrorStack->ilErrorPlace >= IL_ERROR_STACK_SIZE) {
    ILuint i;
    iTrace("**** Too many errors, dropping oldest one...");
    for (i = 0; i < IL_ERROR_STACK_SIZE - 2; i++) {
      ErrorStack->ilErrorNum[i] = ErrorStack->ilErrorNum[i+1];
    }
    ErrorStack->ilErrorPlace = IL_ERROR_STACK_SIZE - 1;
  } 
  ErrorStack->ilErrorNum[ErrorStack->ilErrorPlace] = Error;
}


//! Gets the last error on the error stack
ILenum iGetError(void) {
  ILenum ilReturn;
  IL_ERROR_STACK *ErrorStack = &iGetTLSData()->CurError;

  if (ErrorStack->ilErrorPlace >= 0) {
    ilReturn = ErrorStack->ilErrorNum[ErrorStack->ilErrorPlace];
    ErrorStack->ilErrorPlace--;
  } else {
    ilReturn = IL_NO_ERROR;
  }

  return ilReturn;
}
