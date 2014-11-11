//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 11/07/2008
//
// Filename: src-IL/src/il_register.c
//
// Description: Allows the caller to specify user-defined callback functions
//         to open files DevIL does not support, to parse files
//         differently, or anything else a person can think up.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_register.h"
#include <string.h>


// Global variables: Linked lists of registered formats
static iFormatL *LoadProcs = NULL;
static iFormatS *SaveProcs = NULL;

ILboolean iRegisterLoad(ILconst_string Ext, IL_LOADPROC Load) {
  iFormatL *TempNode, *NewNode;

  TempNode = LoadProcs;
  if (TempNode != NULL) {
    while (TempNode->Next != NULL) {
      TempNode = TempNode->Next;
      if (!iStrCmp(TempNode->Ext, Ext)) {  // already registered
        return IL_TRUE;
      }
    }
  }

  NewNode = (iFormatL*)ialloc(sizeof(iFormatL));
  if (NewNode == NULL) {
    return IL_FALSE;
  }

  if (TempNode != NULL) {
    TempNode->Next = NewNode;
  } else {
    LoadProcs = NewNode;
  }

  NewNode->Ext = iStrDup(Ext);
  NewNode->Load = Load;
  NewNode->Next = NULL;

  return IL_TRUE;
}


ILboolean iRegisterSave(ILconst_string Ext, IL_SAVEPROC Save) {
  iFormatS *TempNode, *NewNode;

  TempNode = SaveProcs;
  if (TempNode != NULL) {
    while (TempNode->Next != NULL) {
      TempNode = TempNode->Next;
      if (!iStrCmp(TempNode->Ext, Ext)) {  // already registered
        return IL_TRUE;
      }
    }
  }

  NewNode = (iFormatS*)ialloc(sizeof(iFormatL));
  if (NewNode == NULL) {
    return IL_FALSE;
  }

  if (TempNode != NULL) {
    TempNode->Next = NewNode;
  } else {
    SaveProcs = NewNode;
  }

  NewNode->Ext = iStrDup(Ext);
  NewNode->Save = Save;
  NewNode->Next = NULL;

  return IL_TRUE;
}

ILboolean iRemoveLoad(ILconst_string Ext) {
  iFormatL *TempNode = LoadProcs, *PrevNode = NULL;

  while (TempNode != NULL) {
    if (!iStrCmp(Ext, TempNode->Ext)) {
      if (PrevNode == NULL) {  // first node in the list
        LoadProcs = TempNode->Next;
        ifree(TempNode->Ext);
        ifree(TempNode);
      }
      else {
        PrevNode->Next = TempNode->Next;
        ifree(TempNode->Ext);
        ifree(TempNode);
      }

      return IL_TRUE;
    }

    PrevNode = TempNode;
    TempNode = TempNode->Next;
  }

  return IL_FALSE;
}

ILboolean iRemoveSave(ILconst_string Ext) {
  iFormatS *TempNode = SaveProcs, *PrevNode = NULL;

  while (TempNode != NULL) {
    if (!iStrCmp(Ext, TempNode->Ext)) {
      if (PrevNode == NULL) {  // first node in the list
        SaveProcs = TempNode->Next;
        ifree(TempNode->Ext);
        ifree(TempNode);
      }
      else {
        PrevNode->Next = TempNode->Next;
        ifree(TempNode->Ext);
        ifree(TempNode);
      }

      return IL_TRUE;
    }

    PrevNode = TempNode;
    TempNode = TempNode->Next;
  }

  return IL_FALSE;
}


// Automatically removes all registered formats.
void ilRemoveRegistered()
{
  iFormatL *TempNodeL = LoadProcs;
  iFormatS *TempNodeS = SaveProcs;

  while (LoadProcs != NULL) {
    TempNodeL = LoadProcs->Next;
    ifree(LoadProcs->Ext);
    ifree(LoadProcs);
    LoadProcs = TempNodeL;
  }

  while (SaveProcs != NULL) {
    TempNodeS = SaveProcs->Next;
    ifree(SaveProcs->Ext);
    ifree(SaveProcs);
    SaveProcs = TempNodeS;
  }

  return;
}


ILboolean iLoadRegistered(ILconst_string FileName)
{
  iFormatL  *TempNode = LoadProcs;
  ILstring  Ext = iGetExtension(FileName);
  ILenum    Error;

  if (!Ext)
    return IL_FALSE;

  while (TempNode != NULL) {
    if (!iStrCmp(Ext, TempNode->Ext)) {
      Error = TempNode->Load(FileName);
      if (Error == IL_NO_ERROR) { 
        return IL_TRUE;
      }
      else {
        iSetError(Error);
        return IL_FALSE;
      }
    }
    TempNode = TempNode->Next;
  }

  return IL_FALSE;
}


ILboolean iSaveRegistered(ILconst_string FileName)
{
  iFormatS  *TempNode = SaveProcs;
  ILstring  Ext = iGetExtension(FileName);
  ILenum    Error;

  if (!Ext)
    return IL_FALSE;

  while (TempNode != NULL) {
    if (!iStrCmp(Ext, TempNode->Ext)) {
      Error = TempNode->Save(FileName);
      if (Error == IL_NO_ERROR) {
        return IL_TRUE;
      }
      else {
        iSetError(Error);
        return IL_FALSE;
      }
    }
    TempNode = TempNode->Next;
  }

  return IL_FALSE;
}


//
// "Reporting" functions
//

void iRegisterOrigin(ILimage *Image, ILenum Origin) {
  switch (Origin)
  {
    case IL_ORIGIN_LOWER_LEFT:
    case IL_ORIGIN_UPPER_LEFT:
      Image->Origin = Origin;
      break;
    default:
      iSetError(IL_INVALID_ENUM);
  }
}

void iRegisterFormat(ILimage *Image, ILenum Format) {
  switch (Format) {
    case IL_COLOUR_INDEX:
    case IL_RGB:
    case IL_RGBA:
    case IL_BGR:
    case IL_BGRA:
    case IL_LUMINANCE:
    case IL_LUMINANCE_ALPHA:
      Image->Format = Format;
      break;
    default:
      iSetError(IL_INVALID_ENUM);
  }
}

ILboolean iRegisterNumFaces(ILimage *Image, ILuint Num) {
  ILimage *Next, *Prev;

  if (Image->Faces)
    iCloseImage(Image->Faces);

  Image->Faces = NULL;
  if (Num == 0)  // Just gets rid of all the mipmaps.
    return IL_TRUE;

  Image->Faces = iNewImage(1, 1, 1, 1, 1);
  if (Image->Faces == NULL)
    return IL_FALSE;

  Next = Image->Faces;
  Num--;

  while (Num) {
    Next->Faces = iNewImage(1, 1, 1, 1, 1);
    if (Next->Faces == NULL) {
      // Clean up before we error out.
      Prev = Image->Faces;
      while (Prev) {
        Next = Prev->Faces;
        iCloseImage(Prev);
        Prev = Next;
      }
      return IL_FALSE;
    }
    Next = Next->Faces;
    Num--;
  }

  return IL_TRUE;
}

ILboolean iRegisterMipNum(ILimage *Image, ILuint Num) {
  ILimage *Next, *Prev;

  if (Image->Mipmaps)
    iCloseImage(Image->Mipmaps);

  Image->Mipmaps = NULL;
  if (Num == 0)  // Just gets rid of all the mipmaps.
    return IL_TRUE;

  Image->Mipmaps = iNewImage(1, 1, 1, 1, 1);
  if (Image->Mipmaps == NULL)
    return IL_FALSE;
  Next = Image->Mipmaps;
  Num--;

  while (Num) {
    Next->Mipmaps = iNewImage(1, 1, 1, 1, 1);
    if (Next->Mipmaps == NULL) {
      // Clean up before we error out.
      Prev = Image->Mipmaps;
      while (Prev) {
        Next = Prev->Mipmaps;
        iCloseImage(Prev);
        Prev = Next;
      }
      return IL_FALSE;
    }
    Next = Next->Mipmaps;
    Num--;
  }

  return IL_TRUE;
}

ILboolean iRegisterNumImages(ILimage *Image, ILuint Num) {
  ILimage *Next, *Prev;

  if (Image->Next)
    iCloseImage(Image->Next);

  Image->Next = NULL;
  if (Num == 0)  // Just gets rid of all the "next" images.
    return IL_TRUE;

  Image->Next = iNewImage(1, 1, 1, 1, 1);
  if (Image->Next == NULL)
    return IL_FALSE;
  Next = Image->Next;
  Num--;

  while (Num) {
    Next->Next = iNewImage(1, 1, 1, 1, 1);
    if (Next->Next == NULL) {
      // Clean up before we error out.
      Prev = Image->Next;
      while (Prev) {
        Next = Prev->Next;
        iCloseImage(Prev);
        Prev = Next;
      }
      return IL_FALSE;
    }
    Next = Next->Next;
    Num--;
  }

  return IL_TRUE;
}

void iRegisterType(ILimage *Image, ILenum Type) {
  switch (Type)   {
    case IL_BYTE:
    case IL_UNSIGNED_BYTE:
    case IL_SHORT:
    case IL_UNSIGNED_SHORT:
    case IL_INT:
    case IL_UNSIGNED_INT:
    case IL_FLOAT:
    case IL_DOUBLE:
      Image->Type = Type;
      break;
    default:
      iSetError(IL_INVALID_ENUM);
  }

  return;
}

void iRegisterPal(ILimage *Image, void *Pal, ILuint Size, ILenum Type) {
  if (!Image->Pal.Palette || !Image->Pal.PalSize || Image->Pal.PalType != IL_PAL_NONE) {
    ifree(Image->Pal.Palette);
  }

  Image->Pal.PalSize = Size;
  Image->Pal.PalType = Type;
  Image->Pal.Palette = (ILubyte*)ialloc(Size);
  if (Image->Pal.Palette == NULL)
    return;

  if (Pal != NULL) {
    memcpy(Image->Pal.Palette, Pal, Size);
  }
  else {
    iSetError(IL_INVALID_PARAM);
  }
}

ILboolean iSetDuration(ILimage *Image, ILuint Duration) {
  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  Image->Duration = Duration;
  return IL_TRUE;
}
