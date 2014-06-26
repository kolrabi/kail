//-----------------------------------------------------------------------------
//
// ImageLib Utility Toolkit Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 01/30/2009
//
// Filename: src-ILUT/src/ilut_directx9.c
//
// Description: DirectX 9 functions for textures
//
//-----------------------------------------------------------------------------
 
/**
 * @file
 * @brief DirectX 9 functions.
 * @defgroup ILUT
 * @ingroup ILUT
 * @{
 * @defgroup ilut_dx9 DirectX 9 functionality.
 */

#include "ilut_internal.h"

#ifdef ILUT_USE_DIRECTX9

#include <d3d9.h>

typedef struct {
  ILboolean UseDXTC;
  ILuint    MipLevels;
  D3DPOOL   Pool;
  ILboolean GenDXTC;
  ILenum    DXTCFormat;
  ILboolean ForceInteger;
  ILuint    AlphaKey;
  ILenum Filter;
} ILUT_TEXTURE_SETTINGS_DX9;

static ILimage*  MakeD3D9Compliant(ILimage *Image, IDirect3DDevice9 *Device, D3DFORMAT *DestFormat, ILUT_TEXTURE_SETTINGS_DX9 *Settings);
static D3DFORMAT D3DGetDXTCNumDX9(ILenum DXTCFormat);
static ILboolean iD3D9CreateMipmaps(IDirect3DTexture9 *Texture, ILimage *Image, ILUT_TEXTURE_SETTINGS_DX9 *Settings);
static IDirect3DTexture9* iD3DMakeTexture( IDirect3DDevice9 *Device, void *Data, ILuint DLen, ILuint Width, ILuint Height, D3DFORMAT Format, D3DPOOL Pool, ILuint Levels );
static IDirect3DTexture9* iD3D9Texture(ILimage *ilutCurImage, IDirect3DDevice9 *Device, ILUT_TEXTURE_SETTINGS_DX9 *Settings);
static IDirect3DCubeTexture9* iD3D9CubeTexture(ILimage *ilutCurImage, IDirect3DDevice9 *Device, ILUT_TEXTURE_SETTINGS_DX9 *Settings);
static IDirect3DVolumeTexture9* iD3D9VolumeTexture(ILimage *ilutCurImage, IDirect3DDevice9 *Device, ILUT_TEXTURE_SETTINGS_DX9 *Settings);

#define ILUT_TEXTUREFORMAT_D3D9_COUNT   7
static ILboolean FormatsDX9Checked = IL_FALSE;
static ILboolean FormatsDX9supported[ILUT_TEXTUREFORMAT_D3D9_COUNT] =
  { IL_FALSE, IL_FALSE, IL_FALSE, IL_FALSE, IL_FALSE, IL_FALSE, IL_FALSE };
static D3DFORMAT FormatsDX9[ILUT_TEXTUREFORMAT_D3D9_COUNT] =
  { D3DFMT_R8G8B8, D3DFMT_A8R8G8B8, D3DFMT_L8, D3DFMT_DXT1, D3DFMT_DXT3, D3DFMT_DXT5, D3DFMT_A16B16G16R16F};

// called by ilutInit/ilutRenderer
ILboolean ilutD3D9Init() {
  return IL_TRUE;
}

static void GetSettings(ILUT_TEXTURE_SETTINGS_DX9 *settings) {
  settings->UseDXTC     = ilutGetBoolean(ILUT_D3D_USE_DXTC);
  settings->MipLevels   = ilutGetInteger(ILUT_D3D_MIPLEVELS);
  settings->Pool        = (D3DPOOL)ilutGetInteger(ILUT_D3D_POOL);
  settings->GenDXTC     = ilutGetBoolean(ILUT_D3D_GEN_DXTC);
  settings->DXTCFormat  = ilutGetInteger(ILUT_DXTC_FORMAT);
  settings->ForceInteger = ilutGetInteger(ILUT_FORCE_INTEGER_FORMAT);
  settings->AlphaKey    = ilutGetInteger(ILUT_D3D_ALPHA_KEY_COLOR);
  settings->Filter = iluGetInteger(ILU_FILTER);
}

static void CheckFormatsDX9(IDirect3DDevice9 *Device) {
  D3DDISPLAYMODE  DispMode;
  HRESULT     hr;
  IDirect3D9    *TestD3D9;
  ILuint      i;

  IDirect3DDevice9_GetDirect3D(Device, (IDirect3D9**)&TestD3D9);
  IDirect3DDevice9_GetDisplayMode(Device, 0, &DispMode);

  for (i = 0; i < ILUT_TEXTUREFORMAT_D3D9_COUNT; i++) {
    hr = IDirect3D9_CheckDeviceFormat(TestD3D9, D3DADAPTER_DEFAULT,
      D3DDEVTYPE_HAL, DispMode.Format, 0, D3DRTYPE_TEXTURE, FormatsDX9[i]);
    FormatsDX9supported[i] = (ILboolean)SUCCEEDED(hr);
  }

  IDirect3D9_Release(TestD3D9);
  FormatsDX9Checked = IL_TRUE;
}


#ifndef _WIN32_WCE
ILboolean ILAPIENTRY ilutD3D9TexFromFile(IDirect3DDevice9 *Device, ILconst_string FileName, IDirect3DTexture9 **Texture)
{
  ILimage *ilutCurImage;
  ILimage* Temp;
  ILUT_TEXTURE_SETTINGS_DX9 Settings;

  iLockState();
  ilutCurImage = iLockCurImage();
  Temp = ilNewImage(1,1,1, 1,1);
  Temp->io = ilutCurImage->io;
  Temp->io.handle = NULL;
  iUnlockImage(ilutCurImage);

  GetSettings(&Settings);
  iUnlockState();

  if (!iLoad(Temp, IL_TYPE_UNKNOWN, FileName)) {
    ilCloseImage(Temp);
    return IL_FALSE;
  }

  *Texture = iD3D9Texture(Temp, Device, &Settings);
  ilCloseImage(Temp);

  return IL_TRUE;
}
#endif//_WIN32_WCE

#ifndef _WIN32_WCE
/**
 * Load a cube texture image from a file and store it in @a Texture.
 * Uses the following settings:
 * - ILUT_D3D_MIPLEVELS
 * - ILUT_D3D_POOL
 * @ingroup ilut_dx9
 */
ILboolean ILAPIENTRY ilutD3D9CubeTexFromFile(IDirect3DDevice9 *Device,
      ILconst_string FileName, IDirect3DCubeTexture9 **Texture)
{
  ILimage *ilutCurImage;
  ILimage* Temp;
  ILUT_TEXTURE_SETTINGS_DX9 Settings;

  iLockState();
  ilutCurImage = iLockCurImage();
  Temp         = ilNewImage(1,1,1, 1,1);
  Temp->io              = ilutCurImage->io;
  Temp->io.handle       = NULL;
  iUnlockImage(ilutCurImage);

  GetSettings(&Settings);
  iUnlockState();

  if (!iLoad(Temp, IL_TYPE_UNKNOWN, FileName)) {
    ilCloseImage(Temp);
    return IL_FALSE;
  }

  *Texture = iD3D9CubeTexture(Temp, Device, &Settings);
  ilCloseImage(Temp);

  return IL_TRUE;
}

#endif//_WIN32_WCE
ILboolean ILAPIENTRY ilutD3D9CubeTexFromFileInMemory(IDirect3DDevice9 *Device,
        void *Lump, ILuint Size, IDirect3DCubeTexture9 **Texture)
{
  ILimage* Temp;
  ILUT_TEXTURE_SETTINGS_DX9 Settings;

  iLockState();
  Temp = ilNewImage(1,1,1, 1,1);
  GetSettings(&Settings);
  iUnlockState();

  iSetInputLump(Temp, Lump, Size);
  if (!iLoadFuncs2(Temp, IL_TYPE_UNKNOWN)) {
    ilCloseImage(Temp);
    return IL_FALSE;
  }

  *Texture = iD3D9CubeTexture(Temp, Device, &Settings);
  ilCloseImage(Temp);

  return IL_TRUE;   
}
 
ILboolean ILAPIENTRY ilutD3D9CubeTexFromResource(IDirect3DDevice9 *Device,
    HMODULE SrcModule, ILconst_string SrcResource, IDirect3DCubeTexture9 **Texture)
{
  ILimage* Temp;
  ILUT_TEXTURE_SETTINGS_DX9 Settings;
  HRSRC   Resource;
  ILubyte *Data;

  iLockState();
  Temp = ilNewImage(1,1,1, 1,1);
  GetSettings(&Settings);
  iUnlockState();

  Resource = (HRSRC)LoadResource(SrcModule, FindResource(SrcModule, SrcResource, RT_BITMAP));
  Data = (ILubyte*)LockResource(Resource);

  iSetInputLump(Temp, Data, SizeofResource(SrcModule, FindResource(SrcModule, SrcResource, RT_BITMAP)));
  if (!iLoadFuncs2(Temp, IL_TYPE_UNKNOWN)) {
    ilCloseImage(Temp);
    return IL_FALSE;
  }

  *Texture = iD3D9CubeTexture(Temp, Device, &Settings);
  ilCloseImage(Temp);

  return IL_TRUE;
 }

ILboolean ILAPIENTRY ilutD3D9CubeTexFromFileHandle(IDirect3DDevice9 *Device,
      ILHANDLE File, IDirect3DCubeTexture9 **Texture)
{
  ILimage *ilutCurImage;
  ILimage* Temp;
  ILUT_TEXTURE_SETTINGS_DX9 Settings;

  iLockState();
  ilutCurImage = iLockCurImage();
  Temp = ilNewImage(1,1,1, 1,1);
  Temp->io = ilutCurImage->io;
  Temp->io.handle = File;
  iUnlockImage(ilutCurImage);

  GetSettings(&Settings);
  iUnlockState();
  
  if (!iLoadFuncs2(Temp, IL_TYPE_UNKNOWN)) {
    ilCloseImage(Temp);
    return IL_FALSE;
  }

  *Texture = iD3D9CubeTexture(Temp, Device, &Settings);

  ilCloseImage(Temp);
  return IL_TRUE;
}

static D3DCUBEMAP_FACES iToD3D9Cube(ILuint cube) {
  switch (cube) {
    case IL_CUBEMAP_POSITIVEX:      return D3DCUBEMAP_FACE_POSITIVE_X;
    case IL_CUBEMAP_NEGATIVEX:      return D3DCUBEMAP_FACE_NEGATIVE_X;
    case IL_CUBEMAP_POSITIVEY:      return D3DCUBEMAP_FACE_POSITIVE_Y;
    case IL_CUBEMAP_NEGATIVEY:      return D3DCUBEMAP_FACE_NEGATIVE_Y;
    case IL_CUBEMAP_POSITIVEZ:      return D3DCUBEMAP_FACE_POSITIVE_Z;
    case IL_CUBEMAP_NEGATIVEZ:      return D3DCUBEMAP_FACE_NEGATIVE_Z;
    default:                        return D3DCUBEMAP_FACE_POSITIVE_X; //???
  }
}
 
static IDirect3DCubeTexture9* iD3D9CubeTexture(ILimage *ilutCurImage, IDirect3DDevice9 *Device, ILUT_TEXTURE_SETTINGS_DX9 *Settings) {
  IDirect3DCubeTexture9 * Texture;
  D3DLOCKED_RECT          Box;
  D3DFORMAT               Format;
  ILimage *               StartImage;
  ILimage *               Image;
  ILint                   i;

  Texture = NULL;
  Image = NULL;

  if (ilutCurImage == NULL) {
    iSetError(ILUT_ILLEGAL_OPERATION);
    return NULL;
  }

  if (!FormatsDX9Checked)
    CheckFormatsDX9(Device);

  Image = MakeD3D9Compliant(ilutCurImage, Device, &Format, Settings);
  if (Image == NULL) {
    return NULL;
  }

  StartImage = ilutCurImage;
  if (FAILED(
        IDirect3DDevice9_CreateCubeTexture(
          Device, StartImage->Width,
          Settings->MipLevels,0,Format, 
          Settings->Pool, &Texture, NULL
        )
      )) {
    return NULL;
  }
  for (i = 0; i < CUBEMAP_SIDES; i++) {
    if (ilutCurImage == NULL || ilutCurImage->CubeFlags == 0) {
        SAFE_RELEASE(Texture)
        return NULL;
    }

    Image = MakeD3D9Compliant(ilutCurImage, Device, &Format, Settings);
    if( Image == NULL ) {
      SAFE_RELEASE(Texture)
      return NULL;
    }

    if( FAILED(IDirect3DCubeTexture9_LockRect(Texture,iToD3D9Cube(Image->CubeFlags), 0, &Box, NULL, /*D3DLOCK_DISCARD*/0))) {
      SAFE_RELEASE(Texture)
      if (Image != ilutCurImage)
        ilCloseImage(Image);
      return NULL;
    }

    memcpy(Box.pBits, Image->Data, Image->SizeOfData);
    if (IDirect3DCubeTexture9_UnlockRect(Texture,iToD3D9Cube(Image->CubeFlags), 0) != D3D_OK) {
      SAFE_RELEASE(Texture)
      if (Image != ilutCurImage)
        ilCloseImage(Image);
      return IL_FALSE;
    }
    ilutCurImage = ilutCurImage->Faces;

    if (Image != ilutCurImage)
      ilCloseImage(Image);
  }

  ilutCurImage = StartImage;
  return Texture;
}

IDirect3DCubeTexture9* ILAPIENTRY ilutD3D9CubeTexture(IDirect3DDevice9 *Device) {
  ILUT_TEXTURE_SETTINGS_DX9 Settings;
  ILimage * Image;
  IDirect3DCubeTexture9 * Result;

  iLockState();
  Image     = iLockCurImage();
  GetSettings(&Settings);
  iUnlockState();

  Result = iD3D9CubeTexture(Image, Device, &Settings);
  iUnlockImage(Image);
  return Result;
}

#ifndef _WIN32_WCE
ILboolean ILAPIENTRY ilutD3D9VolTexFromFile(IDirect3DDevice9 *Device, ILconst_string FileName, IDirect3DVolumeTexture9 **Texture) {
  ILUT_TEXTURE_SETTINGS_DX9 Settings;
  ILimage * ilutCurImage;
  ILimage * Temp;

  iLockState();
  ilutCurImage = iLockCurImage();
  Temp = ilNewImage(1,1,1, 1,1);
  Temp->io = ilutCurImage->io;
  Temp->io.handle = NULL;
  iUnlockImage(ilutCurImage);

  GetSettings(&Settings);
  iUnlockState();

  if (!iLoad(Temp, IL_TYPE_UNKNOWN, FileName)) {
    ilCloseImage(Temp);
    return IL_FALSE;
  }

  *Texture = iD3D9VolumeTexture(Temp, Device, &Settings);
  ilCloseImage(Temp);

  return IL_TRUE;  
}
#endif//_WIN32_WCE


ILboolean ILAPIENTRY ilutD3D9TexFromFileInMemory(IDirect3DDevice9 *Device, void *Lump, ILuint Size, IDirect3DTexture9 **Texture)
{
  ILUT_TEXTURE_SETTINGS_DX9 Settings;
  ILimage * Temp;

  iLockState();
  Temp = ilNewImage(1,1,1, 1,1);
  GetSettings(&Settings);
  iUnlockState();

  iSetInputLump(Temp, Lump, Size);
  if (!iLoadFuncs2(Temp, IL_TYPE_UNKNOWN)) {
    ilCloseImage(Temp);
    return IL_FALSE;
  }

  *Texture = iD3D9Texture(Temp, Device, &Settings);
  ilCloseImage(Temp);

  return IL_TRUE;  }


ILboolean ILAPIENTRY ilutD3D9VolTexFromFileInMemory(IDirect3DDevice9 *Device, void *Lump, ILuint Size, IDirect3DVolumeTexture9 **Texture)
{
  ILUT_TEXTURE_SETTINGS_DX9 Settings;
  ILimage * Temp;

  iLockState();
  Temp = ilNewImage(1,1,1, 1,1);
  GetSettings(&Settings);
  iUnlockState();

  iSetInputLump(Temp, Lump, Size);
  if (!iLoadFuncs2(Temp, IL_TYPE_UNKNOWN)) {
    ilCloseImage(Temp);
    return IL_FALSE;
  }

  *Texture = iD3D9VolumeTexture(Temp, Device, &Settings);
  ilCloseImage(Temp);

  return IL_TRUE;  
}


ILboolean ILAPIENTRY ilutD3D9TexFromResource(IDirect3DDevice9 *Device, HMODULE SrcModule, ILconst_string SrcResource, IDirect3DTexture9 **Texture)
{
  ILUT_TEXTURE_SETTINGS_DX9 Settings;
  ILimage * Temp;
  HRSRC Resource;
  ILubyte *Data;

  Resource = (HRSRC)LoadResource(SrcModule, FindResource(SrcModule, SrcResource, RT_BITMAP));
  Data = (ILubyte*)LockResource(Resource);

  iLockState();
  Temp = ilNewImage(1,1,1, 1,1);
  GetSettings(&Settings);
  iUnlockState();

  iSetInputLump(Temp, Data, SizeofResource(SrcModule, FindResource(SrcModule, SrcResource, RT_BITMAP)));
  if (!iLoadFuncs2(Temp, IL_TYPE_UNKNOWN)) {
    ilCloseImage(Temp);
    return IL_FALSE;
  }

  *Texture = iD3D9Texture(Temp, Device, &Settings);
  ilCloseImage(Temp);

  return IL_TRUE;
}


ILboolean ILAPIENTRY ilutD3D9VolTexFromResource(IDirect3DDevice9 *Device, HMODULE SrcModule, ILconst_string SrcResource, IDirect3DVolumeTexture9 **Texture)
{
  ILUT_TEXTURE_SETTINGS_DX9 Settings;
  ILimage * Temp;
  HRSRC Resource;
  ILubyte *Data;

  Resource = (HRSRC)LoadResource(SrcModule, FindResource(SrcModule, SrcResource, RT_BITMAP));
  Data = (ILubyte*)LockResource(Resource);

  iLockState();
  Temp = ilNewImage(1,1,1, 1,1);
  GetSettings(&Settings);
  iUnlockState();

  iSetInputLump(Temp, Data, SizeofResource(SrcModule, FindResource(SrcModule, SrcResource, RT_BITMAP)));
  if (!iLoadFuncs2(Temp, IL_TYPE_UNKNOWN)) {
    ilCloseImage(Temp);
    return IL_FALSE;
  }

  *Texture = iD3D9VolumeTexture(Temp, Device, &Settings);
  ilCloseImage(Temp);

  return IL_TRUE;
}


ILboolean ILAPIENTRY ilutD3D9TexFromFileHandle(IDirect3DDevice9 *Device, ILHANDLE File, IDirect3DTexture9 **Texture)
{
  ILUT_TEXTURE_SETTINGS_DX9 Settings;
  ILimage * ilutCurImage;
  ILimage * Temp;

  iLockState();
  ilutCurImage = iLockCurImage();
  Temp = ilNewImage(1,1,1, 1,1);
  Temp->io = ilutCurImage->io;
  Temp->io.handle = File;
  iUnlockImage(ilutCurImage);

  GetSettings(&Settings);
  iUnlockState();
  
  if (!iLoadFuncs2(Temp, IL_TYPE_UNKNOWN)) {
    ilCloseImage(Temp);
    return IL_FALSE;
  }

  *Texture = iD3D9Texture(Temp, Device, &Settings);
  ilCloseImage(Temp);
  
  return IL_TRUE;
}


ILboolean ILAPIENTRY ilutD3D9VolTexFromFileHandle(IDirect3DDevice9 *Device, ILHANDLE File, IDirect3DVolumeTexture9 **Texture)
{
  ILUT_TEXTURE_SETTINGS_DX9 Settings;
  ILimage * ilutCurImage;
  ILimage * Temp;

  iLockState();
  ilutCurImage = iLockCurImage();
  Temp = ilNewImage(1,1,1, 1,1);
  Temp->io = ilutCurImage->io;
  Temp->io.handle = File;
  iUnlockImage(ilutCurImage);

  GetSettings(&Settings);
  iUnlockState();
  
  if (!iLoadFuncs2(Temp, IL_TYPE_UNKNOWN)) {
    ilCloseImage(Temp);
    return IL_FALSE;
  }

  *Texture = iD3D9VolumeTexture(Temp, Device, &Settings);
  ilCloseImage(Temp);

  return IL_TRUE;  
}


D3DFORMAT D3DGetDXTCNumDX9(ILenum DXTCFormat)
{
  switch (DXTCFormat)
  {
    case IL_DXT1:
      return D3DFMT_DXT1;
    case IL_DXT3:
      return D3DFMT_DXT3;
    case IL_DXT5:
      return D3DFMT_DXT5;
  }

  return D3DFMT_UNKNOWN;
}

/*
ILenum D3DGetDXTCFormat(D3DFORMAT DXTCNum)
{
  switch (DXTCNum)
  {
    case D3DFMT_DXT1:
      return IL_DXT1;
    case D3DFMT_DXT3:
      return IL_DXT3;
    case D3DFMT_DXT5:
      return IL_DXT5;
    default:
      return D3DFMT_UNKNOWN;
  }

  return D3DFMT_UNKNOWN;
}
*/

IDirect3DTexture9* iD3DMakeTexture( IDirect3DDevice9 *Device, void *Data, ILuint DLen, ILuint Width, ILuint Height, D3DFORMAT Format, D3DPOOL Pool, ILuint Levels )
{
  IDirect3DTexture9 *Texture;
  D3DLOCKED_RECT Rect;
  
  if (FAILED(IDirect3DDevice9_CreateTexture(Device, Width, Height, Levels,
      0, Format, Pool, &Texture, NULL)))
    return NULL;
  if (FAILED(IDirect3DTexture9_LockRect(Texture, 0, &Rect, NULL, 0)))
    return NULL;
  memcpy(Rect.pBits, Data, DLen);
  IDirect3DTexture9_UnlockRect(Texture, 0);
  
  return Texture;
}

static IDirect3DTexture9* iD3D9Texture(ILimage *ilutCurImage, IDirect3DDevice9 *Device, ILUT_TEXTURE_SETTINGS_DX9 *Settings) {
  IDirect3DTexture9 * Texture;
  D3DFORMAT           Format;
  ILimage *           Image;
  ILuint              Size;
  ILubyte *           Buffer;

  if (ilutCurImage == NULL) {
    iSetError(ILUT_ILLEGAL_OPERATION);
    return NULL;
  }

  Image = ilutCurImage;

  if (!FormatsDX9Checked)
    CheckFormatsDX9(Device);

  if (Settings->UseDXTC && FormatsDX9supported[3] && FormatsDX9supported[4] && FormatsDX9supported[5]) {
    if (ilutCurImage->DxtcData != NULL && ilutCurImage->DxtcSize != 0) {
      ILuint  dxtcFormat = Settings->DXTCFormat;
      Format = D3DGetDXTCNumDX9(ilutCurImage->DxtcFormat);
      ilutSetInteger(ILUT_DXTC_FORMAT, ilutCurImage->DxtcFormat);

      Texture = iD3DMakeTexture(Device, ilutCurImage->DxtcData, ilutCurImage->DxtcSize,
        ilutCurImage->Width, ilutCurImage->Height, Format,
        ((Settings->Pool == D3DPOOL_DEFAULT) ? D3DPOOL_SYSTEMMEM : Settings->Pool), Settings->MipLevels);
      if (!Texture)
        return NULL;
      iD3D9CreateMipmaps(Texture, Image, Settings);
      if (Settings->Pool == D3DPOOL_DEFAULT) {
        IDirect3DTexture9 *SysTex = Texture;
        // copy texture to device memory
        if (FAILED(IDirect3DDevice9_CreateTexture(Device, ilutCurImage->Width,
            ilutCurImage->Height, Settings->MipLevels, 0, Format,
            D3DPOOL_DEFAULT, &Texture, NULL))) {
          IDirect3DTexture9_Release(SysTex);
          return NULL;
        }
        if (FAILED(IDirect3DDevice9_UpdateTexture(Device, (LPDIRECT3DBASETEXTURE9)SysTex, (LPDIRECT3DBASETEXTURE9)Texture))) {
          IDirect3DTexture9_Release(SysTex);
          return NULL;
        }
        IDirect3DTexture9_Release(SysTex);
      }
      ilutSetInteger(ILUT_DXTC_FORMAT, dxtcFormat);

      goto success;
    }

    if (Settings->GenDXTC) {
      Size = ilGetDXTCData(NULL, 0, Settings->DXTCFormat);
      if (Size != 0) {
        Buffer = (ILubyte*)ialloc(Size);
        if (Buffer == NULL)
          return NULL;
        Size = ilGetDXTCData(Buffer, Size, Settings->DXTCFormat);
        if (Size == 0) {
          ifree(Buffer);
          return NULL;
        }

        Format = D3DGetDXTCNumDX9(Settings->DXTCFormat);
        Texture = iD3DMakeTexture(Device, Buffer, Size,
          ilutCurImage->Width, ilutCurImage->Height, Format,
          ((Settings->Pool == D3DPOOL_DEFAULT) ? D3DPOOL_SYSTEMMEM : Settings->Pool), Settings->MipLevels);
        if (!Texture)
          return NULL;
        iD3D9CreateMipmaps(Texture, Image, Settings);
        if (Settings->Pool == D3DPOOL_DEFAULT) {
          IDirect3DTexture9 *SysTex = Texture;
          
          if (FAILED(IDirect3DDevice9_CreateTexture(Device, ilutCurImage->Width,
              ilutCurImage->Height, Settings->MipLevels, 0, Format,
              D3DPOOL_DEFAULT, &Texture, NULL))) {
            IDirect3DTexture9_Release(SysTex);
            return NULL;
          }
          if (FAILED(IDirect3DDevice9_UpdateTexture(Device, (LPDIRECT3DBASETEXTURE9)SysTex, (LPDIRECT3DBASETEXTURE9)Texture))) {
            IDirect3DTexture9_Release(SysTex);
            return NULL;
          }
          IDirect3DTexture9_Release(SysTex);
        }
        
        goto success;
      }
    }
  }

  Image = MakeD3D9Compliant(ilutCurImage, Device, &Format, Settings);
  if (Image == NULL) {
    return NULL;
  }

  Texture = iD3DMakeTexture(Device, Image->Data, Image->SizeOfPlane,
    Image->Width, Image->Height, Format,
    ((Settings->Pool == D3DPOOL_DEFAULT) ? D3DPOOL_SYSTEMMEM : Settings->Pool), Settings->MipLevels);
  
  if (!Texture) {
    if (Image != ilutCurImage)
      ilCloseImage(Image);    
    return NULL;
  }

  iD3D9CreateMipmaps(Texture, Image, Settings);
  if (Settings->Pool == D3DPOOL_DEFAULT) {
    IDirect3DTexture9 *SysTex = Texture;
    // create texture in system memory
    if (FAILED(IDirect3DDevice9_CreateTexture(Device, Image->Width,
        Image->Height, Settings->MipLevels, 0, Format,
        Settings->Pool, &Texture, NULL))) {
      IDirect3DTexture9_Release(SysTex);
      if (Image != ilutCurImage)
        ilCloseImage(Image);
      return NULL;
    }

    if (FAILED(IDirect3DDevice9_UpdateTexture(Device, (LPDIRECT3DBASETEXTURE9)SysTex, (LPDIRECT3DBASETEXTURE9)Texture))) {
      IDirect3DTexture9_Release(SysTex);
      if (Image != ilutCurImage)
        ilCloseImage(Image);
      return NULL;
    }

    IDirect3DTexture9_Release(SysTex);
  }

success:

  if (Image != ilutCurImage)
    ilCloseImage(Image);

  return Texture;
}


IDirect3DTexture9* ILAPIENTRY ilutD3D9Texture(IDirect3DDevice9 *Device) {
  ILUT_TEXTURE_SETTINGS_DX9 Settings;
  ILimage * Image;
  IDirect3DTexture9 *Result;

  iLockState();
  Image = iLockCurImage();
  GetSettings(&Settings);
  iUnlockState();

  Result = iD3D9Texture(Image, Device, &Settings);
  iUnlockImage(Image);
  return Result;
}

static IDirect3DVolumeTexture9* iD3D9VolumeTexture(ILimage *ilutCurImage, IDirect3DDevice9 *Device, ILUT_TEXTURE_SETTINGS_DX9 *Settings) {
  IDirect3DVolumeTexture9 *Texture;
  D3DLOCKED_BOX Box;
  D3DFORMAT   Format;
  ILimage     *Image;

  if (ilutCurImage == NULL) {
    iSetError(ILUT_ILLEGAL_OPERATION);
    return NULL;
  }

  if (!FormatsDX9Checked)
    CheckFormatsDX9(Device);

  Image = MakeD3D9Compliant(ilutCurImage, Device, &Format, Settings);
  if (Image == NULL)
    return NULL;

  if (FAILED(IDirect3DDevice9_CreateVolumeTexture(Device, Image->Width, Image->Height,
    Image->Depth, 1, 0, Format, Settings->Pool, &Texture, NULL))) {
    if (Image != ilutCurImage)
      ilCloseImage(Image);
    return NULL;  
  }

  if (FAILED(IDirect3DVolumeTexture9_LockBox(Texture, 0, &Box, NULL, 0))) {
    if (Image != ilutCurImage)
      ilCloseImage(Image);
    return NULL;
  }

  memcpy(Box.pBits, Image->Data, Image->SizeOfData);
  if (!IDirect3DVolumeTexture9_UnlockBox(Texture, 0)) {
    if (Image != ilutCurImage)
      ilCloseImage(Image);
    return NULL;
  }

  // We don't want to have mipmaps for such a large image.

  if (Image != ilutCurImage)
    ilCloseImage(Image);

  return Texture;
}


IDirect3DVolumeTexture9* ILAPIENTRY ilutD3D9VolumeTexture(IDirect3DDevice9 *Device) {
  ILUT_TEXTURE_SETTINGS_DX9 Settings;
  ILimage *Image;
  IDirect3DVolumeTexture9 *Result;

  iLockState();
  Image = iLockCurImage();
  GetSettings(&Settings);
  iUnlockState();

  Result = iD3D9VolumeTexture(Image, Device, &Settings);
  iUnlockImage(Image);
  return Result;
}


static ILimage *MakeD3D9Compliant(ILimage *ilutCurImage, IDirect3DDevice9 *Device, D3DFORMAT *DestFormat, ILUT_TEXTURE_SETTINGS_DX9 *Settings) {
  ILuint  color;
  ILimage *Converted, *Scaled;
  ILuint nConversionType, ilutFormat;
  ILboolean bForceIntegerFormat = Settings->ForceInteger;

  (void)Device;

  if (!ilutCurImage)
    return NULL;

  ilutFormat      = ilutCurImage->Format;
  nConversionType = ilutCurImage->Type;

  switch (ilutCurImage->Type) {
    case IL_UNSIGNED_BYTE:
    case IL_BYTE:
    case IL_SHORT:
    case IL_UNSIGNED_SHORT:
    case IL_INT:
    case IL_UNSIGNED_INT:
      *DestFormat     = D3DFMT_A8R8G8B8;
      nConversionType = IL_UNSIGNED_BYTE;
      ilutFormat      = IL_BGRA;
      break;
    case IL_FLOAT:
    case IL_DOUBLE:
    case IL_HALF:
      if (bForceIntegerFormat || (!FormatsDX9supported[6]))
      {
        *DestFormat     = D3DFMT_A8R8G8B8;
        nConversionType = IL_UNSIGNED_BYTE;
        ilutFormat      = IL_BGRA;
      }
      else
      {
        *DestFormat     = D3DFMT_A16B16G16R16F;
        nConversionType = IL_HALF;
        ilutFormat      = IL_RGBA;

        //
      }
      break;
  }

  // Images must be in BGRA format.
  if (((ilutCurImage->Format != ilutFormat))
    || (ilutCurImage->Type != nConversionType)) 
  {
    Converted = iConvertImage(ilutCurImage, ilutFormat, nConversionType);
    if (Converted == NULL)
      return NULL;
  }
  else 
  {
    Converted = ilutCurImage;
  }

  // perform alpha key on images if requested
  color = Settings->AlphaKey;

  if(nConversionType == IL_UNSIGNED_BYTE)
  {
    ILubyte *data;
    ILubyte *maxdata;
    ILuint t;

      data=(Converted->Data);
      maxdata=(Converted->Data+Converted->SizeOfData);
      while(data<maxdata)
      {
        t= (data[2]<<16) + (data[1]<<8) + (data[0]) ;

        if((t&0x00ffffff)==(color&0x00ffffff))
        {
          data[0]=0;
          data[1]=0;
          data[2]=0;
          data[3]=0;
        }
        data+=4;
      }
  }

  // Images must have their origin in the upper left.
  if (Converted->Origin != IL_ORIGIN_UPPER_LEFT) {
    iFlipImage(Converted);
  }

  // Images must have powers-of-2 dimensions.
  if (ilNextPower2(ilutCurImage->Width)  != ilutCurImage->Width ||
      ilNextPower2(ilutCurImage->Height) != ilutCurImage->Height ||
      ilNextPower2(ilutCurImage->Depth)  != ilutCurImage->Depth) {
      Scaled = iluScale_(Converted, ilNextPower2(ilutCurImage->Width),
            ilNextPower2(ilutCurImage->Height), ilNextPower2(ilutCurImage->Depth), Settings->Filter);
      if (Converted != ilutCurImage) {
        ilCloseImage(Converted);
      }
      if (Scaled == NULL) {
        return NULL;
      }
      Converted = Scaled;
  }

  return Converted;
}


static ILboolean iD3D9CreateMipmaps(IDirect3DTexture9 *Texture, ILimage *Image, ILUT_TEXTURE_SETTINGS_DX9 *Settings) {
  D3DLOCKED_RECT  Rect;
  D3DSURFACE_DESC Desc;
  ILuint      NumMips,  srcMips, Width, Height, i;
  ILimage     *MipImage, *Temp;
  ILuint      Size;
  ILubyte     *Buffer;
  ILboolean   useDXTC = IL_FALSE;

  NumMips = IDirect3DTexture9_GetLevelCount(Texture);
  Width = Image->Width;
  Height = Image->Height;

  if (NumMips == 1)
    return IL_TRUE;

  MipImage = Image;
  srcMips = Settings->MipLevels;;
  if ( srcMips != NumMips-1) {
    MipImage = ilCopyImage_(Image);
    if (!iBuildMipmaps(MipImage, MipImage->Width >> 1, MipImage->Height >> 1, MipImage->Depth >> 1)) {
      ilCloseImage(MipImage);
      return IL_FALSE;
    }
  }

  Temp = MipImage->Mipmaps;

  if (Settings->UseDXTC && FormatsDX9supported[3] && FormatsDX9supported[4] && FormatsDX9supported[5])
    useDXTC = IL_TRUE;

  // Counts the base texture as 1.
  for (i = 1; i < NumMips && Temp != NULL; i++) {
    // ilSetCurImage(Temp);
    if (FAILED(IDirect3DTexture9_LockRect(Texture, i, &Rect, NULL, 0)))
      return IL_FALSE;

    Width = IL_MAX(1, Width / 2);
    Height = IL_MAX(1, Height / 2);

    IDirect3DTexture9_GetLevelDesc(Texture, i, &Desc);
    if (Desc.Width != Width || Desc.Height != Height) {
      IDirect3DTexture9_UnlockRect(Texture, i);
      return IL_FALSE;
    }

    if (useDXTC) {
      if (Temp->DxtcData != NULL && Temp->DxtcSize != 0) {
        memcpy(Rect.pBits, Temp->DxtcData, Temp->DxtcSize);
      } else if (Settings->GenDXTC) {
        Size = iGetDXTCData(Temp, NULL, 0, Settings->DXTCFormat);
        if (Size != 0) {
          Buffer = (ILubyte*)ialloc(Size);
          if (Buffer == NULL) {
            IDirect3DTexture9_UnlockRect(Texture, i);
            return IL_FALSE;
          }
          Size = iGetDXTCData(Temp, Buffer, Size, Settings->DXTCFormat);
          if (Size == 0) {
            ifree(Buffer);
            IDirect3DTexture9_UnlockRect(Texture, i);
            return IL_FALSE;
          }
          memcpy(Rect.pBits, Buffer, Size);
        } else {
          IDirect3DTexture9_UnlockRect(Texture, i);
          return IL_FALSE;
        }
      } else {
        IDirect3DTexture9_UnlockRect(Texture, i);
        return IL_FALSE;
      }
    } else
      memcpy(Rect.pBits, Temp->Data, Temp->SizeOfData);

    IDirect3DTexture9_UnlockRect(Texture, i);
    Temp = Temp->Mipmaps;
  }

  if (MipImage != Image)
    ilCloseImage(MipImage);

  return IL_TRUE;
}


//
// SaveSurfaceToFile.cpp
//
// Copyright (c) 2001 David Galeano
//
// Permission to use, copy, modify and distribute this software
// is hereby granted, provided that both the copyright notice and 
// this permission notice appear in all copies of the software, 
// derivative works or modified versions.
//
// THE AUTHOR ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
// CONDITION AND DISCLAIMS ANY LIABILITY OF ANY KIND FOR ANY DAMAGES 
// WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
//

static ILboolean iD3D9LoadSurface(ILimage *ilutCurImage, IDirect3DDevice9 *Device, IDirect3DSurface9 *Surface) {
  HRESULT       hr;
  D3DSURFACE_DESC   d3dsd;
  LPDIRECT3DSURFACE9  SurfaceCopy;
  D3DLOCKED_RECT    d3dLR;
  ILboolean     bHasAlpha;
  ILubyte       *Image, *ImageAux, *Data;
  ILuint        y, x;
  ILushort      dwColor;

  IDirect3DSurface9_GetDesc(Surface, &d3dsd);

  bHasAlpha = (ILboolean)(d3dsd.Format == D3DFMT_A8R8G8B8 || d3dsd.Format == D3DFMT_A1R5G5B5);

  if (bHasAlpha) {
    if (!iTexImage(ilutCurImage, d3dsd.Width, d3dsd.Height, 1, 4, IL_BGRA, IL_UNSIGNED_BYTE, NULL)) {
      return IL_FALSE;
    }
  }
  else {
    if (!iTexImage(ilutCurImage, d3dsd.Width, d3dsd.Height, 1, 3, IL_BGR, IL_UNSIGNED_BYTE, NULL)) {
      return IL_FALSE;
    }
  }

  hr = IDirect3DDevice9_CreateOffscreenPlainSurface(Device, d3dsd.Width, d3dsd.Height, d3dsd.Format, D3DPOOL_SCRATCH, &SurfaceCopy, NULL);
  if (FAILED(hr)) {
    iSetError(ILUT_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  hr = IDirect3DDevice9_StretchRect(Device, Surface, NULL, SurfaceCopy, NULL, D3DTEXF_NONE);
  if (FAILED(hr)) {
    iSetError(ILUT_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  hr = IDirect3DSurface9_LockRect(SurfaceCopy, &d3dLR, NULL, D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_NOSYSLOCK | D3DLOCK_READONLY);
  if (FAILED(hr)) {
    IDirect3DSurface9_Release(SurfaceCopy);
    iSetError(ILUT_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  Image = (ILubyte*)d3dLR.pBits;
  Data = ilutCurImage->Data;

  for (y = 0; y < d3dsd.Height; y++) {
    if (d3dsd.Format == D3DFMT_X8R8G8B8) {
      ImageAux = Image;
      for (x = 0; x < d3dsd.Width; x++) {
        Data[0] = ImageAux[0];
        Data[1] = ImageAux[1];
        Data[2] = ImageAux[2];

        Data += 3;
        ImageAux += 4;
      }
    }
    else if (d3dsd.Format == D3DFMT_A8R8G8B8) {
      memcpy(Data, Image, d3dsd.Width * 4);
    }
    else if (d3dsd.Format == D3DFMT_R5G6B5) {
      ImageAux = Image;
      for (x = 0; x < d3dsd.Width; x++) {
        dwColor = *((ILushort*)ImageAux);

        Data[0] = (ILubyte)((dwColor&0x001f)<<3);
        Data[1] = (ILubyte)(((dwColor&0x7e0)>>5)<<2);
        Data[2] = (ILubyte)(((dwColor&0xf800)>>11)<<3);

        Data += 3;
        ImageAux += 2;
      }
    }
    else if (d3dsd.Format == D3DFMT_X1R5G5B5) {
      ImageAux = Image;
      for (x = 0; x < d3dsd.Width; x++) {
        dwColor = *((ILushort*)ImageAux);

        Data[0] = (ILubyte)((dwColor&0x001f)<<3);
        Data[1] = (ILubyte)(((dwColor&0x3e0)>>5)<<3);
        Data[2] = (ILubyte)(((dwColor&0x7c00)>>10)<<3);

        Data += 3;
        ImageAux += 2;
      }
    }
    else if (d3dsd.Format == D3DFMT_A1R5G5B5) {
      ImageAux = Image;
      for (x = 0; x < d3dsd.Width; x++) {
        dwColor = *((ILushort*)ImageAux);

        Data[0] = (ILubyte)((dwColor&0x001f)<<3);
        Data[1] = (ILubyte)(((dwColor&0x3e0)>>5)<<3);
        Data[2] = (ILubyte)(((dwColor&0x7c00)>>10)<<3);
        Data[3] = (ILubyte)(((dwColor&0x8000)>>15)*255);

        Data += 4;
        ImageAux += 2;
      }
    }

    Image += d3dLR.Pitch;
  }

  IDirect3DSurface9_UnlockRect(SurfaceCopy);
  IDirect3DSurface9_Release(SurfaceCopy);

  return IL_TRUE;
}


ILAPI ILboolean ILAPIENTRY ilutD3D9LoadSurface(IDirect3DDevice9 *Device, IDirect3DSurface9 *Surface) {
  ILimage * Image;
  ILboolean Result;

  iLockState();
  Image     = iLockCurImage();
  iUnlockState();

  Result = iD3D9LoadSurface(Image, Device, Surface);
  iUnlockImage(Image);
  return Result;
}

#endif//ILUT_USE_DIRECTX9

/** @} */
