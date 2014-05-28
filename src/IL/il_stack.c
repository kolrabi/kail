//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 12/17/2008
//
// Filename: src-IL/src/il_stack.c
//
// Description: The main image stack
//
//-----------------------------------------------------------------------------

// Credit goes to John Villar (johnny@reliaschev.com) for making the suggestion
//  of not letting the user use ILimage structs but instead binding images
//  like OpenGL.

#include "il_internal.h"
#include "il_stack.h"

static void iSetImage0();

// Global variables for il_stack.c
ILuint    StackSize = 0;
ILuint    LastUsed = 0;
ILuint    CurName = 0;
ILimage   **ImageStack = NULL;
iFree   *FreeNames = NULL;
ILboolean OnExit = IL_FALSE;
ILboolean ParentImage = IL_TRUE;
ILimage *iCurImage = NULL;

//! Creates Num images and puts their index in Images - similar to glGenTextures().
void ILAPIENTRY ilGenImages(ILsizei Num, ILuint *Images)
{
  ILsizei Index = 0;
  iFree *TempFree = FreeNames;

  if (Num < 1 || Images == NULL) {
    ilSetError(IL_INVALID_VALUE);
    return;
  }

  // No images have been generated yet, so create the image stack.
  if (ImageStack == NULL)
    if (!iEnlargeStack())
      return;

  do {
    if (FreeNames != NULL) {  // If any have been deleted, then reuse their image names.
      TempFree = (iFree*)FreeNames->Next;
      Images[Index] = FreeNames->Name;
      ImageStack[FreeNames->Name] = ilNewImage(1, 1, 1, 1, 1);
      ifree(FreeNames);
      FreeNames = TempFree;
    } else {
      if (LastUsed >= StackSize)
        if (!iEnlargeStack())
          return;
      Images[Index] = LastUsed;
      // Must be all 1's instead of 0's, because some functions would divide by 0.
      ImageStack[LastUsed] = ilNewImage(1, 1, 1, 1, 1);
      LastUsed++;
    }
  } while (++Index < Num);
}

ILuint ILAPIENTRY ilGenImage()
{
    ILuint i;
    ilGenImages(1,&i);
    return i;
}

ILimage * ILAPIENTRY iGetImage(ILuint Image)
{
  if (ImageStack == NULL || StackSize == 0) {
    return NULL;
  }

  // If the user requests a high image name.
  if (Image >= StackSize) {
    return NULL;
  }

  return ImageStack[Image];
}

//! Makes Image the current active image - similar to glBindTexture().
void ILAPIENTRY ilBindImage(ILuint Image)
{
  if (ImageStack == NULL || StackSize == 0) {
    if (!iEnlargeStack()) {
      return;
    }
  }

  // If the user requests a high image name.
  while (Image >= StackSize) {
    if (!iEnlargeStack()) {
      return;
    }
  }

  if (ImageStack[Image] == NULL) {
    ImageStack[Image] = ilNewImage(1, 1, 1, 1, 1);
    if (Image >= LastUsed) // >= ?
      LastUsed = Image + 1;
  }

  iCurImage = ImageStack[Image];
  CurName = Image;

  ParentImage = IL_TRUE;

  return;
}


//! Deletes Num images from the image stack - similar to glDeleteTextures().
void ILAPIENTRY ilDeleteImages(ILsizei Num, const ILuint *Images)
{
  iFree *Temp = FreeNames;
  ILuint  Index = 0;

  if (Num < 1) {
    //ilSetError(IL_INVALID_VALUE);
    return;
  }
  if (StackSize == 0)
    return;

  do {
    if (Images[Index] > 0 && Images[Index] < LastUsed) {  // <= ?
      /*if (FreeNames != NULL) {  // Terribly inefficient
        Temp = FreeNames;
        do {
          if (Temp->Name == Images[Index]) {
            continue;  // Sufficient?
          }
        } while ((Temp = Temp->Next));
      }*/

      // Already has been deleted or was never used.
      if (ImageStack[Images[Index]] == NULL)
        continue;

      // Find out if current image - if so, set to default image zero.
      if (Images[Index] == CurName || Images[Index] == 0) {
        iCurImage = ImageStack[0];
        CurName = 0;
      }
      
      // Should *NOT* be NULL here!
      ilCloseImage(ImageStack[Images[Index]]);
      ImageStack[Images[Index]] = NULL;

      // Add to head of list - works for empty and non-empty lists
      Temp = (iFree*)ialloc(sizeof(iFree));
      if (!Temp) {
        return;
      }
      Temp->Name = Images[Index];
      Temp->Next = FreeNames;
      FreeNames = Temp;
    }
    /*else {  // Shouldn't set an error...just continue onward.
      ilSetError(IL_ILLEGAL_OPERATION);
    }*/
  } while (++Index < (ILuint)Num);
}


void ILAPIENTRY ilDeleteImage(const ILuint Num) {
    ilDeleteImages(1,&Num);
}

//! Checks if Image is a valid ilGenImages-generated image (like glIsTexture()).
ILboolean ILAPIENTRY ilIsImage(ILuint Image)
{
  //iFree *Temp = FreeNames;

  if (ImageStack == NULL)
    return IL_FALSE;
  if (Image >= LastUsed || Image == 0)
    return IL_FALSE;

  /*do {
    if (Temp->Name == Image)
      return IL_FALSE;
  } while ((Temp = Temp->Next));*/

  if (ImageStack[Image] == NULL)  // Easier check.
    return IL_FALSE;

  return IL_TRUE;
}


//! Closes Image and frees all memory associated with it.
ILAPI void ILAPIENTRY ilCloseImage(ILimage *Image)
{
  if (Image == NULL)
    return;

  if (Image->Data != NULL) {
    ifree(Image->Data);
    Image->Data = NULL;
  }

  if (Image->Pal.Palette != NULL && Image->Pal.PalSize > 0 && Image->Pal.PalType != IL_PAL_NONE) {
    ifree(Image->Pal.Palette);
    Image->Pal.Palette = NULL;
  }

  if (Image->Next != NULL) {
    ilCloseImage(Image->Next);
    Image->Next = NULL;
  }

  if (Image->Faces != NULL) {
    ilCloseImage(Image->Faces);
    Image->Mipmaps = NULL;
  }

  if (Image->Mipmaps != NULL) {
    ilCloseImage(Image->Mipmaps);
    Image->Mipmaps = NULL;
  }

  if (Image->Layers != NULL) {
    ilCloseImage(Image->Layers);
    Image->Layers = NULL;
  }

  if (Image->AnimList != NULL && Image->AnimSize != 0) {
    ifree(Image->AnimList);
    Image->AnimList = NULL;
  }

  if (Image->Profile != NULL && Image->ProfileSize != 0) {
    ifree(Image->Profile);
    Image->Profile = NULL;
    Image->ProfileSize = 0;
  }

  if (Image->DxtcData != NULL && Image->DxtcFormat != IL_DXT_NO_COMP) {
    ifree(Image->DxtcData);
    Image->DxtcData = NULL;
    Image->DxtcFormat = IL_DXT_NO_COMP;
    Image->DxtcSize = 0;
  }

  ifree(Image);
  Image = NULL;

  return;
}


ILAPI ILboolean ILAPIENTRY ilIsValidPal(ILpal *Palette)
{
  if (Palette == NULL)
    return IL_FALSE;
  if (Palette->PalSize == 0 || Palette->Palette == NULL)
    return IL_FALSE;
  switch (Palette->PalType)
  {
    case IL_PAL_RGB24:
    case IL_PAL_RGB32:
    case IL_PAL_RGBA32:
    case IL_PAL_BGR24:
    case IL_PAL_BGR32:
    case IL_PAL_BGRA32:
      return IL_TRUE;
  }
  return IL_FALSE;
}


//! Closes Palette and frees all memory associated with it.
ILAPI void ILAPIENTRY ilClosePal(ILpal *Palette)
{
  if (Palette == NULL)
    return;
  if (!ilIsValidPal(Palette))
    return;
  ifree(Palette->Palette);
  ifree(Palette);
  return;
}


ILimage *iGetBaseImage()
{
  return ImageStack[ilGetCurName()];
}


//! Sets the current mipmap level
ILboolean ILAPIENTRY ilActiveMipmap(ILuint Number)
{
  if (iCurImage == NULL) {
    ilSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (Number == 0) {
    return IL_TRUE;
  }

   ILimage * iTempImage = iCurImage;
  iCurImage->Mipmaps->io = iCurImage->io;
  iCurImage = iCurImage->Mipmaps;
  if (iCurImage == NULL) {
    iCurImage = iTempImage;
    ilSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  ILuint Current;
  for (Current = 1; Current < Number; Current++) {
    iCurImage->Mipmaps->io = iCurImage->io;
    iCurImage = iCurImage->Mipmaps;
    if (iCurImage == NULL) {
      ilSetError(IL_ILLEGAL_OPERATION);
      iCurImage = iTempImage;
      return IL_FALSE;
    }
  }

  ParentImage = IL_FALSE;

  return IL_TRUE;
}

ILimage *ILAPIENTRY iGetMipmap(ILimage *Image, ILuint Number)
{
  if (Image == NULL) {
    return NULL;
  }

  if (Number == 0) {
    return Image;
  }

  ILimage * iTempImage = Image;
  Image = Image->Mipmaps;
  if (Image == NULL) {
    return NULL;
  }

  ILuint Current;
  for (Current = 1; Current < Number; Current++) {
    Image = Image->Mipmaps;
    if (Image == NULL) {
      return NULL;
    }
  }

  Image->io = iTempImage->io;
  return Image;
}


//! Used for setting the current image if it is an animation.
ILboolean ILAPIENTRY ilActiveImage(ILuint Number)
{
  ILuint Current;
  ILimage *iTempImage;
    
  if (iCurImage == NULL) {
    ilSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (Number == 0) {
    return IL_TRUE;
  }

    iTempImage = iCurImage;
  iCurImage = iCurImage->Next;
  if (iCurImage == NULL) {
    iCurImage = iTempImage;
    ilSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  Number--;  // Skip 0 (parent image)
  for (Current = 0; Current < Number; Current++) {
    iCurImage = iCurImage->Next;
    if (iCurImage == NULL) {
      ilSetError(IL_ILLEGAL_OPERATION);
      iCurImage = iTempImage;
      return IL_FALSE;
    }
  }

  ParentImage = IL_FALSE;

  return IL_TRUE;
}

//! Used for setting the current image if it is an animation.
ILimage * ILAPIENTRY iGetSubImage(ILimage *Image, ILuint Number)
{
  if (Image == NULL) {
    return NULL;
  }

  if (Number == 0) {
    return Image;
  }

  if (Image->Next == NULL) {
    return NULL;
  }

  ILuint Current;
  for (Current = 0; Current < Number; Current++) {
    if (Image->Next == NULL) {
      return NULL;
    }
    Image = Image->Next;
  }

  return Image;
}


//! Used for setting the current face if it is a cubemap.
ILboolean ILAPIENTRY ilActiveFace(ILuint Number)
{
  ILuint Current;
    ILimage *iTempImage;

  if (iCurImage == NULL) {
    ilSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (Number == 0) {
    return IL_TRUE;
  }

    iTempImage = iCurImage;
  iCurImage = iCurImage->Faces;
  if (iCurImage == NULL) {
    iCurImage = iTempImage;
    ilSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  //Number--;  // Skip 0 (parent image)
  for (Current = 1; Current < Number; Current++) {
    iCurImage = iCurImage->Faces;
    if (iCurImage == NULL) {
      ilSetError(IL_ILLEGAL_OPERATION);
      iCurImage = iTempImage;
      return IL_FALSE;
    }
  }

  ParentImage = IL_FALSE;

  return IL_TRUE;
}



//! Used for setting the current layer if layers exist.
ILboolean ILAPIENTRY ilActiveLayer(ILuint Number)
{
  ILuint Current;
    ILimage *iTempImage;

  if (iCurImage == NULL) {
    ilSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (Number == 0) {
    return IL_TRUE;
  }

    iTempImage = iCurImage;
  iCurImage = iCurImage->Layers;
  if (iCurImage == NULL) {
    iCurImage = iTempImage;
    ilSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  //Number--;  // Skip 0 (parent image)
  for (Current = 1; Current < Number; Current++) {
    iCurImage = iCurImage->Layers;
    if (iCurImage == NULL) {
      ilSetError(IL_ILLEGAL_OPERATION);
      iCurImage = iTempImage;
      return IL_FALSE;
    }
  }

  ParentImage = IL_FALSE;

  return IL_TRUE;
}


ILuint ILAPIENTRY ilCreateSubImage(ILenum Type, ILuint Num)
{
  ILimage *SubImage;
  ILuint  Count ;  // Create one before we go in the loop.

  if (iCurImage == NULL) {
    ilSetError(IL_ILLEGAL_OPERATION);
    return 0;
  }
  if (Num == 0)  {
    return 0;
  }

  switch (Type)
  {
    case IL_SUB_NEXT:
      if (iCurImage->Next)
        ilCloseImage(iCurImage->Next);
      iCurImage->Next = ilNewImage(1, 1, 1, 1, 1);
      SubImage = iCurImage->Next;
      break;

    case IL_SUB_MIPMAP:
      if (iCurImage->Mipmaps)
        ilCloseImage(iCurImage->Mipmaps);
      iCurImage->Mipmaps = ilNewImage(1, 1, 1, 1, 1);
      SubImage = iCurImage->Mipmaps;
      break;

    case IL_SUB_LAYER:
      if (iCurImage->Layers)
        ilCloseImage(iCurImage->Layers);
      iCurImage->Layers = ilNewImage(1, 1, 1, 1, 1);
      SubImage = iCurImage->Layers;
      break;

    default:
      ilSetError(IL_INVALID_ENUM);
      return IL_FALSE;
  }

  if (SubImage == NULL) {
    return 0;
  }

  for (Count = 1; Count < Num; Count++) {
    SubImage->Next = ilNewImage(1, 1, 1, 1, 1);
    SubImage = SubImage->Next;
    if (SubImage == NULL)
      return Count;
  }

  return Count;
}


// Returns the current index.
ILAPI ILuint ILAPIENTRY ilGetCurName()
{
  if (iCurImage == NULL || ImageStack == NULL || StackSize == 0)
    return 0;
  return CurName;
}


// Returns the current image.
ILAPI ILimage* ILAPIENTRY iGetCurImage()
{
  return iCurImage;
}


// To be only used when the original image is going to be set back almost immediately.
ILAPI void ILAPIENTRY ilSetCurImage(ILimage *Image)
{
  iCurImage = Image;
  return;
}

// Completely replaces the current image and the version in the image stack.
ILAPI void ILAPIENTRY ilReplaceCurImage(ILimage *Image)
{
  if (iCurImage) {
    ilActiveImage(0);
    ilCloseImage(iCurImage);
  }
  ImageStack[ilGetCurName()] = Image;
  iCurImage = Image;
  ParentImage = IL_TRUE;
  return;
}


// Like realloc but sets new memory to 0.
void* ILAPIENTRY ilRecalloc(void *Ptr, ILuint OldSize, ILuint NewSize)
{
  void *Temp = ialloc(NewSize);
  ILuint CopySize = (OldSize < NewSize) ? OldSize : NewSize;

  if (Temp != NULL) {
    if (Ptr != NULL) {
      memcpy(Temp, Ptr, CopySize);
      ifree(Ptr);
    }

    Ptr = Temp;

    if (OldSize < NewSize)
      imemclear((ILubyte*)Temp + OldSize, NewSize - OldSize);
  }

  return Temp;
}


// Internal function to enlarge the image stack by I_STACK_INCREMENT members.
ILboolean iEnlargeStack()
{
  if (!(ImageStack = (ILimage**)ilRecalloc(ImageStack, StackSize * sizeof(ILimage*), (StackSize + I_STACK_INCREMENT) * sizeof(ILimage*)))) {
    return IL_FALSE;
  }
  StackSize += I_STACK_INCREMENT;
  return IL_TRUE;
}

// Global variables
static ILboolean IsInit = IL_FALSE;

// ONLY call at startup.
void ILAPIENTRY ilInit()
{
  // if it is already initialized skip initialization
  if (IsInit == IL_TRUE ) 
    return;

  const char *IL_TRACE_ENV      = getenv("IL_TRACE");
  const char *IL_TRACE_FILE_ENV = getenv("IL_TRACE_FILE");

  if (IL_TRACE_FILE_ENV)          iTraceOut = fopen(IL_TRACE_FILE_ENV, "w+b");
  if (IL_TRACE_ENV && !iTraceOut) iTraceOut = stderr;
  
  //ilSetMemory(NULL, NULL);  Now useless 3/4/2006 (due to modification in il_alloc.c)
  ilSetError(IL_NO_ERROR);
  ilDefaultStates();  // Set states to their defaults.
  iSetImage0();  // Beware!  Clears all existing textures!

  iBindImageTemp();  // Go ahead and create the temporary image.

  iInitFormats();

  IsInit = IL_TRUE;
  return;
}


// Frees any extra memory in the stack.
//  - Called on exit
void ILAPIENTRY ilShutDown()
{
  // if it is not initialized do not shutdown
  iFree* TempFree = (iFree*)FreeNames;
  ILuint i;
  
  if (!IsInit)
    return;

  if (!IsInit) {  // Prevent from being called when not initialized.
    ilSetError(IL_ILLEGAL_OPERATION);
    return;
  }

  while (TempFree != NULL) {
    FreeNames = (iFree*)TempFree->Next;
    ifree(TempFree);
    TempFree = FreeNames;
  }

  //for (i = 0; i < LastUsed; i++) {
  for (i = 0; i < StackSize; i++) {
    if (ImageStack[i] != NULL)
      ilCloseImage(ImageStack[i]);
  }

  if (ImageStack)
    ifree(ImageStack);
  ImageStack = NULL;
  LastUsed = 0;
  StackSize = 0;

  iDeinitFormats();

  if (iTraceOut && iTraceOut != stderr) fclose(iTraceOut);

  IsInit = IL_FALSE;
  return;
}


// Initializes the image stack's first entry (default image) -- ONLY CALL ONCE!
static void iSetImage0() {
  if (ImageStack == NULL)
    if (!iEnlargeStack())
      return;

  LastUsed = 1;
  CurName = 0;
  ParentImage = IL_TRUE;
  if (!ImageStack[0])
    ImageStack[0] = ilNewImage(1, 1, 1, 1, 1);
  iCurImage = ImageStack[0];
  ilDefaultImage();
}


ILAPI void ILAPIENTRY iBindImageTemp()
{
  if (ImageStack == NULL || StackSize <= 1)
    if (!iEnlargeStack())
      return;

  ILimage *lastImage = iCurImage;

  if (LastUsed < 2)
    LastUsed = 2;
  CurName = 1;
  ParentImage = IL_TRUE;
  if (!ImageStack[1])
    ImageStack[1] = ilNewImage(1, 1, 1, 1, 1);
  iCurImage = ImageStack[1];
  iCurImage->io = lastImage->io;
}
