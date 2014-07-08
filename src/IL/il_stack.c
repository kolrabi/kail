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

#if IL_THREAD_SAFE_PTHREAD
#include <pthread.h>

static pthread_key_t iTlsKey;
pthread_mutex_t iStateMutex;

#elif IL_THREAD_SAFE_WIN32

static DWORD         iTlsKey = ~0;
HANDLE iStateMutex = NULL;

#endif

static void iSetImage0();
ILboolean iDefaultImage(ILimage *Image);

// Global variables for il_stack.c shared among threads
ILuint      StackSize     = 0;
ILuint      LastUsed      = 0;
ILimage **  ImageStack    = NULL;
iFree   *   FreeNames     = NULL;

static void iInitTlsData(IL_TLS_DATA *Data) {

  imemclear(Data, sizeof(*Data)); 
  Data->CurError.ilErrorPlace = -1;
}

#if IL_THREAD_SAFE_PTHREAD
// only called when thread safety is enabled and a thread stops
static void iFreeTLSData(void *ptr) {
  IL_TLS_DATA *Data = (IL_TLS_DATA*)ptr;
  // TODO: clear set strings from state
  ifree(Data);
}
#endif

IL_TLS_DATA * iGetTLSData() {
#if IL_THREAD_SAFE_PTHREAD
  IL_TLS_DATA *iDataPtr = (IL_TLS_DATA*)pthread_getspecific(iTlsKey);
  if (iDataPtr == NULL) {
    iDataPtr = ioalloc(IL_TLS_DATA);
    pthread_setspecific(iTlsKey, iDataPtr);
    iInitTlsData(iDataPtr);
  }

  return iDataPtr;

#elif IL_THREAD_SAFE_WIN32

  IL_TLS_DATA *iDataPtr = (IL_TLS_DATA*)TlsGetValue(iTlsKey);
  if (iDataPtr == NULL) {
    iDataPtr = ioalloc(IL_TLS_DATA);
    TlsSetValue(iTlsKey, iDataPtr);
    iInitTlsData(iDataPtr);
  }

  return iDataPtr;

#else
  static IL_TLS_DATA iData;
  static IL_TLS_DATA *iDataPtr = NULL;

  if (iDataPtr == NULL) {
    iDataPtr = &iData;
    iInitTlsData(iDataPtr);
  }
  return iDataPtr;
#endif
}

static IL_IMAGE_SELECTION *iGetSelection() {
  IL_TLS_DATA *iData = iGetTLSData();
  return &iData->CurSel;
}

static void iSetSelection(ILuint Image, ILuint Frame, ILuint Face, ILuint Layer, ILuint Mipmap) {
  iGetSelection()->CurName    = Image;
  iGetSelection()->CurFrame   = Frame;
  iGetSelection()->CurFace    = Face;
  iGetSelection()->CurLayer   = Layer;
  iGetSelection()->CurMipmap  = Mipmap;
}

ILimage * iGetImage(ILuint Image) {
  if (ImageStack == NULL || StackSize == 0) {
    return NULL;
  }

  // If the user requests a high image name.
  if (Image >= StackSize) {
    return NULL;
  }

  return ImageStack[Image];
}

ILimage * iGetSelectedImage(const IL_IMAGE_SELECTION *CurSel) {
  ILimage *Image;
  ILuint i;

  if (CurSel == NULL) CurSel = iGetSelection();

  Image = iGetImage(CurSel->CurName);

  for (i=0; i<CurSel->CurFrame; i++) {
    if (Image) Image = Image->Next;
  }
  for (i=0; i<CurSel->CurFace; i++) {
    if (Image) Image = Image->Faces;
  }
  for (i=0; i<CurSel->CurLayer; i++) {
    if (Image) Image = Image->Layers;
  }
  for (i=0; i<CurSel->CurMipmap; i++) {
    if (Image) Image = Image->Mipmaps;
  }
  return Image;
}

void iGenImages(ILsizei Num, ILuint *Images) {
  ILsizei Index = 0;
  iFree *TempFree = FreeNames;

  if (Num < 1 || Images == NULL) {
    iSetError(IL_INVALID_VALUE);
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
      ImageStack[FreeNames->Name] = iNewImage(1, 1, 1, 1, 1);
      ifree(FreeNames);
      FreeNames = TempFree;
    } else {
      if (LastUsed >= StackSize)
        if (!iEnlargeStack())
          return;
      Images[Index] = LastUsed;
      // Must be all 1's instead of 0's, because some functions would divide by 0.
      ImageStack[LastUsed] = iNewImage(1, 1, 1, 1, 1);
      LastUsed++;
    }
  } while (++Index < Num);
}

void iBindImage(ILuint Image) {
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
    ImageStack[Image] = iNewImage(1, 1, 1, 1, 1);
    if (Image >= LastUsed) // >= ?
      LastUsed = Image + 1;
  }

  iSetSelection(Image, 0, 0, 0, 0);
}


void iDeleteImages(ILsizei Num, const ILuint *Images) {
  iFree *Temp = FreeNames;
  ILuint  Index = 0;

  if (Num < 1) {
    //iSetError(IL_INVALID_VALUE);
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
      if (Images[Index] == iGetCurName() || Images[Index] == 0) {
        iSetSelection(0, 0, 0, 0, 0);
      }
      
      // Should *NOT* be NULL here!
      iCloseImage(ImageStack[Images[Index]]);
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
      iSetError(IL_ILLEGAL_OPERATION);
    }*/
  } while (++Index < (ILuint)Num);
}


//! Checks if Image is a valid ilGenImages-generated image (like glIsTexture()).
ILboolean iIsImage(ILuint Image)
{
  if (ImageStack == NULL)
    return IL_FALSE;

  if (Image >= LastUsed || Image == 0)
    return IL_FALSE;

  if (ImageStack[Image] == NULL)  // Easier check.
    return IL_FALSE;

  return IL_TRUE;
}


//! Closes Image and frees all memory associated with it.
ILAPI void ILAPIENTRY iCloseImageReal(ILimage *Image)
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
    iCloseImage(Image->Next);
    Image->Next = NULL;
  }

  if (Image->Faces != NULL) {
    iCloseImage(Image->Faces);
    Image->Mipmaps = NULL;
  }

  if (Image->Mipmaps != NULL) {
    iCloseImage(Image->Mipmaps);
    Image->Mipmaps = NULL;
  }

  if (Image->Layers != NULL) {
    iCloseImage(Image->Layers);
    Image->Layers = NULL;
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

  // clear old exif data first
  while(Image->MetaTags) {
    ILmeta *NextExif = Image->MetaTags->Next;
    ifree(Image->MetaTags->Data);
    ifree(Image->MetaTags->String);
    ifree(Image->MetaTags);
    Image->MetaTags = NextExif;
  }
  Image->MetaTags = NULL;

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
ILAPI void ILAPIENTRY iClosePalReal(ILpal *Palette)
{
  if (Palette == NULL)
    return;
  if (!ilIsValidPal(Palette))
    return;
  ifree(Palette->Palette);
  ifree(Palette);
  return;
}

//! Sets the current mipmap level
ILboolean iActiveMipmap(ILuint Number) {
  IL_IMAGE_SELECTION CurSel = *iGetSelection();
  ILimage *Image;

  if (iGetInt(IL_IMAGE_SELECTION_MODE) == IL_ABSOLUTE)
    CurSel.CurMipmap = Number;
  else
    CurSel.CurMipmap += Number; // this is for compatibility
  
  Image = iGetSelectedImage(&CurSel);
  if (!Image) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  *iGetSelection() = CurSel;
  return IL_TRUE;
}

ILimage *ILAPIENTRY iGetMipmap(ILimage *Image, ILuint Number)
{
  ILimage * iTempImage = Image;
  ILuint Current;

  if (Image == NULL) {
    return NULL;
  }

  if (Number == 0) {
    return Image;
  }

  Image = Image->Mipmaps;
  if (Image == NULL) {
    return NULL;
  }

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
ILboolean iActiveImage(ILuint Number)
{
  IL_IMAGE_SELECTION CurSel = *iGetSelection();
  ILimage *Image;

  if (iGetInt(IL_IMAGE_SELECTION_MODE) == IL_ABSOLUTE)
    CurSel.CurFrame = Number;
  else    
    CurSel.CurFrame += Number; // this is for compatibility
 

  Image = iGetSelectedImage(&CurSel);
  if (!Image) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  *iGetSelection() = CurSel;
  return IL_TRUE;
}

//! Used for setting the current image if it is an animation.
ILimage * ILAPIENTRY iGetSubImage(ILimage *Image, ILuint Number)
{
  ILuint Current;

  if (Image == NULL) {
    return NULL;
  }

  if (Number == 0) {
    return Image;
  }

  if (Image->Next == NULL) {
    return NULL;
  }

  for (Current = 0; Current < Number; Current++) {
    if (Image->Next == NULL) {
      return NULL;
    }
    Image = Image->Next;
  }

  return Image;
}

ILboolean iActiveFace(ILuint Number) {
  IL_IMAGE_SELECTION CurSel = *iGetSelection();
  ILimage *Image;

  if (iGetInt(IL_IMAGE_SELECTION_MODE) == IL_ABSOLUTE)
    CurSel.CurFace = Number;
  else    
    CurSel.CurFace += Number; // this is for compatibility
  
  Image = iGetSelectedImage(&CurSel);
  if (!Image) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  *iGetSelection() = CurSel;
  return IL_TRUE;
}

//! Used for setting the current layer if layers exist.
ILboolean iActiveLayer(ILuint Number)
{
  IL_IMAGE_SELECTION CurSel = *iGetSelection();
  ILimage *Image;

  if (iGetInt(IL_IMAGE_SELECTION_MODE) == IL_ABSOLUTE)
    CurSel.CurLayer = Number;
  else    
    CurSel.CurLayer += Number; // this is for compatibility
  
  Image = iGetSelectedImage(&CurSel);
  if (!Image) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  *iGetSelection() = CurSel;
  return IL_TRUE;  
}

ILuint iCreateSubImage(ILimage *Image, ILenum Type, ILuint Num) {
  ILimage *SubImage;
  ILuint  Count ;  // Create one before we go in the loop.

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return 0;
  }

  if (Num == 0)  {
    return 0;
  }

  switch (Type)
  {
    case IL_SUB_NEXT:
      if (Image->Next)
        iCloseImage(Image->Next);
      Image->Next = iNewImage(1, 1, 1, 1, 1);
      SubImage = Image->Next;
      break;

    case IL_SUB_MIPMAP:
      if (Image->Mipmaps)
        iCloseImage(Image->Mipmaps);
      Image->Mipmaps = iNewImage(1, 1, 1, 1, 1);
      SubImage = Image->Mipmaps;
      break;

    case IL_SUB_LAYER:
      if (Image->Layers)
        iCloseImage(Image->Layers);
      Image->Layers = iNewImage(1, 1, 1, 1, 1);
      SubImage = Image->Layers;
      break;

    default:
      iSetError(IL_INVALID_ENUM);
      return IL_FALSE;
  }

  if (SubImage == NULL) {
    return 0;
  }

  for (Count = 1; Count < Num; Count++) {
    ILimage *Next = iNewImage(1, 1, 1, 1, 1);
    if (!Next) break;
    switch (Type) {
      case IL_SUB_NEXT:     SubImage->Next    = Next; break;
      case IL_SUB_MIPMAP:   SubImage->Mipmaps = Next; break;
      case IL_SUB_LAYER:    SubImage->Layers  = Next; break;
    }
    SubImage = Next;
  }

  return Count;
}

// Returns the current index.
ILAPI ILuint ILAPIENTRY iGetCurName() {
  return iGetSelection()->CurName;
}

// Returns the current image.
static ILimage* iGetCurImage() {
  return iGetSelectedImage(iGetSelection());
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
void iInitIL()
{
  const char *IL_TRACE_ENV      = getenv("IL_TRACE");
  const char *IL_TRACE_FILE_ENV = getenv("IL_TRACE_FILE");

  // if it is already initialized skip initialization
  if (IsInit == IL_TRUE ) 
    return;

  if (IL_TRACE_FILE_ENV)          iTraceOut = fopen(IL_TRACE_FILE_ENV, "w+b");
  if (IL_TRACE_ENV && !iTraceOut) iTraceOut = stderr;

  #if IL_THREAD_SAFE_PTHREAD
  pthread_mutex_init(&iStateMutex, NULL);
  pthread_key_create(&iTlsKey, &iFreeTLSData);
  #elif IL_THREAD_SAFE_WIN32
  iStateMutex = CreateMutex(NULL, FALSE, NULL);
  iTlsKey = TlsAlloc();
  #endif
  
  iSetError(IL_NO_ERROR);
  ilDefaultStates();      // Set states to their defaults.
  iSetImage0();           // Beware!  Clears all existing textures!
  // iBindImageTemp();       // Go ahead and create the temporary image.
  iInitFormats();

  IsInit = IL_TRUE;
  return;
}


// Frees any extra memory in the stack.
//  - Called on exit
void iShutDownIL()
{
  // if it is not initialized do not shutdown
  iFree* TempFree = (iFree*)FreeNames;
  ILuint i;
  
  if (!IsInit)
    return;

  while (TempFree != NULL) {
    FreeNames = (iFree*)TempFree->Next;
    ifree(TempFree);
    TempFree = FreeNames;
  }

  for (i = 0; i < StackSize; i++) {
    if (ImageStack[i] != NULL)
      iCloseImage(ImageStack[i]);
  }

  if (ImageStack)
    ifree(ImageStack);

  ImageStack = NULL;
  LastUsed = 0;
  StackSize = 0;

  iDeinitFormats();

  if (iTraceOut && iTraceOut != stderr) fclose(iTraceOut);

  #if IL_THREAD_SAFE_WIN32
  CloseHandle(iStateMutex);
  #endif

  IsInit = IL_FALSE;
  return;
}


// Initializes the image stack's first entry (default image) -- ONLY CALL ONCE!
static void iSetImage0() {
  if (ImageStack == NULL)
    if (!iEnlargeStack())
      return;

  LastUsed = 1;
  iSetSelection(0, 0,0,0,0);
  if (!ImageStack[0])
    ImageStack[0] = iNewImage(1, 1, 1, 1, 1);
  iDefaultImage(ImageStack[0]);
}

ILAPI void ILAPIENTRY iLockState() {
#if IL_THREAD_SAFE_PTHREAD
  pthread_mutex_lock(&iStateMutex);
#elif IL_THREAD_SAFE_WIN32
  WaitForSingleObject(iStateMutex, INFINITE);
#endif
}

ILAPI void ILAPIENTRY iUnlockState() {
#if IL_THREAD_SAFE_PTHREAD
  pthread_mutex_unlock(&iStateMutex);
#elif IL_THREAD_SAFE_WIN32
  ReleaseMutex(iStateMutex);
#endif
} 

ILAPI ILimage * ILAPIENTRY iLockCurImage() {
  ILimage *Image = iGetCurImage();
  if (!Image || !Image->BaseImage) {
    return NULL;
  }

#if IL_THREAD_SAFE_PTHREAD
  pthread_mutex_lock(&Image->BaseImage->Mutex);
#elif IL_THREAD_SAFE_WIN32
  WaitForSingleObject(Image->BaseImage->Mutex, INFINITE);
#endif
  return Image;
}

ILAPI ILimage * ILAPIENTRY iLockImage(ILuint Name) {
  ILimage *Image = iGetImage(Name);
  if (!Image) {
    return NULL;
  }

#if IL_THREAD_SAFE_PTHREAD
  pthread_mutex_lock(&Image->BaseImage->Mutex);
#elif IL_THREAD_SAFE_WIN32
  WaitForSingleObject(Image->BaseImage->Mutex, INFINITE);
#endif

  return Image;
}

ILAPI void ILAPIENTRY iUnlockImage(ILimage *Image) {
  if (!Image || !Image->BaseImage) return;

#if IL_THREAD_SAFE_PTHREAD
  pthread_mutex_unlock(&Image->BaseImage->Mutex);
#elif IL_THREAD_SAFE_WIN32
  ReleaseMutex(Image->BaseImage->Mutex);
#endif
}

