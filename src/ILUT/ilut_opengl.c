//-----------------------------------------------------------------------------
//
// ImageLib Utility Toolkit Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2002 <--Y2K Compliant! =]
//
// Filename: src-ILUT/src/ilut_opengl.c
//
// Description: OpenGL functions for images
//
//-----------------------------------------------------------------------------


/**
 * @addtogroup ILUT Image Library Utility Toolkit
 * @{
 * @defgroup ilut_gl OpenGL Functionality
 * Contains all functions to convert/copy image data from the IL to OpenGL
 * textures and back.
 * @{
 */

#include "ilut_opengl.h"

#ifdef ILUT_USE_OPENGL

#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER
  #pragma comment(lib, "opengl32.lib")
  #pragma comment(lib, "Glu32.lib")
#endif

#ifdef linux
  // fix for glXGetProcAddressARB
  #define GLX_GLXEXT_PROTOTYPES
  #include <GL/glx.h>
#endif

//used for automatic texture target detection
#define ILGL_TEXTURE_CUBE_MAP       0x8513
// #define ILGL_TEXTURE_BINDING_CUBE_MAP   0x8514
#define ILGL_TEXTURE_CUBE_MAP_POSITIVE_X  0x8515
#define ILGL_TEXTURE_CUBE_MAP_NEGATIVE_X  0x8516
#define ILGL_TEXTURE_CUBE_MAP_POSITIVE_Y  0x8517
#define ILGL_TEXTURE_CUBE_MAP_NEGATIVE_Y  0x8518
#define ILGL_TEXTURE_CUBE_MAP_POSITIVE_Z  0x8519
#define ILGL_TEXTURE_CUBE_MAP_NEGATIVE_Z  0x851A
#define ILGL_CLAMP_TO_EDGE          0x812F
#define ILGL_TEXTURE_WRAP_R         0x8072

// Not defined in OpenGL 1.1 headers.
#define ILGL_TEXTURE_DEPTH          0x8071
#define ILGL_TEXTURE_3D           0x806F
#define ILGL_MAX_3D_TEXTURE_SIZE      0x8073

typedef struct {
  ILenum    DXTCFormat;
  ILuint    MaxTexW, MaxTexH, MaxTexD;
  ILenum    Filter;
  ILuint    Flags;
} ILUT_TEXTURE_SETTINGS_GL;

static ILenum    ilutGLFormat(ILenum, ILubyte);
static ILimage*  MakeGLCompliant2D(ILimage *Src, ILUT_TEXTURE_SETTINGS_GL *Settings);
static ILimage*  MakeGLCompliant3D(ILimage *Src, ILUT_TEXTURE_SETTINGS_GL *Settings);
static ILboolean IsExtensionSupported(const char *extension);

static ILboolean HasCubemapHardware = IL_FALSE;
static ILboolean HasNonPowerOfTwoHardware = IL_FALSE;
#if defined(_WIN32) || defined(_WIN64) || defined(linux) || defined(__APPLE__)
  static ILGLTEXIMAGE3DARBPROC     ilGLTexImage3D = NULL;
  static ILGLTEXSUBIMAGE3DARBPROC    ilGLTexSubImage3D = NULL;
  static ILGLCOMPRESSEDTEXIMAGE2DARBPROC ilGLCompressed2D = NULL;
  static ILGLCOMPRESSEDTEXIMAGE3DARBPROC ilGLCompressed3D = NULL;
#endif

static ILboolean ilutGLSetTex2D_(ILimage *ilutCurImage, GLuint TexID);

static void GetSettings(ILUT_TEXTURE_SETTINGS_GL *settings) {
  settings->Flags = ilutGetCurrentFlags();
  settings->DXTCFormat = (ILenum)ilutGetInteger(ILUT_S3TC_FORMAT);
  settings->MaxTexW = (ILuint)ilutGetInteger(ILUT_MAXTEX_WIDTH);
  settings->MaxTexH = (ILuint)ilutGetInteger(ILUT_MAXTEX_HEIGHT);
  settings->MaxTexD = (ILuint)ilutGetInteger(ILUT_MAXTEX_DEPTH);
  settings->Filter = (ILenum)iluGetInteger(ILU_FILTER);
}

static ILboolean iInitGLExtensions()
{
  #if (defined (_WIN32) || defined(_WIN64))

    if (IsExtensionSupported("GL_ARB_texture_compression") &&
        IsExtensionSupported("GL_EXT_texture_compression_s3tc")) {
      ilGLCompressed2D = (ILGLCOMPRESSEDTEXIMAGE2DARBPROC)wglGetProcAddress("glCompressedTexImage2DARB");
    }
    if (IsExtensionSupported("GL_EXT_texture3D")) {
      ilGLTexImage3D = (ILGLTEXIMAGE3DARBPROC)wglGetProcAddress("glTexImage3D");
      ilGLTexSubImage3D = (ILGLTEXSUBIMAGE3DARBPROC)wglGetProcAddress("glTexSubImage3D");
    }
    if (IsExtensionSupported("GL_ARB_texture_compression") &&
        IsExtensionSupported("GL_EXT_texture_compression_s3tc") &&
        IsExtensionSupported("GL_EXT_texture3D")) {
      ilGLCompressed3D = (ILGLCOMPRESSEDTEXIMAGE3DARBPROC)wglGetProcAddress("glCompressedTexImage3DARB");
    }
    return IL_TRUE;

  #elif defined(__APPLE__)

    // Mac OS X headers are OpenGL 1.4 compliant already.
    ilGLCompressed2D  = glCompressedTexImage2DARB;
    ilGLTexImage3D    = glTexImage3D;
    ilGLTexSubImage3D = glTexSubImage3D;
    ilGLCompressed3D  = glCompressedTexImage3DARB;

  #elif defined(HAVE_GL_GLX_H)

    if (IsExtensionSupported("GL_ARB_texture_compression") &&
        IsExtensionSupported("GL_EXT_texture_compression_s3tc")) {
        ilGLCompressed2D = (ILGLCOMPRESSEDTEXIMAGE2DARBPROC)
          glXGetProcAddressARB((const GLubyte*)"glCompressedTexImage2DARB");
    }
    if (IsExtensionSupported("GL_EXT_texture3D")) {
      ilGLTexImage3D = (ILGLTEXIMAGE3DARBPROC)glXGetProcAddressARB((const GLubyte*)"glTexImage3D");
      ilGLTexSubImage3D = (ILGLTEXSUBIMAGE3DARBPROC)glXGetProcAddress((const GLubyte*)"glTexSubImage3D");
    }
    if (IsExtensionSupported("GL_ARB_texture_compression") &&
        IsExtensionSupported("GL_EXT_texture_compression_s3tc") &&
        IsExtensionSupported("GL_EXT_texture3D")) {
      ilGLCompressed3D = (ILGLCOMPRESSEDTEXIMAGE3DARBPROC)glXGetProcAddressARB((const GLubyte*)"glCompressedTexImage3DARB");
    }
    return IL_TRUE;
  #else
    return IL_FALSE;  // @TODO: Find any other systems that we could be on.
  #endif
}

// Absolutely *have* to call this if planning on using the image library with OpenGL.
//  Call this after OpenGL has initialized.
ILboolean ilutGLInit()
{
  ILint MaxTexW, MaxTexH, MaxTexD = 1;

  iLockState(); // lock to prevent running more than once at the same time

  // Should we really be setting all this ourselves?  Seems too much like a glu(t) approach...
  glEnable(GL_TEXTURE_2D);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);

  if (!iInitGLExtensions()) {
    iUnlockState();
    return IL_FALSE;
  }

  // Use PROXY_TEXTURE_2D/3D with glTexImage2D/3D() to test more accurately...
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&MaxTexW);
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&MaxTexH);
  if (ilGLTexImage3D != NULL)
    glGetIntegerv(ILGL_MAX_3D_TEXTURE_SIZE, (GLint*)&MaxTexD);
  if (MaxTexW <= 0 || MaxTexH <= 0 || MaxTexD <= 0) {
    MaxTexW = MaxTexH = 256;  // Trying this because of the VooDoo series of cards.
    MaxTexD = 1;
  }

  ilutSetInteger(ILUT_MAXTEX_WIDTH, MaxTexW);
  ilutSetInteger(ILUT_MAXTEX_HEIGHT, MaxTexH);
  ilutSetInteger(ILUT_MAXTEX_DEPTH, MaxTexD);

  if (IsExtensionSupported("GL_ARB_texture_cube_map"))
    HasCubemapHardware = IL_TRUE;
  if (IsExtensionSupported("GL_ARB_texture_non_power_of_two"))
    HasNonPowerOfTwoHardware = IL_TRUE;
  
  iUnlockState();
  return IL_TRUE;
}


// @TODO:  Check what dimensions an image has and use the appropriate IL_IMAGE_XD #define!

static GLuint ilutGLBindTexImage_(ILimage *Image, ILUT_TEXTURE_SETTINGS_GL *Settings) {
  GLuint  TexID = 0, Target = GL_TEXTURE_2D;

  if (Image == NULL) {
    iSetError(ILUT_ILLEGAL_OPERATION);
    return 0;
  }

  if (Settings->Flags & ILUT_STATE_FLAG_AUTODETECT_TARGET) {
    if (HasCubemapHardware && Image->CubeFlags != 0)
      Target = ILGL_TEXTURE_CUBE_MAP;
    // TODO: GL_TEXTURE_3D
  }

  glGenTextures(1, &TexID);
  glBindTexture(Target, TexID);

  if (Target == GL_TEXTURE_2D) {
    glTexParameteri(Target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(Target, GL_TEXTURE_WRAP_T, GL_REPEAT);
  }
  else if (Target == ILGL_TEXTURE_CUBE_MAP) {
    glTexParameteri(Target, GL_TEXTURE_WRAP_S, ILGL_CLAMP_TO_EDGE);
    glTexParameteri(Target, GL_TEXTURE_WRAP_T, ILGL_CLAMP_TO_EDGE);
    glTexParameteri(Target, ILGL_TEXTURE_WRAP_R, ILGL_CLAMP_TO_EDGE);
  }
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexParameteri(Target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(Target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_SWAP_BYTES, IL_FALSE);

  if (!ilutGLTexImage(0)) {
    glDeleteTextures(1, &TexID);
    iUnlockImage(Image);
    return 0;
  }

  iUnlockImage(Image);
  return TexID;
}


GLuint ILAPIENTRY ilutGLBindTexImage() {
  ILUT_TEXTURE_SETTINGS_GL Settings;
  ILimage *Image;
  GLuint Result;

  iLockState(); 
  Image = iLockCurImage(); 
  GetSettings(&Settings);
  iUnlockState(); 

  Result = ilutGLBindTexImage_(Image, &Settings);
  iUnlockImage(Image);
  return Result;
}


static ILuint GLGetDXTCNum(ILenum DXTCFormat) {
  switch (DXTCFormat)
  {
    // Constants from glext.h.
    case IL_DXT1: DXTCFormat = 0x83F1; break;
    case IL_DXT3: DXTCFormat = 0x83F2; break;
    case IL_DXT5: DXTCFormat = 0x83F3; break;
  }

  return DXTCFormat;
}


// We assume *all* states have been set by the user, including 2D texturing!
static ILboolean ILAPIENTRY ilutGLTexImage_(GLint Level, GLuint Target, ILimage *Image, ILUT_TEXTURE_SETTINGS_GL *Settings)
{
  ILimage *ImageCopy;

  if (Image == NULL) {
    iSetError(ILUT_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if ((Settings->Flags & ILUT_STATE_FLAG_USE_S3TC) && ilGLCompressed2D != NULL) {
    if (Image->DxtcData != NULL && Image->DxtcSize != 0) {
      ILenum DXTCFormat = GLGetDXTCNum(Image->DxtcFormat);
      ilGLCompressed2D(Target, Level, DXTCFormat, (GLsizei)Image->Width,
        (GLsizei)Image->Height, 0, (GLsizei)Image->DxtcSize, Image->DxtcData);
      return IL_TRUE;
    }

    if ((Settings->Flags & ILUT_STATE_FLAG_GEN_S3TC)) {
      ILuint  Size = iGetDXTCData(Image, NULL, 0, Settings->DXTCFormat);
      if (Size != 0) {
          ILubyte *Buffer = (ILubyte*)ialloc(Size);
        if (Buffer == NULL) {         
          return IL_FALSE;
        }

        Size = iGetDXTCData(Image, Buffer, Size, Settings->DXTCFormat);
        if (Size == 0) {
          ifree(Buffer);
          return IL_FALSE;
        }

        Settings->DXTCFormat = GLGetDXTCNum(Settings->DXTCFormat);
        ilGLCompressed2D(Target, Level, Settings->DXTCFormat, (GLsizei)Image->Width,
          (GLsizei)Image->Height, 0, (GLsizei)Size, Buffer);
        ifree(Buffer);
        return IL_TRUE;
      }
    }
  }

  ImageCopy = MakeGLCompliant2D(Image, Settings);
  if (ImageCopy == NULL)
    return IL_FALSE;

  glTexImage2D(
      Target, 
      Level, 
      (GLint)ilutGLFormat(ImageCopy->Format, ImageCopy->Bpp),
      (GLsizei)ImageCopy->Width,
      (GLsizei)ImageCopy->Height,
      0,
      ImageCopy->Format,
      ImageCopy->Type,
      ImageCopy->Data); 

  if (Image != ImageCopy)
    iCloseImage(ImageCopy);

  return IL_TRUE;
}

static GLuint iToGLCube(ILuint cube)
{
  switch (cube) {
    case IL_CUBEMAP_POSITIVEX:
      return ILGL_TEXTURE_CUBE_MAP_POSITIVE_X;
    case IL_CUBEMAP_POSITIVEY:
      return ILGL_TEXTURE_CUBE_MAP_POSITIVE_Y;
    case IL_CUBEMAP_POSITIVEZ:
      return ILGL_TEXTURE_CUBE_MAP_POSITIVE_Z;
    case IL_CUBEMAP_NEGATIVEX:
      return ILGL_TEXTURE_CUBE_MAP_NEGATIVE_X;
    case IL_CUBEMAP_NEGATIVEY:
      return ILGL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
    case IL_CUBEMAP_NEGATIVEZ:
      return ILGL_TEXTURE_CUBE_MAP_NEGATIVE_Z;

    default:
      return ILGL_TEXTURE_CUBE_MAP_POSITIVE_X; //???
  }
}

ILboolean ILAPIENTRY ilutGLTexImage(GLint Level)
{
  ILimage *ilutCurImage;
  ILimage *Temp;
  ILUT_TEXTURE_SETTINGS_GL Settings;

  iLockState(); 
  ilutCurImage = iLockCurImage(); 
  GetSettings(&Settings);
  iUnlockState(); 

  if (!(Settings.Flags & ILUT_STATE_FLAG_AUTODETECT_TARGET)) {
    ILboolean Result = ilutGLTexImage_(Level, GL_TEXTURE_2D, ilutCurImage, &Settings);
    iUnlockImage(ilutCurImage);
    return Result;
  } else {
    // Autodetect texture target

    // Cubemap
    if (ilutCurImage->CubeFlags != 0 && HasCubemapHardware) { //bind to cubemap
      Temp = ilutCurImage;
      while (Temp != NULL && Temp->CubeFlags != 0) {
        ilutGLTexImage_(Level, iToGLCube(Temp->CubeFlags), Temp, &Settings);
        Temp = Temp->Faces;
      }
      iUnlockImage(ilutCurImage);
      return IL_TRUE; //@TODO: check for errors??
    }
    else { // 2D texture
      ILboolean Result = ilutGLTexImage_(Level, GL_TEXTURE_2D, ilutCurImage, &Settings);
      iUnlockImage(ilutCurImage);
      return Result;
    }
  }
}

GLuint ILAPIENTRY ilutGLBindMipmaps()
{
  GLuint  TexID = 0;

//  glPushAttrib(GL_ALL_ATTRIB_BITS);

  glGenTextures(1, &TexID);
  glBindTexture(GL_TEXTURE_2D, TexID);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

  if (!ilutGLBuildMipmaps()) {
    glDeleteTextures(1, &TexID);
    return 0;
  }

//  glPopAttrib();

  return TexID;
}


ILboolean ILAPIENTRY ilutGLBuildMipmaps()
{
  ILimage *ilutCurImage;
  ILimage *Image;
  ILUT_TEXTURE_SETTINGS_GL Settings;

  iLockState(); 
  ilutCurImage = iLockCurImage(); 
  GetSettings(&Settings);
  iUnlockState(); 

  if (ilutCurImage == NULL) {
    iSetError(ILUT_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  Image = MakeGLCompliant2D(ilutCurImage, &Settings);
  iUnlockImage(ilutCurImage);

  if (Image == NULL) {
   iUnlockImage(ilutCurImage);
   return IL_FALSE;
  }

  gluBuild2DMipmaps(GL_TEXTURE_2D, (GLint)ilutGLFormat(Image->Format, Image->Bpp), (GLint)Image->Width,
            (GLint)Image->Height, Image->Format, Image->Type, Image->Data);

  if (Image != ilutCurImage)
    iCloseImage(Image);
 
  iUnlockImage(ilutCurImage); 
  return IL_TRUE;
}


ILboolean ILAPIENTRY ilutGLSubTex(GLuint TexID, ILint XOff, ILint YOff)
{
  return ilutGLSubTex2D(TexID, XOff, YOff);
}


ILboolean ILAPIENTRY ilutGLSubTex2D(GLuint TexID, ILint XOff, ILint YOff)
{
  ILimage *ilutCurImage;
  ILimage *Image;
  ILint Width, Height;
  ILUT_TEXTURE_SETTINGS_GL Settings;

  iLockState(); 
  ilutCurImage = iLockCurImage(); 
  GetSettings(&Settings);
  iUnlockState(); 

  if (ilutCurImage == NULL) {
    iSetError(ILUT_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  Image = MakeGLCompliant2D(ilutCurImage, &Settings);

  if (Image == NULL) {
    iUnlockImage(ilutCurImage);
    return IL_FALSE;
  }

  glBindTexture(GL_TEXTURE_2D, TexID);

  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  (GLint*)&Width);
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, (GLint*)&Height);

  if ((ILint)Image->Width + XOff > Width || (ILint)Image->Height + YOff > Height) {
    iSetError(ILUT_BAD_DIMENSIONS);
    iUnlockImage(ilutCurImage);
    return IL_FALSE;
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_SWAP_BYTES, IL_FALSE);
  glTexSubImage2D(GL_TEXTURE_2D, 0, XOff, YOff, (GLsizei)Image->Width, (GLsizei)Image->Height, Image->Format,
      Image->Type, Image->Data);

  if (Image != ilutCurImage)
    iCloseImage(Image);

  iUnlockImage(ilutCurImage);
  return IL_TRUE;
}


ILboolean ILAPIENTRY ilutGLSubTex3D(GLuint TexID, ILint XOff, ILint YOff, ILint ZOff)
{
  ILimage *ilutCurImage; 
  ILimage *Image;
  ILint Width, Height, Depth;
  ILUT_TEXTURE_SETTINGS_GL Settings;

  if (ilGLTexSubImage3D == NULL) {
    iSetError(ILUT_ILLEGAL_OPERATION);  // Set a different error?
    return IL_FALSE;
  }

  iLockState(); 
  ilutCurImage = iLockCurImage(); 
  GetSettings(&Settings);
  iUnlockState(); 

  if (ilutCurImage == NULL) {
    iSetError(ILUT_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  Image = MakeGLCompliant3D(ilutCurImage, &Settings);
  if (Image == NULL) {
    iUnlockImage(ilutCurImage);
    return IL_FALSE;
  }

  glBindTexture(ILGL_TEXTURE_3D, TexID);

  glGetTexLevelParameteriv(ILGL_TEXTURE_3D, 0, GL_TEXTURE_WIDTH,  (GLint*)&Width);
  glGetTexLevelParameteriv(ILGL_TEXTURE_3D, 0, GL_TEXTURE_HEIGHT, (GLint*)&Height);
  glGetTexLevelParameteriv(ILGL_TEXTURE_3D, 0, ILGL_TEXTURE_DEPTH, (GLint*)&Depth);

  if ((GLsizei)Image->Width + XOff > Width || (GLsizei)Image->Height + YOff > Height
    || (GLsizei)Image->Depth + ZOff > Depth) {
    iSetError(ILUT_BAD_DIMENSIONS);
    iUnlockImage(ilutCurImage);
    return IL_FALSE;
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_SWAP_BYTES, IL_FALSE);
  ilGLTexSubImage3D(ILGL_TEXTURE_3D, 0, XOff, YOff, ZOff, (GLsizei)Image->Width, (GLsizei)Image->Height, (GLsizei)Image->Depth, 
      Image->Format, Image->Type, Image->Data);

  if (Image != ilutCurImage)
    iCloseImage(Image);

  iUnlockImage(ilutCurImage);
  return IL_TRUE;
}


static ILimage* MakeGLCompliant2D(ILimage *Src, ILUT_TEXTURE_SETTINGS_GL* Settings) {
  ILimage   *Dest = Src, *Temp;
  ILboolean Created = IL_FALSE;
  ILubyte   *Flipped;
  ILboolean   need_resize = IL_FALSE;

  if (Src->Pal.Palette != NULL && Src->Pal.PalSize != 0 && Src->Pal.PalType != IL_PAL_NONE) {
    //ilSetCurImage(Src);
    Dest = iConvertImage(Src, iGetPalBaseType(Src->Pal.PalType), IL_UNSIGNED_BYTE);
    //Dest = iConvertImage(IL_BGR);
    //ilSetCurImage(ilutCurImage);
    if (Dest == NULL)
      return NULL;

    Created = IL_TRUE;

    // Change here!


    // Set Dest's palette stuff here
    Dest->Pal.PalType = IL_PAL_NONE;
  }

  if (HasNonPowerOfTwoHardware == IL_FALSE && 
      (Src->Width  != iNextPower2(Src->Width)  ||
       Src->Height != iNextPower2(Src->Height)  )) {
        need_resize = IL_TRUE;
      }

  if (Src->Width > Settings->MaxTexW || Src->Height > Settings->MaxTexH)
    need_resize = IL_TRUE;

  if (need_resize == IL_TRUE) {
    ILuint DestW = IL_MIN( Settings->MaxTexW, HasNonPowerOfTwoHardware ? Dest->Width  : iNextPower2(Dest->Width)  );
    ILuint DestH = IL_MIN( Settings->MaxTexH, HasNonPowerOfTwoHardware ? Dest->Height : iNextPower2(Dest->Height) );

    if (!Created) {
      Dest = iCloneImage(Src);
      if (Dest == NULL) {
        return NULL;
      }
      // Created = IL_TRUE;
    }

    if (Src->Format == IL_COLOUR_INDEX) {
      Temp = iluScale_(Dest, DestW, DestH, 1, ILU_NEAREST);
    } else {
      Temp = iluScale_(Dest, DestW, DestH, 1, ILU_BILINEAR);
    }
    iCloseImage(Dest);

    if (!Temp) {
      return NULL;
    }
    Dest = Temp;
  }

  if (Dest->Origin != IL_ORIGIN_LOWER_LEFT) {
    Flipped = iGetFlipped(Dest);
    ifree(Dest->Data);
    Dest->Data = Flipped;
    Dest->Origin = IL_ORIGIN_LOWER_LEFT;
  }

  return Dest;
}


ILimage* MakeGLCompliant3D(ILimage *Src, ILUT_TEXTURE_SETTINGS_GL *Settings)
{
  ILimage   *Dest = Src, *Temp;
  ILboolean Created = IL_FALSE;
  ILubyte   *Flipped;
  ILboolean   need_resize = IL_FALSE;

  if (Src->Pal.Palette != NULL && Src->Pal.PalSize != 0 && Src->Pal.PalType != IL_PAL_NONE) {
    Dest = iConvertImage(Src, iGetPalBaseType(Src->Pal.PalType), IL_UNSIGNED_BYTE);

    if (Dest == NULL)
      return NULL;

    Created = IL_TRUE;

    // Change here!
    
    // Set Dest's palette stuff here
    Dest->Pal.PalType = IL_PAL_NONE;
  }

  if (HasNonPowerOfTwoHardware == IL_FALSE && 
      (Src->Width  != iNextPower2(Src->Width)  ||
       Src->Height != iNextPower2(Src->Height) ||
       Src->Depth  != iNextPower2(Src->Depth) )) {
        need_resize = IL_TRUE;
      }

  if (Src->Width > Settings->MaxTexW || Src->Height > Settings->MaxTexH || Src->Depth > Settings->MaxTexD)
    need_resize = IL_TRUE;

  if (need_resize == IL_TRUE) {
    ILuint DestW = IL_MIN( Settings->MaxTexW, HasNonPowerOfTwoHardware ? Dest->Width  : iNextPower2(Dest->Width)  );
    ILuint DestH = IL_MIN( Settings->MaxTexH, HasNonPowerOfTwoHardware ? Dest->Height : iNextPower2(Dest->Height) );

    if (!Created) {
      Dest = iCloneImage(Src);
      if (Dest == NULL) {
        return NULL;
      }
      // Created = IL_TRUE;
    }

    if (Src->Format == IL_COLOUR_INDEX) {
      Temp = iluScale_(Dest, DestW, DestH, 1, ILU_NEAREST);
    } else {
      Temp = iluScale_(Dest, DestW, DestH, 1, ILU_BILINEAR);
    }

    iCloseImage(Dest);
    
    if (!Temp) {
      return NULL;
    }
    Dest = Temp;
  }

  if (Dest->Origin != IL_ORIGIN_LOWER_LEFT) {
    Flipped = iGetFlipped(Dest);
    ifree(Dest->Data);
    Dest->Data = Flipped;
    Dest->Origin = IL_ORIGIN_LOWER_LEFT;
  }

  return Dest;
}


//! Just a convenience function.
#ifndef _WIN32_WCE
GLuint ILAPIENTRY ilutGLLoadImage(ILstring FileName)
{
  GLuint  TexId;
  ILimage *ilutCurImage;
  ILimage* Temp;
  ILUT_TEXTURE_SETTINGS_GL Settings;

  iLockState();
  ilutCurImage = iLockCurImage();
  Temp = iNewImage(1,1,1,1,1);
  Temp->io = ilutCurImage->io;
  iUnlockImage(ilutCurImage);
  GetSettings(&Settings);
  iUnlockState();

  if (!iLoad(Temp, IL_TYPE_UNKNOWN, FileName)) {
    iCloseImage(Temp);
    return 0;
  }

  TexId = ilutGLBindTexImage_(Temp, &Settings);
  iCloseImage(Temp);
  return TexId;
}
#endif//_WIN32_WCE


#ifndef _WIN32_WCE
ILboolean ILAPIENTRY ilutGLSaveImage(ILstring FileName, GLuint TexID) {
  ILboolean Saved;
  ILimage *ilutCurImage;
  ILimage* Temp;
  ILUT_TEXTURE_SETTINGS_GL Settings;
  
  iLockState();
  ilutCurImage = iLockCurImage();
  Temp = iNewImage(1,1,1,1,1);
  Temp->io = ilutCurImage->io;
  iUnlockImage(ilutCurImage);
  GetSettings(&Settings);
  iUnlockState();

  if (!ilutGLSetTex2D_(Temp, TexID)) {
    return IL_FALSE;
  }

  Saved = iSave(Temp, IL_TYPE_UNKNOWN, FileName);

  return Saved;
}
#endif//_WIN32_WCE


//! Takes a screenshot of the current OpenGL window.
static ILboolean ilutGLScreen_(ILimage *ilutCurImage) {
  ILuint  ViewPort[4];

  if (ilutCurImage == NULL) {
    iSetError(ILUT_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  glGetIntegerv(GL_VIEWPORT, (GLint*)ViewPort);

  if (!iTexImage(ilutCurImage, ViewPort[2], ViewPort[3], 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL)) {
    iUnlockImage(ilutCurImage);
    return IL_FALSE;  // Error already set.
  }
  ilutCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, (GLsizei)ViewPort[2], (GLsizei)ViewPort[3], GL_RGB, GL_UNSIGNED_BYTE, ilutCurImage->Data);

  iUnlockImage(ilutCurImage);
  return IL_TRUE;
}

ILboolean ILAPIENTRY ilutGLScreen() {
  ILboolean Result;
  ILimage *ilutCurImage;

  iLockState();
  ilutCurImage = iLockCurImage();
  iUnlockState();

  Result = ilutGLScreen_(ilutCurImage);
  iUnlockImage(ilutCurImage);
  return Result;
}

#ifndef _WIN32_WCE
ILboolean ILAPIENTRY ilutGLScreenie() {
  FILE    *File = NULL;
  char    Buff[255];
  ILuint    i;
  ILboolean ReturnVal = IL_TRUE;
  ILimage *Temp;

  // Could go above 128 easily...
  for (i = 0; i < 128; i++) {
    snprintf(Buff, sizeof(Buff), "screen%04u.tga", i);
    File = fopen(Buff, "rb");

    if (!File)
      break;

    fclose(File);
  }

  if (File != NULL) {
    iSetError(ILUT_COULD_NOT_OPEN_FILE);
    return IL_FALSE;
  }

  Temp = iNewImage(1,1,1, 1,1);

  if (!ilutGLScreen_(Temp)) {
    ReturnVal = IL_FALSE;
  }

  if (ReturnVal) {
#ifdef _UNICODE
    ILstring FileName = iWideFromMultiByte(Buff);
    iSave(Temp, IL_TGA, FileName);
    ifree(FileName);
#else
    iSave(Temp, IL_TGA, Buff);
#endif
  }

  iCloseImage(Temp);
  return ReturnVal;
}
#endif//_WIN32_WCE


ILboolean ilutGLSetTex2D_(ILimage *ilutCurImage, GLuint TexID) {
  ILubyte *Data;
  ILuint Width, Height;

  glBindTexture(GL_TEXTURE_2D, TexID);

  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  (GLint*)&Width);
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, (GLint*)&Height);

  Data = (ILubyte*)ialloc(Width * Height * 4);
  if (Data == NULL) {
    return IL_FALSE;
  }

  glGetTexImage(GL_TEXTURE_2D, 0, IL_BGRA, GL_UNSIGNED_BYTE, Data);

  if (!iTexImage(ilutCurImage, Width, Height, 1, 4, IL_BGRA, IL_UNSIGNED_BYTE, Data)) {
    ifree(Data);
    return IL_FALSE;
  }

  ilutCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

  ifree(Data);
  return IL_TRUE;
}


ILboolean ILAPIENTRY ilutGLSetTex3D(GLuint TexID)
{
  ILimage *ilutCurImage;
  ILubyte *Data;
  ILuint Width, Height, Depth;

  if (ilGLTexImage3D == NULL) {
    iSetError(ILUT_ILLEGAL_OPERATION);  // Set a different error?
    return IL_FALSE;
  }

  glBindTexture(ILGL_TEXTURE_3D, TexID);

  glGetTexLevelParameteriv(ILGL_TEXTURE_3D, 0, GL_TEXTURE_WIDTH,  (GLint*)&Width);
  glGetTexLevelParameteriv(ILGL_TEXTURE_3D, 0, GL_TEXTURE_HEIGHT, (GLint*)&Height);
  glGetTexLevelParameteriv(ILGL_TEXTURE_3D, 0, ILGL_TEXTURE_DEPTH, (GLint*)&Depth);

  Data = (ILubyte*)ialloc(Width * Height * Depth * 4);
  if (Data == NULL) {
    return IL_FALSE;
  }

  glGetTexImage(ILGL_TEXTURE_3D, 0, IL_BGRA, GL_UNSIGNED_BYTE, Data);

  iLockState();
  ilutCurImage = iLockCurImage();
  iUnlockState();

  if (!iTexImage(ilutCurImage, Width, Height, Depth, 4, IL_BGRA, IL_UNSIGNED_BYTE, Data)) {
    ifree(Data);
    iUnlockImage(ilutCurImage);
    return IL_FALSE;
  }
  ilutCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

  ifree(Data);
  iUnlockImage(ilutCurImage);
  return IL_TRUE;
}


//! Deprecated - use ilutGLSetTex2D instead.
ILboolean ILAPIENTRY ilutGLSetTex(GLuint TexID)
{
  return ilutGLSetTex2D(TexID);
}


ILboolean ILAPIENTRY ilutGLSetTex2D(GLuint TexID)
{
  ILimage *ilutCurImage;
  ILboolean Result;

  iLockState();
  ilutCurImage = iLockCurImage();
  iUnlockState();

  Result = ilutGLSetTex2D_(ilutCurImage, TexID);
  iUnlockImage(ilutCurImage);
  return Result;
}


ILenum ilutGLFormat(ILenum Format, ILubyte Bpp)
{
  if (Format == IL_RGB || Format == IL_BGR) {
    if (ilutIsEnabled(ILUT_OPENGL_CONV)) {
      return GL_RGB8;
    }
  }
  else if (Format == IL_RGBA || Format == IL_BGRA) {
    if (ilutIsEnabled(ILUT_OPENGL_CONV)) {
      return GL_RGBA8;
    }
  }
  else if (Format == IL_ALPHA) {
    if (ilutIsEnabled(ILUT_OPENGL_CONV)) {
      return GL_ALPHA;
    }
  }

  return Bpp;
}


// From http://www.opengl.org/News/Special/OGLextensions/OGLextensions.html
//  Should we make this accessible outside the lib?
ILboolean IsExtensionSupported(const char *extension)
{
  const GLubyte *extensions;// = NULL;
  const GLubyte *start;
  GLubyte *where, *terminator;

  // Extension names should not have spaces.
  where = (GLubyte *) strchr(extension, ' ');
  if (where || *extension == '\0')
    return IL_FALSE;
  extensions = glGetString(GL_EXTENSIONS);
  if (!extensions)
    return IL_FALSE;
  // It takes a bit of care to be fool-proof about parsing the
  // OpenGL extensions string. Don't be fooled by sub-strings, etc.
  start = extensions;
  for (;;) {
    where = (GLubyte *)strstr((const char *) start, extension);
    if (!where)
      break;
    terminator = where + strlen(extension);
    if (where == start || *(where - 1) == ' ')
    if (*terminator == ' ' || *terminator == '\0')
      return IL_TRUE;
    start = terminator;
  }
  return IL_FALSE;
}


#endif//ILUT_USE_OPENGL
/** @} */
/** @} */
