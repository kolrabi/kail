//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 2014-05-28 by Björn Paetzel
//
// Filename: src/IL/il_string.c
//
// Description: String manipulation stuff
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#include <ctype.h>

//! Glut's portability.txt says to use this...
ILstring ILAPIENTRY iStrDup(ILconst_string Str) {
  ILstring copy;

  copy = (ILstring)ialloc((iStrLen(Str) + 1) * sizeof(ILchar));
  if (copy == NULL)
    return NULL;
  iStrCpy(copy, Str);
  return copy;
}

char * ILAPIENTRY iCharStrDup(const char *Str) {
  char *copy;

  if (Str == NULL) 
    return NULL;

  copy = (char *)ialloc((iCharStrLen(Str) + 1) * sizeof(char));

  if (copy == NULL)
    return NULL;
  
  iCharStrCpy(copy, Str);
  return copy;
}

// Simple function to test if a filename has a given extension, disregarding case
/*ILboolean ILAPIENTRY iCheckExtension(ILconst_string Arg, ILconst_string Ext)
{
  ILboolean PeriodFound = IL_FALSE;
  ILint i, Len;
  ILstring Argu = (ILstring)Arg;

  if (Arg == NULL || Ext == NULL || !iStrLen(Arg) || !iStrLen(Ext))  // if not a good filename/extension, exit early
    return IL_FALSE;

  Len = (ILint)iStrLen(Arg);
  Argu += Len;  // start at the end

  for (i = Len; i >= 0; i--) {
    if (*Argu == '.') {  // try to find a period 
      PeriodFound = IL_TRUE;
      break;
    }
    Argu--;
  }

  if (!PeriodFound)  // if no period, no extension
    return IL_FALSE;

  if (!iStrCmp(Argu+1, Ext))  // extension and ext match?
    return IL_TRUE;

  return IL_FALSE;  // if all else fails, return IL_FALSE
}*/


ILstring ILAPIENTRY iGetExtension(ILconst_string FileName)
{
  ILboolean PeriodFound = IL_FALSE;
  ILstring Ext = (ILstring)FileName;
  ILint i, Len;

  if (FileName == NULL)  // if not a good filename/extension, exit early
    return NULL;

  Len = (ILint)iStrLen(FileName);
  if (Len == 0)
    return NULL;

  Ext += Len;  // start at the end

  for (i = Len; i >= 0; i--) {
    if (*Ext == '.') {  // try to find a period 
      PeriodFound = IL_TRUE;
      break;
    }
    Ext--;
  }

  if (!PeriodFound)  // if no period, no extension
    return NULL;

  return Ext+1;
}
