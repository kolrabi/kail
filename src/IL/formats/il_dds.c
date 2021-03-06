//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 02/28/2009
//
// Filename: src-IL/src/il_dds.c
//
// Description: Reads from a DirectDraw Surface (.dds) file.
//
//-----------------------------------------------------------------------------


//  However, some not really valid .dds files are also read, for example
//  Volume Textures without the COMPLEX bit set, so the specs aren't taken
//  too strictly while reading.


#include "il_internal.h"
#ifndef IL_NO_DDS
#include "il_dds.h"
#include "il_manip.h"

#include "pack_push.h"

typedef struct {
  ILubyte alpha[8];
  ILubyte colours[4];
  ILuint  mask;
} DxtcDXT3Data;

typedef struct {
  ILubyte alpha[2];
  ILubyte alphamask[6];
  ILubyte colours[4];
  ILuint  mask;
} DxtcDXT5Data;

#include "pack_pop.h"

static ILboolean  ReadData(DDS_CONTEXT *);
static ILboolean  AllocImage(DDS_CONTEXT *, ILuint CompFormat);
static ILboolean  DdsDecompress(DDS_CONTEXT *, ILuint CompFormat);
static ILboolean  ReadMipmaps(DDS_CONTEXT *, ILuint CompFormat);
static void       AdjustVolumeTexture(DDS_CONTEXT *, ILuint CompFormat);

static ILboolean  DecompressARGB(DDS_CONTEXT *, ILuint CompFormat);
static ILboolean  DecompressFloat(ILimage *Image, ILuint CompFormat, const void *CompData, ILuint CompSize);
static ILboolean  DecompressARGB16(DDS_CONTEXT *, ILuint CompFormat);

static void       CorrectPreMult(ILimage *image);
static void       GetBitsFromMask(ILuint Mask, ILuint *ShiftLeft, ILuint *ShiftRight);

static const ILuint CubemapDirections[CUBEMAP_SIDES] = {
  DDS_CUBEMAP_POSITIVEX,
  DDS_CUBEMAP_NEGATIVEX,
  DDS_CUBEMAP_POSITIVEY,
  DDS_CUBEMAP_NEGATIVEY,
  DDS_CUBEMAP_POSITIVEZ,
  DDS_CUBEMAP_NEGATIVEZ
};


// Internal function used to get the .dds header from the current file.
static ILboolean iGetDdsHead(SIO* io, DDSHEAD *Header)
{
  ILint i;

  SIOread(io, &Header->Signature, 1, 4);
  Header->Size1 = GetLittleUInt(io);
  Header->Flags1 = GetLittleUInt(io);
  Header->Height = GetLittleUInt(io);
  Header->Width = GetLittleUInt(io);
  Header->LinearSize = GetLittleUInt(io);
  Header->Depth = GetLittleUInt(io);
  Header->MipMapCount = GetLittleUInt(io);

  for (i = 0; i < 11; ++i)
    Header->Reserved[i] = GetLittleUInt(io);

  Header->Size2 = GetLittleUInt(io);
  Header->Flags2 = GetLittleUInt(io);
  Header->FourCC = GetLittleUInt(io);
  Header->RGBBitCount = GetLittleUInt(io);
  Header->RBitMask = GetLittleUInt(io);
  Header->GBitMask = GetLittleUInt(io);
  Header->BBitMask = GetLittleUInt(io);
  Header->RGBAlphaBitMask = GetLittleUInt(io);
  Header->ddsCaps1 = GetLittleUInt(io);
  Header->ddsCaps2 = GetLittleUInt(io);
  Header->ddsCaps3 = GetLittleUInt(io);
  Header->ddsCaps4 = GetLittleUInt(io);
  Header->TextureStage = GetLittleUInt(io);

  if (Header->Depth == 0)
    Header->Depth = 1;

  return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidDds(SIO* io)
{
  char Sig[4];
  ILuint Start = SIOtell(io);
  ILuint Read = SIOread(io, Sig, 1, 4);
  SIOseek(io, Start, IL_SEEK_SET);
  return Read == 4 && memcmp(Sig, "DDS ", 4) == 0;
}


// Internal function used to check if the HEADER is a valid .dds header.
ILboolean iCheckDds(DDSHEAD *Head)
{
  if (strncmp((const char*)Head->Signature, "DDS ", 4))
    return IL_FALSE;
  //note that if Size1 is "DDS " this is not a valid dds file according
  //to the file spec. Some broken tool out there seems to produce files
  //with this value in the size field, so we support reading them...
  if (Head->Size1 != 124 && Head->Size1 != IL_MAKEFOURCC('D', 'D', 'S', ' '))
    return IL_FALSE;
  if (Head->Size2 != 32)
    return IL_FALSE;
  if (Head->Width == 0 || Head->Height == 0)
    return IL_FALSE;
  return IL_TRUE;
}


static void Check16BitComponents(DDS_CONTEXT *ctx)
{
  if (ctx->Head.RGBBitCount != 32)
    ctx->Has16BitComponents = IL_FALSE;
  // a2b10g10r10 format
  if (ctx->Head.RBitMask == 0x3FF00000 && ctx->Head.GBitMask == 0x000FFC00 && ctx->Head.BBitMask == 0x000003FF
    && ctx->Head.RGBAlphaBitMask == 0xC0000000)
    ctx->Has16BitComponents = IL_TRUE;
  // a2r10g10b10 format
  else if (ctx->Head.RBitMask == 0x000003FF && ctx->Head.GBitMask == 0x000FFC00 && ctx->Head.BBitMask == 0x3FF00000
    && ctx->Head.RGBAlphaBitMask == 0xC0000000)
    ctx->Has16BitComponents = IL_TRUE;
  else
    ctx->Has16BitComponents = IL_FALSE;
}


static ILubyte iCompFormatToBpp(DDS_CONTEXT *ctx, ILenum Format)
{
  //non-compressed (= non-FOURCC) codes
  if (Format == PF_LUMINANCE || Format == PF_LUMINANCE_ALPHA || Format == PF_ARGB)
    return (ILubyte)(ctx->Head.RGBBitCount/8);

  //fourcc formats
  else if (Format == PF_RGB || Format == PF_3DC || Format == PF_RXGB)
    return 3;
  else if (Format == PF_ATI1N)
    return 1;
  //else if (Format == PF_R16F)
  //  return 2;
  else if (Format == PF_A16B16G16R16 || Format == PF_A16B16G16R16F
    || Format == PF_G32R32F)
    return 8;
  else if (Format == PF_A32B32G32R32F)
    return 16;
  else //if (Format == PF_G16R16F || Format == PF_R32F || dxt)
    return 4;
}


static ILubyte iCompFormatToBpc(DDS_CONTEXT *ctx, ILenum Format)
{
  if (ctx->Has16BitComponents)
    return 2;
  if (Format == PF_R16F || Format == PF_G16R16F || Format == PF_A16B16G16R16F)
    //DevIL has no internal half type, so these formats are converted to 32 bits
    return 4;
  else if (Format == PF_R32F || Format == PF_R16F || Format == PF_G32R32F || Format == PF_A32B32G32R32F)
    return 4;
  else if(Format == PF_A16B16G16R16)
    return 2;
  else
    return 1;
}


static ILubyte iCompFormatToChannelCount(ILenum Format)
{
  if (Format == PF_RGB || Format == PF_3DC || Format == PF_RXGB)
    return 3;
  else if (Format == PF_LUMINANCE || /*Format == PF_R16F || Format == PF_R32F ||*/ Format == PF_ATI1N)
    return 1;
  else if (Format == PF_LUMINANCE_ALPHA /*|| Format == PF_G16R16F || Format == PF_G32R32F*/)
    return 2;
  else if (Format == PF_G16R16F || Format == PF_G32R32F || Format == PF_R32F || Format == PF_R16F)
    return 3;
  else //if(Format == PF_ARGB || dxt)
    return 4;
}


static ILboolean iLoadDdsCubemapInternal(DDS_CONTEXT *ctx, ILuint CompFormat)
{
  ILuint  i;
  ILubyte Channels, Bpc;

  ctx->CompData = NULL;

  Channels = iCompFormatToChannelCount(CompFormat);
  Bpc = iCompFormatToBpc(ctx, CompFormat);
  if (CompFormat == PF_LUMINANCE && ctx->Head.RGBBitCount == 16 && ctx->Head.RBitMask == 0xFFFF) { //@TODO: This is a HACK.
    Bpc = 2; 
  }

  // Run through cube map possibilities
  for (i = 0; i < CUBEMAP_SIDES; i++) {
    // Reset each time
    ctx->Width  = ctx->Head.Width;
    ctx->Height = ctx->Head.Height;
    ctx->Depth  = ctx->Head.Depth;
    if (ctx->Head.ddsCaps2 & CubemapDirections[i]) {
      if (i != 0) {
        ctx->Image->Faces = iNewImage(ctx->Width, ctx->Height, ctx->Depth, Channels, Bpc);
        if (ctx->Image->Faces == NULL)
          return IL_FALSE;

        ctx->Image = ctx->Image->Faces;

        if (CompFormat == PF_R16F
          || CompFormat == PF_G16R16F
          || CompFormat == PF_A16B16G16R16F
          || CompFormat == PF_R32F
          || CompFormat == PF_G32R32F
          || CompFormat == PF_A32B32G32R32F) {
          // DevIL's format autodetection doesn't work for
          //  float images...correct this.
          ctx->Image->Type = IL_FLOAT;
          ctx->Image->Bpp = Channels;
        }
      }

      if (!ReadData(ctx))
        return IL_FALSE;

      if (!AllocImage(ctx, CompFormat)) {
        if (ctx->CompData) {
          ifree(ctx->CompData);
          ctx->CompData = NULL;
        }
        return IL_FALSE;
      }

      ctx->Image->CubeFlags = CubemapDirections[i];

      if (!DdsDecompress(ctx, CompFormat)) {
        if (ctx->CompData) {
          ifree(ctx->CompData);
          ctx->CompData = NULL;
        }
        return IL_FALSE;
      }

      if (!ReadMipmaps(ctx, CompFormat)) {
        if (ctx->CompData) {
          ifree(ctx->CompData);
          ctx->CompData = NULL;
        }
        return IL_FALSE;
      }
    }
  }

  if (ctx->CompData) {
    ifree(ctx->CompData);
    ctx->CompData = NULL;
  }

  return IL_TRUE;
}


static ILuint DecodePixelFormat(DDS_CONTEXT *ctx, ILuint *CompFormat)
{
  ILuint BlockSize;

  if (ctx->Head.Flags2 & DDS_FOURCC) {
    BlockSize = ((ctx->Head.Width + 3)/4) * ((ctx->Head.Height + 3)/4) * ctx->Head.Depth;
    switch (ctx->Head.FourCC)
    {
      case IL_MAKEFOURCC('D','X','T','1'):
        *CompFormat = PF_DXT1;
        BlockSize *= 8;
        break;

      case IL_MAKEFOURCC('D','X','T','2'):
        *CompFormat = PF_DXT2;
        BlockSize *= 16;
        break;

      case IL_MAKEFOURCC('D','X','T','3'):
        *CompFormat = PF_DXT3;
        BlockSize *= 16;
        break;

      case IL_MAKEFOURCC('D','X','T','4'):
        *CompFormat = PF_DXT4;
        BlockSize *= 16;
        break;

      case IL_MAKEFOURCC('D','X','T','5'):
        *CompFormat = PF_DXT5;
        BlockSize *= 16;
        break;

      case IL_MAKEFOURCC('A', 'T', 'I', '1'):
        *CompFormat = PF_ATI1N;
        BlockSize *= 8;
        break;

      case IL_MAKEFOURCC('A', 'T', 'I', '2'):
        *CompFormat = PF_3DC;
        BlockSize *= 16;
        break;

      case IL_MAKEFOURCC('R', 'X', 'G', 'B'):
        *CompFormat = PF_RXGB;
        BlockSize *= 16;
        break;

      case IL_MAKEFOURCC('$', '\0', '\0', '\0'):
        *CompFormat = PF_A16B16G16R16;
        BlockSize = ctx->Head.Width * ctx->Head.Height * ctx->Head.Depth * 8;
        break;

      case IL_MAKEFOURCC('o', '\0', '\0', '\0'):
        *CompFormat = PF_R16F;
        BlockSize = ctx->Head.Width * ctx->Head.Height * ctx->Head.Depth * 2;
        break;

      case IL_MAKEFOURCC('p', '\0', '\0', '\0'):
        *CompFormat = PF_G16R16F;
        BlockSize = ctx->Head.Width * ctx->Head.Height * ctx->Head.Depth * 4;
        break;

      case IL_MAKEFOURCC('q', '\0', '\0', '\0'):
        *CompFormat = PF_A16B16G16R16F;
        BlockSize = ctx->Head.Width * ctx->Head.Height * ctx->Head.Depth * 8;
        break;

      case IL_MAKEFOURCC('r', '\0', '\0', '\0'):
        *CompFormat = PF_R32F;
        BlockSize = ctx->Head.Width * ctx->Head.Height * ctx->Head.Depth * 4;
        break;

      case IL_MAKEFOURCC('s', '\0', '\0', '\0'):
        *CompFormat = PF_G32R32F;
        BlockSize = ctx->Head.Width * ctx->Head.Height * ctx->Head.Depth * 8;
        break;

      case IL_MAKEFOURCC('t', '\0', '\0', '\0'):
        *CompFormat = PF_A32B32G32R32F;
        BlockSize = ctx->Head.Width * ctx->Head.Height * ctx->Head.Depth * 16;
        break;

      default:
        *CompFormat = PF_UNKNOWN;
        BlockSize *= 16;
        break;
    }
  } else {
    // This dds texture isn't compressed so write out ARGB or luminance format
    if (ctx->Head.Flags2 & DDS_LUMINANCE) {
      if (ctx->Head.Flags2 & DDS_ALPHAPIXELS) {
        *CompFormat = PF_LUMINANCE_ALPHA;
      } else {
        *CompFormat = PF_LUMINANCE;
      }
    }
    else {
      if (ctx->Head.Flags2 & DDS_ALPHAPIXELS) {
        *CompFormat = PF_ARGB;
      } else {
        *CompFormat = PF_RGB;
      }
    }
    BlockSize = (ctx->Head.Width * ctx->Head.Height * ctx->Head.Depth * (ctx->Head.RGBBitCount >> 3));
  }

  return BlockSize;
}


ILboolean iLoadDdsInternal(ILimage* image)
{
  ILuint  BlockSize = 0;
  ILuint  CompFormat;
  DDS_CONTEXT ctx;

  imemclear(&ctx, sizeof(ctx));

  if (image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  ctx.BaseImage = image;

  if (!iGetDdsHead(&image->io, &ctx.Head)) {
    iSetError(IL_INVALID_FILE_HEADER);
    return IL_FALSE;
  }

  if (!iCheckDds(&ctx.Head)) {
    iSetError(IL_INVALID_FILE_HEADER);
    return IL_FALSE;
  }

  BlockSize = DecodePixelFormat(&ctx, &CompFormat);
  if (CompFormat == PF_UNKNOWN) {
    iSetError(IL_INVALID_FILE_HEADER);
    return IL_FALSE;
  }
  Check16BitComponents(&ctx);

  // Microsoft bug, they're not following their own documentation.
  if (!(ctx.Head.Flags1 & (DDS_LINEARSIZE | DDS_PITCH))
    || ctx.Head.LinearSize == 0) {
    ctx.Head.Flags1 |= DDS_LINEARSIZE;
    ctx.Head.LinearSize = BlockSize;
  }

  ctx.Image = image;
  if (ctx.Head.ddsCaps1 & DDS_COMPLEX) {
    if (ctx.Head.ddsCaps2 & DDS_CUBEMAP) {
      if (!iLoadDdsCubemapInternal(&ctx, CompFormat))
        return IL_FALSE;
      return IL_TRUE;
    }
  }

  ctx.Width = ctx.Head.Width;
  ctx.Height = ctx.Head.Height;
  ctx.Depth = ctx.Head.Depth;
  AdjustVolumeTexture(&ctx, CompFormat);

  if (!ReadData(&ctx))
    return IL_FALSE;

  if (!AllocImage(&ctx, CompFormat)) {
    if (ctx.CompData) {
      ifree(ctx.CompData);
      ctx.CompData = NULL;
    }
    return IL_FALSE;
  }

  if (!DdsDecompress(&ctx, CompFormat)) {
    if (ctx.CompData) {
      ifree(ctx.CompData);
      ctx.CompData = NULL;
    }
    return IL_FALSE;
  }

  if (!ReadMipmaps(&ctx, CompFormat)) {
    if (ctx.CompData) {
      ifree(ctx.CompData);
      ctx.CompData = NULL;
    }
    return IL_FALSE;
  }

  if (ctx.CompData) {
    ifree(ctx.CompData);
    ctx.CompData = NULL;
  }

  return IL_TRUE;
}


// The few volume textures that I have don't have consistent LinearSize
//  entries, even though the DDS_LINEARSIZE flag is set.
void AdjustVolumeTexture(DDS_CONTEXT *ctx, ILuint CompFormat)
{
  if (ctx->Head.Depth <= 1)
    return;

  // All volume textures I've seem so far didn't have the DDS_COMPLEX flag set,
  // even though this is normally required. But because noone does set it,
  // also read images without it (TODO: check file size for 3d texture?)
  if (/*!(ctx->Head.ddsCaps1 & DDS_COMPLEX) ||*/ !(ctx->Head.ddsCaps2 & DDS_VOLUME)) {
    ctx->Head.Depth = 1;
    ctx->Depth = 1;
  }

  switch (CompFormat)
  {
    case PF_ARGB:
    case PF_RGB:
    case PF_LUMINANCE:
    case PF_LUMINANCE_ALPHA:
      //don't use the iCompFormatToBpp() function because this way
      //argb images with only 8 bits (eg. a1r2g3b2) work as well
      ctx->Head.LinearSize = IL_MAX(1,ctx->Head.Width) * IL_MAX(1,ctx->Head.Height) *
        (ctx->Head.RGBBitCount / 8);
      break;
  
    case PF_DXT1:

    case PF_ATI1N:
      ctx->Head.LinearSize = ((ctx->Head.Width+3)/4) * ((ctx->Head.Height+3)/4) * 8;
      break;

    case PF_DXT2:
    case PF_DXT3:
    case PF_DXT4:
    case PF_DXT5:
    case PF_3DC:
    case PF_RXGB:
      ctx->Head.LinearSize = ((ctx->Head.Width+3)/4) * ((ctx->Head.Height+3)/4) * 16;
      break;

    case PF_A16B16G16R16:
    case PF_R16F:
    case PF_G16R16F:
    case PF_A16B16G16R16F:
    case PF_R32F:
    case PF_G32R32F:
    case PF_A32B32G32R32F:
      ctx->Head.LinearSize = IL_MAX(1,ctx->Head.Width) * IL_MAX(1,ctx->Head.Height) *
        iCompFormatToBpp(ctx, CompFormat);
      break;
  }

  ctx->Head.Flags1 |= DDS_LINEARSIZE;
  ctx->Head.LinearSize *= ctx->Head.Depth;

  return;
}


// Reads the compressed data
static ILboolean ReadData(DDS_CONTEXT *ctx)
{
  ILuint  Bps;
  ILuint  y, z;
  ILubyte *Temp;
  SIO * io = &ctx->BaseImage->io;

  if (ctx->CompData) {
    ifree(ctx->CompData);
    ctx->CompData = NULL;
  }

  if (ctx->Head.Flags1 & DDS_LINEARSIZE) {
    iTrace("---- DDS_LINEARSIZE %u", ctx->Head.LinearSize);
    //Head.LinearSize = Head.LinearSize * Depth;

    ctx->CompSize = ctx->Head.LinearSize;
    ctx->CompData = (ILubyte*)ialloc(ctx->Head.LinearSize);
    if (ctx->CompData == NULL) {
      return IL_FALSE;
    }

    if (SIOread(io, ctx->CompData, 1, ctx->Head.LinearSize) != (ILuint)ctx->Head.LinearSize) {
      ifree(ctx->CompData);
      ctx->CompData = NULL;
      return IL_FALSE;
    }
  }
  else {
    Bps = ctx->Width * ctx->Head.RGBBitCount / 8;
    ctx->CompSize = Bps * ctx->Height * ctx->Depth;

    iTrace("---- !DDS_LINEARSIZE %u %u", Bps, ctx->CompSize);

    ctx->CompData = (ILubyte*)ialloc(ctx->CompSize);
    if (ctx->CompData == NULL) {
      return IL_FALSE;
    }

    Temp = ctx->CompData;
    for (z = 0; z < ctx->Depth; z++) {
      for (y = 0; y < ctx->Height; y++) {
        if (SIOread(io, Temp, 1, Bps) != Bps) {
          ifree(ctx->CompData);
          ctx->CompData = NULL;
          return IL_FALSE;
        }
        Temp += Bps;
      }
    }
  }

  return IL_TRUE;
}


ILboolean AllocImage(DDS_CONTEXT *ctx, ILuint CompFormat)
{
  ILubyte channels = 4;
  ILenum format = IL_RGBA;
  ILimage* image = ctx->BaseImage;

  switch (CompFormat)
  {
    case PF_RGB:
      if (!iTexImage(image, ctx->Width, ctx->Height, ctx->Depth, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL))
        return IL_FALSE;
      break;
    case PF_ARGB:
      if (!iTexImage(image, ctx->Width, ctx->Height, ctx->Depth, 4, IL_RGBA, ctx->Has16BitComponents ? IL_UNSIGNED_SHORT : IL_UNSIGNED_BYTE, NULL))
        return IL_FALSE;
      break;

    case PF_LUMINANCE:
      if (ctx->Head.RGBBitCount == 16 && ctx->Head.RBitMask == 0xFFFF) { //HACK
        if (!iTexImage(image, ctx->Width, ctx->Height, ctx->Depth, 1, IL_LUMINANCE, IL_UNSIGNED_SHORT, NULL))
          return IL_FALSE;
      }
      else
        if (!iTexImage(image, ctx->Width, ctx->Height, ctx->Depth, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, NULL))
          return IL_FALSE;
      break;

    case PF_LUMINANCE_ALPHA:
      if (!iTexImage(image, ctx->Width, ctx->Height, ctx->Depth, 2, IL_LUMINANCE_ALPHA, IL_UNSIGNED_BYTE, NULL))
        return IL_FALSE;
      break;

    case PF_ATI1N:
      //right now there's no OpenGL api to use the compressed 3dc data, so
      //throw it away (I don't know how DirectX works, though)?
      if (!iTexImage(image, ctx->Width, ctx->Height, ctx->Depth, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, NULL))
        return IL_FALSE;
      break;

    case PF_3DC:
      //right now there's no OpenGL api to use the compressed 3dc data, so
      //throw it away (I don't know how DirectX works, though)?
      if (!iTexImage(image, ctx->Width, ctx->Height, ctx->Depth, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL))
        return IL_FALSE;
      break;

    case PF_A16B16G16R16:
      if (!iTexImage(image, ctx->Width, ctx->Height, ctx->Depth, iCompFormatToChannelCount(CompFormat),
        iGetFormatBpp(iCompFormatToChannelCount(CompFormat)), IL_UNSIGNED_SHORT, NULL))
        return IL_FALSE;
      break;

    case PF_R16F:
    case PF_G16R16F:
    case PF_A16B16G16R16F:
    case PF_R32F:
    case PF_G32R32F:
    case PF_A32B32G32R32F:
      if (!iTexImage(image, ctx->Width, ctx->Height, ctx->Depth, iCompFormatToChannelCount(CompFormat),
        iGetFormatBpp(iCompFormatToChannelCount(CompFormat)), IL_FLOAT, NULL))
        return IL_FALSE;
      break;

    default:
      if (CompFormat == PF_RXGB) {
        channels = 3; //normal map
        format = IL_RGB;
      }

      if (!iTexImage(image, ctx->Width, ctx->Height, ctx->Depth, channels, format, IL_UNSIGNED_BYTE, NULL))
        return IL_FALSE;
      if (iIsEnabled(IL_KEEP_DXTC_DATA) && ctx->CompData) {
        image->DxtcData = (ILubyte*)ialloc(ctx->Head.LinearSize);
        if (image->DxtcData == NULL)
          return IL_FALSE;
        image->DxtcFormat = CompFormat - PF_DXT1 + IL_DXT1;
        image->DxtcSize = ctx->Head.LinearSize;
        memcpy(image->DxtcData, ctx->CompData, image->DxtcSize);
      }
      break;
  }

  image->Origin = IL_ORIGIN_UPPER_LEFT;

  return IL_TRUE;
}


/*
 * Assumes that the global variable CompFormat stores the format of the
 * global pointer CompData (that's the pointer to the compressed data).
 * Decompresses this data into Image->Data, returns if it was successful.
 * It also uses the globals Width and Height.
 *
 * Assumes that image has valid Width, Height, Depth, Data, SizeOfData,
 * Bpp, Bpc, Bps, SizeOfPlane, Format and Type fields. It is more or
 * less assumed that the image has u8 rgba data (perhaps not for float
 * images...)
 */
ILboolean DdsDecompress(DDS_CONTEXT *ctx, ILuint CompFormat)
{
  switch (CompFormat)
  {
    case PF_ARGB:
    case PF_RGB:
    case PF_LUMINANCE:
    case PF_LUMINANCE_ALPHA:  return DecompressARGB(ctx, CompFormat);
    case PF_DXT1:             return DecompressDXT1(ctx->Image, ctx->CompData, ctx->CompSize);
    case PF_DXT2:             return DecompressDXT2(ctx->Image, ctx->CompData, ctx->CompSize);
    case PF_DXT3:             return DecompressDXT3(ctx->Image, ctx->CompData, ctx->CompSize);
    case PF_DXT4:             return DecompressDXT4(ctx->Image, ctx->CompData, ctx->CompSize);
    case PF_DXT5:             return DecompressDXT5(ctx->Image, ctx->CompData, ctx->CompSize);
    case PF_ATI1N:            return DecompressAti1n(ctx->Image, ctx->CompData, ctx->CompSize);
    case PF_3DC:              return Decompress3Dc(ctx->Image, ctx->CompData, ctx->CompSize);
    case PF_RXGB:             return DecompressRXGB(ctx->Image, ctx->CompData, ctx->CompSize);
    case PF_A16B16G16R16:     memcpy(ctx->Image->Data, ctx->CompData, IL_MIN(ctx->Image->SizeOfData, ctx->CompSize));
                              return IL_TRUE;
    case PF_R16F:
    case PF_G16R16F:
    case PF_A16B16G16R16F:
    case PF_R32F:
    case PF_G32R32F:
    case PF_A32B32G32R32F:    return DecompressFloat(ctx->Image, CompFormat, ctx->CompData, ctx->CompSize);

    case PF_UNKNOWN:          return IL_FALSE;
  }

  return IL_FALSE;
}


ILboolean ReadMipmaps(DDS_CONTEXT *ctx, ILuint CompFormat)
{
  ILuint  i, CompFactor=0;
  ILubyte Bpp, Channels, Bpc;
  ILimage *StartImage, *TempImage;
  ILuint  LastLinear;
  ILuint  minW, minH;

  ILboolean isCompressed = IL_FALSE;

  Bpp       = iCompFormatToBpp(ctx, CompFormat);
  Channels  = iCompFormatToChannelCount(CompFormat);
  Bpc       = iCompFormatToBpc(ctx, CompFormat);
  if (CompFormat == PF_LUMINANCE && ctx->Head.RGBBitCount == 16 && ctx->Head.RBitMask == 0xFFFF) { //HACK
    Bpc = 2; Bpp = 2;
  }

  //This doesn't work for images which first mipmap (= the image
  //itself) has width or height < 4
  //if (Head.Flags1 & DDS_LINEARSIZE) {
  //  CompFactor = (Width * Height * Depth * Bpp) / Head.LinearSize;
  //}
  switch (CompFormat)
  {
    case PF_DXT1:
      // This is officially 6, we have 8 here because DXT1 may contain alpha.
      CompFactor = 8;
      break;
    case PF_DXT2:
    case PF_DXT3:
    case PF_DXT4:
    case PF_DXT5:
      CompFactor = 4;
      break;
    case PF_RXGB:
    case PF_3DC:
      // This is officially 4 for 3dc, but that's bullshit :) There's no
      //  alpha data in 3dc images.
      CompFactor = 3;
      break;

    case PF_ATI1N:
      CompFactor = 2;
      break;
    default:
      CompFactor = 1;
  }

  StartImage = ctx->Image;

  if (!(ctx->Head.Flags1 & DDS_MIPMAPCOUNT) || ctx->Head.MipMapCount == 0) {
    //some .dds-files have their mipmap flag set,
    //but a mipmapcount of 0. Because mipMapCount is an uint, 0 - 1 gives
    //overflow - don't let this happen:
    ctx->Head.MipMapCount = 1;
  }

  LastLinear = ctx->Head.LinearSize;
  for (i = 0; i < ctx->Head.MipMapCount - 1; i++) {
    ctx->Depth = ctx->Depth / 2;
    ctx->Width = ctx->Width / 2;
    ctx->Height = ctx->Height / 2;

    if (ctx->Depth == 0) 
      ctx->Depth = 1;
    if (ctx->Width == 0) 
      ctx->Width = 1;
    if (ctx->Height == 0) 
      ctx->Height = 1;

    ctx->Image->Mipmaps = iNewImage(ctx->Width, ctx->Height, ctx->Depth, Channels, Bpc);
    if (ctx->Image->Mipmaps == NULL)
      goto mip_fail;

    ctx->Image = ctx->Image->Mipmaps;
    ctx->Image->Origin = IL_ORIGIN_UPPER_LEFT;

    if (ctx->Head.Flags1 & DDS_LINEARSIZE) {
      if (CompFormat == PF_R16F
        || CompFormat == PF_G16R16F
        || CompFormat == PF_A16B16G16R16F
        || CompFormat == PF_R32F
        || CompFormat == PF_G32R32F
        || CompFormat == PF_A32B32G32R32F) {
        ctx->Head.LinearSize = ctx->Width * ctx->Height * ctx->Depth * Bpp;

        //DevIL's format autodetection doesn't work for
        //float images...correct this
        ctx->Image->Type = IL_FLOAT;
        ctx->Image->Bpp = Channels;
      }
      else if (CompFormat == PF_A16B16G16R16)
        ctx->Head.LinearSize = ctx->Width * ctx->Height * ctx->Depth * Bpp;
      else if (CompFormat != PF_RGB && CompFormat != PF_ARGB
        && CompFormat != PF_LUMINANCE
        && CompFormat != PF_LUMINANCE_ALPHA) {

        //compressed format
        minW = (((ctx->Width+3)/4))*4;
        minH = (((ctx->Height+3)/4))*4;
        ctx->Head.LinearSize = (minW * minH * ctx->Depth * Bpp) / CompFactor;

        isCompressed = IL_TRUE;
      }
      else {
        //don't use Bpp to support argb images with less than 32 bits
        ctx->Head.LinearSize = ctx->Width * ctx->Height * ctx->Depth * (ctx->Head.RGBBitCount >> 3);
      }
    }
    else {
      ctx->Head.LinearSize >>= 1;
    }

    if (!ReadData(ctx))
      goto mip_fail;

    if (iIsEnabled(IL_KEEP_DXTC_DATA) && isCompressed == IL_TRUE && ctx->CompData) {
      ctx->Image->DxtcData = (ILubyte*)ialloc(ctx->Head.LinearSize);
      if (ctx->Image->DxtcData == NULL)
        return IL_FALSE;
      ctx->Image->DxtcFormat = CompFormat - PF_DXT1 + IL_DXT1;
      ctx->Image->DxtcSize = ctx->Head.LinearSize;
      memcpy(ctx->Image->DxtcData, ctx->CompData, ctx->Image->DxtcSize);
    }

    if (!DdsDecompress(ctx, CompFormat))
      goto mip_fail;
  }

  ctx->Head.LinearSize = LastLinear;
  ctx->Image = StartImage;

  return IL_TRUE;

mip_fail:
  ctx->Image = StartImage;
  StartImage = StartImage->Mipmaps;
  while (StartImage) {
    TempImage = StartImage;
    StartImage = StartImage->Mipmaps;
    ifree(TempImage);
  }

  ctx->Image->Mipmaps = NULL;
  return IL_FALSE;
}


static void DxtcReadColors(const ILubyte* Data, Color8888* Out)
{
  ILubyte r0, g0, b0, r1, g1, b1;

  b0 = (ILubyte)(Data[0] & 0x1F);
  g0 = (ILubyte)(((Data[0] & 0xE0) >> 5) | ((Data[1] & 0x7) << 3));
  r0 = (ILubyte)((Data[1] & 0xF8) >> 3);

  b1 = (ILubyte)(Data[2] & 0x1F);
  g1 = (ILubyte)(((Data[2] & 0xE0) >> 5) | ((Data[3] & 0x7) << 3));
  r1 = (ILubyte)((Data[3] & 0xF8) >> 3);

  Out[0].r = (ILubyte)(r0 << 3 | r0 >> 2);
  Out[0].g = (ILubyte)(g0 << 2 | g0 >> 3);
  Out[0].b = (ILubyte)(b0 << 3 | b0 >> 2);
  Out[0].a = 0xFF;

  Out[1].r = (ILubyte)(r1 << 3 | r1 >> 2);
  Out[1].g = (ILubyte)(g1 << 2 | g1 >> 3);
  Out[1].b = (ILubyte)(b1 << 3 | b1 >> 2);
  Out[1].a = 0xFF;
}

//@TODO: Probably not safe on Big Endian.
void DxtcReadColor(ILushort Data, Color8888* Out)
{
  ILubyte r, g, b;

  b = (ILubyte)(Data & 0x1f);
  g = (ILubyte)((Data & 0x7E0) >> 5);
  r = (ILubyte)((Data & 0xF800) >> 11);

  Out->r = (ILubyte)(r << 3 | r >> 2);
  Out->g = (ILubyte)(g << 2 | g >> 3);
  Out->b = (ILubyte)(b << 3 | r >> 2);
}

ILboolean DecompressDXT1(ILimage *lImage, const void *lCompData, ILuint CompSize)
{
  ILuint    x, y, z, i, j, k, Select;
  Color8888 colours[4], *col;
  ILushort  color_0, color_1;
  ILuint    bitmask, Offset;
  const struct Data {
    ILushort colors[2];
    ILuint   mask;
  } *Temp = lCompData;

  if (!lCompData)
    return IL_FALSE;

  colours[0].a = 0xFF;
  colours[1].a = 0xFF;
  colours[2].a = 0xFF;
  //colours[3].a = 0xFF;
  for (z = 0; z < lImage->Depth; z++) {
    for (y = 0; y < lImage->Height; y += 4) {
      for (x = 0; x < lImage->Width; x += 4) {
        if (!iCheckBuffer(Temp, lCompData, CompSize)) {
          iTrace("**** Unexpected end of compressed data");
          iSetError(IL_INTERNAL_ERROR);
          return IL_FALSE;
        }
        color_0 = Temp->colors[0];
        UShort(&color_0);

        color_1 = Temp->colors[1];
        UShort(&color_1);

        bitmask = Temp->mask;
        UInt(&bitmask);

        DxtcReadColor(color_0, colours);
        DxtcReadColor(color_1, colours + 1);
        Temp ++;

        if (color_0 > color_1) {
          // Four-color block: derive the other two colors.
          // 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
          // These 2-bit codes correspond to the 2-bit fields 
          // stored in the 64-bit block.
          colours[2].b = (2 * colours[0].b + colours[1].b + 1) / 3;
          colours[2].g = (2 * colours[0].g + colours[1].g + 1) / 3;
          colours[2].r = (2 * colours[0].r + colours[1].r + 1) / 3;
          //colours[2].a = 0xFF;

          colours[3].b = (colours[0].b + 2 * colours[1].b + 1) / 3;
          colours[3].g = (colours[0].g + 2 * colours[1].g + 1) / 3;
          colours[3].r = (colours[0].r + 2 * colours[1].r + 1) / 3;
          colours[3].a = 0xFF;
        }
        else { 
          // Three-color block: derive the other color.
          // 00 = color_0,  01 = color_1,  10 = color_2,
          // 11 = transparent.
          // These 2-bit codes correspond to the 2-bit fields 
          // stored in the 64-bit block. 
          colours[2].b = (colours[0].b + colours[1].b) / 2;
          colours[2].g = (colours[0].g + colours[1].g) / 2;
          colours[2].r = (colours[0].r + colours[1].r) / 2;
          //colours[2].a = 0xFF;

          colours[3].b = (colours[0].b + 2 * colours[1].b + 1) / 3;
          colours[3].g = (colours[0].g + 2 * colours[1].g + 1) / 3;
          colours[3].r = (colours[0].r + 2 * colours[1].r + 1) / 3;
          colours[3].a = 0x00;
        }

        for (j = 0, k = 0; j < 4; j++) {
          for (i = 0; i < 4; i++, k++) {
            Select = ((bitmask & (0x03U << k*2)) >> k*2);
            col = &colours[Select];

            if (((x + i) < lImage->Width) && ((y + j) < lImage->Height)) {
              Offset = z * lImage->SizeOfPlane + (y + j) * lImage->Bps + (x + i) * lImage->Bpp;

              if (Offset+3 >= lImage->SizeOfData) {
                iTrace("**** Buffer overrun!");
                iSetError(IL_INTERNAL_ERROR);
                return IL_FALSE;
              }
              lImage->Data[Offset + 0] = col->r;
              lImage->Data[Offset + 1] = col->g;
              lImage->Data[Offset + 2] = col->b;
              lImage->Data[Offset + 3] = col->a;
            }
          }
        }
      }
    }
  }

  return IL_TRUE;
}


ILboolean DecompressDXT2(ILimage *lImage, const void *lCompData, ILuint CompSize)
{
  // Can do color & alpha same as dxt3, but color is pre-multiplied 
  //   so the result will be wrong unless corrected. 
  if (!DecompressDXT3(lImage, lCompData, CompSize))
    return IL_FALSE;
  CorrectPreMult(lImage);

  return IL_TRUE;
}


ILboolean DecompressDXT3(ILimage *lImage, const void *lCompData, ILuint CompSize)
{
  ILuint    x, y, z, i, j, k, Select;
  Color8888 colours[4], *col;
  ILuint    bitmask, Offset;
  ILushort  word;
  const ILubyte   *alpha;
  const DxtcDXT3Data *Data = (const void *)lCompData;

  if (!lCompData)
    return IL_FALSE;

  for (z = 0; z < lImage->Depth; z++) {
    for (y = 0; y < lImage->Height; y += 4) {
      for (x = 0; x < lImage->Width; x += 4) {
        if (!iCheckBuffer(Data, lCompData, CompSize)) {
          iTrace("**** Unexpected end of compressed data");
          iSetError(IL_INTERNAL_ERROR);
          return IL_FALSE;
        }
        alpha = Data->alpha;
        DxtcReadColors(Data->colours, colours);
        bitmask = Data->mask;
        UInt(&bitmask);
        Data ++;

        // Four-color block: derive the other two colors.    
        // 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
        // These 2-bit codes correspond to the 2-bit fields 
        // stored in the 64-bit block.
        colours[2].b = (2 * colours[0].b + colours[1].b + 1) / 3;
        colours[2].g = (2 * colours[0].g + colours[1].g + 1) / 3;
        colours[2].r = (2 * colours[0].r + colours[1].r + 1) / 3;
        //colours[2].a = 0xFF;

        colours[3].b = (colours[0].b + 2 * colours[1].b + 1) / 3;
        colours[3].g = (colours[0].g + 2 * colours[1].g + 1) / 3;
        colours[3].r = (colours[0].r + 2 * colours[1].r + 1) / 3;
        //colours[3].a = 0xFF;

        k = 0;
        for (j = 0; j < 4; j++) {
          for (i = 0; i < 4; i++, k++) {
            Select = (bitmask & (0x03U << k*2)) >> k*2;
            col = &colours[Select];

            if (((x + i) < lImage->Width) && ((y + j) < lImage->Height)) {
              Offset = z * lImage->SizeOfPlane + (y + j) * lImage->Bps + (x + i) * lImage->Bpp;
              if (Offset+2 >= lImage->SizeOfData) {
                iTrace("**** Buffer overrun!");
                iSetError(IL_INTERNAL_ERROR);
                return IL_FALSE;
              }
              lImage->Data[Offset + 0] = col->r;
              lImage->Data[Offset + 1] = col->g;
              lImage->Data[Offset + 2] = col->b;
            }
          }
        }

        for (j = 0; j < 4; j++) {
          word = alpha[2*j] + 256*alpha[2*j+1];
          for (i = 0; i < 4; i++) {
            if (((x + i) < lImage->Width) && ((y + j) < lImage->Height)) {
              Offset = z * lImage->SizeOfPlane + (y + j) * lImage->Bps + (x + i) * lImage->Bpp + 3;
              if (Offset >= lImage->SizeOfData) {
                iTrace("**** Buffer overrun!");
                iSetError(IL_INTERNAL_ERROR);
                return IL_FALSE;
              }
              lImage->Data[Offset] = (ILubyte)(word & 0x0F);
              lImage->Data[Offset] = (ILubyte)(lImage->Data[Offset] | (lImage->Data[Offset] << 4));
            }
            word >>= 4;
          }
        }

      }
    }
  }

  return IL_TRUE;
}


ILboolean DecompressDXT4(ILimage *lImage, const void *lCompData, ILuint CompSize)
{
  // Can do color & alpha same as dxt5, but color is pre-multiplied 
  //   so the result will be wrong unless corrected. 
  if (!DecompressDXT5(lImage, lCompData, CompSize))
    return IL_FALSE;

  CorrectPreMult(lImage);
  return IL_TRUE;
}


ILboolean DecompressDXT5(ILimage *lImage, const void *lCompData, ILuint CompSize)
{
  ILuint    x, y, z, i, j, k, Select;
  Color8888 colours[4], *col;
  ILuint    bitmask, Offset;
  ILubyte   alphas[8];
  const ILubyte *alphamask;
  ILuint    bits;
  const DxtcDXT5Data *Data = lCompData;

  if (!lCompData)
    return IL_FALSE;

  for (z = 0; z < lImage->Depth; z++) {
    for (y = 0; y < lImage->Height; y += 4) {
      for (x = 0; x < lImage->Width; x += 4) {
        if (y >= lImage->Height || x >= lImage->Width)
          break;

        if (!iCheckBuffer(Data, lCompData, CompSize)) {
          iTrace("**** Unexpected end of compressed data");
          iSetError(IL_INTERNAL_ERROR);
          return IL_FALSE;
        }

        alphas[0] = Data->alpha[0];
        alphas[1] = Data->alpha[1];
        alphamask = Data->alphamask;

        DxtcReadColors(Data->colours, colours);
        bitmask = Data->mask;
        UInt(&bitmask);
        Data ++;

        // Four-color block: derive the other two colors.    
        // 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
        // These 2-bit codes correspond to the 2-bit fields 
        // stored in the 64-bit block.
        colours[2].b = (2 * colours[0].b + colours[1].b + 1) / 3;
        colours[2].g = (2 * colours[0].g + colours[1].g + 1) / 3;
        colours[2].r = (2 * colours[0].r + colours[1].r + 1) / 3;
        //colours[2].a = 0xFF;

        colours[3].b = (colours[0].b + 2 * colours[1].b + 1) / 3;
        colours[3].g = (colours[0].g + 2 * colours[1].g + 1) / 3;
        colours[3].r = (colours[0].r + 2 * colours[1].r + 1) / 3;
        //colours[3].a = 0xFF;

        k = 0;
        for (j = 0; j < 4; j++) {
          for (i = 0; i < 4; i++, k++) {

            Select = (bitmask & (0x03U << k*2)) >> k*2;
            col = &colours[Select];

            // only put pixels out < width or height
            if (((x + i) < lImage->Width) && ((y + j) < lImage->Height)) {
              Offset = z * lImage->SizeOfPlane + (y + j) * lImage->Bps + (x + i) * lImage->Bpp;
              if (Offset+2 >= lImage->SizeOfData) {
                iTrace("**** Buffer overrun!");
                iSetError(IL_INTERNAL_ERROR);
                return IL_FALSE;
              }
              lImage->Data[Offset + 0] = col->r;
              lImage->Data[Offset + 1] = col->g;
              lImage->Data[Offset + 2] = col->b;
            }
          }
        }

        // 8-alpha or 6-alpha block?    
        if (alphas[0] > alphas[1]) {    
          // 8-alpha block:  derive the other six alphas.    
          // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
          alphas[2] = (6 * alphas[0] + 1 * alphas[1] + 3) / 7;  // bit code 010
          alphas[3] = (5 * alphas[0] + 2 * alphas[1] + 3) / 7;  // bit code 011
          alphas[4] = (4 * alphas[0] + 3 * alphas[1] + 3) / 7;  // bit code 100
          alphas[5] = (3 * alphas[0] + 4 * alphas[1] + 3) / 7;  // bit code 101
          alphas[6] = (2 * alphas[0] + 5 * alphas[1] + 3) / 7;  // bit code 110
          alphas[7] = (1 * alphas[0] + 6 * alphas[1] + 3) / 7;  // bit code 111
        }
        else {
          // 6-alpha block.
          // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
          alphas[2] = (4 * alphas[0] + 1 * alphas[1] + 2) / 5;  // Bit code 010
          alphas[3] = (3 * alphas[0] + 2 * alphas[1] + 2) / 5;  // Bit code 011
          alphas[4] = (2 * alphas[0] + 3 * alphas[1] + 2) / 5;  // Bit code 100
          alphas[5] = (1 * alphas[0] + 4 * alphas[1] + 2) / 5;  // Bit code 101
          alphas[6] = 0x00;                   // Bit code 110
          alphas[7] = 0xFF;                   // Bit code 111
        }

        // Note: Have to separate the next two loops,
        //  it operates on a 6-byte system.

        // First three bytes
        //bits = *((ILint*)alphamask);
        bits = (ILuint)((alphamask[0]) | (alphamask[1] << 8) | (alphamask[2] << 16));
        for (j = 0; j < 2; j++) {
          for (i = 0; i < 4; i++) {
            // only put pixels out < width or height
            if (((x + i) < lImage->Width) && ((y + j) < lImage->Height)) {
              Offset = z * lImage->SizeOfPlane + (y + j) * lImage->Bps + (x + i) * lImage->Bpp + 3;
              if (Offset >= lImage->SizeOfData) {
                iTrace("**** Buffer overrun!");
                iSetError(IL_INTERNAL_ERROR);
                return IL_FALSE;
              }
              lImage->Data[Offset] = alphas[bits & 0x07];
            }
            bits >>= 3;
          }
        }

        // Last three bytes
        //bits = *((ILint*)&alphamask[3]);
        bits = (ILuint)((alphamask[3]) | (alphamask[4] << 8) | (alphamask[5] << 16));
        for (j = 2; j < 4; j++) {
          for (i = 0; i < 4; i++) {
            // only put pixels out < width or height
            if (((x + i) < lImage->Width) && ((y + j) < lImage->Height)) {
              Offset = z * lImage->SizeOfPlane + (y + j) * lImage->Bps + (x + i) * lImage->Bpp + 3;
              if (Offset >= lImage->SizeOfData) {
                iTrace("**** Buffer overrun!");
                iSetError(IL_INTERNAL_ERROR);
                return IL_FALSE;
              }
              lImage->Data[Offset] = alphas[bits & 0x07];
            }
            bits >>= 3;
          }
        }
      }
    }
  }

  return IL_TRUE;
}


ILboolean Decompress3Dc(ILimage *Image, const void *CompData, ILuint CompSize)
{
  ILuint    x, y, z, i, j, k, t1, t2;
  const ILubyte   *Temp, *Temp2;
  ILubyte   XColours[8], YColours[8];
  ILuint    bitmask, bitmask2, Offset, CurrOffset;

  if (!CompData)
    return IL_FALSE;

  Temp = CompData;
  Offset = 0;
  for (z = 0; z < Image->Depth; z++) {
    for (y = 0; y < Image->Height; y += 4) {
      for (x = 0; x < Image->Width; x += 4) {
        if (!iCheckBuffer(Temp + 8, CompData, CompSize)) {
          iTrace("**** Unexpected end of compressed data");
          iSetError(IL_INTERNAL_ERROR);
          return IL_FALSE;
        }

        Temp2 = Temp + 8;

        //Read Y palette
        t1 = YColours[0] = Temp[0];
        t2 = YColours[1] = Temp[1];
        Temp += 2;

        if (t1 > t2)
          for (i = 2; i < 8; ++i)
            YColours[i] = (ILubyte)( t1 + ((t2 - t1)*(i - 1))/7 );
        else {
          for (i = 2; i < 6; ++i)
            YColours[i] = (ILubyte)( t1 + ((t2 - t1)*(i - 1))/5 );
          YColours[6] = 0;
          YColours[7] = 255;
        }

        // Read X palette
        t1 = XColours[0] = Temp2[0];
        t2 = XColours[1] = Temp2[1];
        Temp2 += 2;
        if (t1 > t2)
          for (i = 2; i < 8; ++i)
            XColours[i] = (ILubyte)( t1 + ((t2 - t1)*(i - 1))/7 );
        else {
          for (i = 2; i < 6; ++i)
            XColours[i] = (ILubyte)( t1 + ((t2 - t1)*(i - 1))/5 );
          XColours[6] = 0;
          XColours[7] = 255;
        }

        //decompress pixel data
        CurrOffset = Offset;
        for (k = 0; k < 4; k += 2) {
          // First three bytes
          bitmask = ((ILuint)(Temp[0]) << 0) | ((ILuint)(Temp[1]) << 8) | ((ILuint)(Temp[2]) << 16);
          bitmask2 = ((ILuint)(Temp2[0]) << 0) | ((ILuint)(Temp2[1]) << 8) | ((ILuint)(Temp2[2]) << 16);
          for (j = 0; j < 2; j++) {
            // only put pixels out < height
            if ((y + k + j) < Image->Height) {
              for (i = 0; i < 4; i++) {
                // only put pixels out < width
                if (((x + i) < Image->Width)) {
                  // ILint tx, ty;

                  t1 = CurrOffset + (x + i)*3;
                  if (t1+2 >= Image->SizeOfData) {
                    iTrace("**** Buffer overrun!");
                    iSetError(IL_INTERNAL_ERROR);
                    return IL_FALSE;
                  }

                  Image->Data[t1 + 1] = /* ty = */ YColours[bitmask & 0x07];
                  Image->Data[t1 + 0] = /* tx = */ XColours[bitmask2 & 0x07];
                  Image->Data[t1 + 2] = 0;

                  /* TODO: make this optional?
                  //calculate b (z) component ((r/255)^2 + (g/255)^2 + (b/255)^2 = 1
                  t = 127*128 - (tx - 127)*(tx - 128) - (ty - 127)*(ty - 128);
                  if (t > 0)
                    ctx->Image->Data[t1 + 2] = (ILubyte)(iSqrt(t) + 128);
                  else
                    ctx->Image->Data[t1 + 2] = 0x7F;
                  */
                }
                bitmask >>= 3;
                bitmask2 >>= 3;
              }
              CurrOffset += Image->Bps;
            }
          }
          Temp += 3;
          Temp2 += 3;
        }

        //skip bytes that were read via Temp2
        Temp += 8;
      }
      Offset += Image->Bps*4;
    }
  }

  return IL_TRUE;
}


ILboolean DecompressAti1n(ILimage *Image, const void *CompData, ILuint CompSize)
{
  ILuint    x, y, z, i, j, k, t1, t2;
  const ILubyte *Temp;
  ILubyte   Colours[8];
  ILuint    bitmask, Offset, CurrOffset;

  if (!CompData)
    return IL_FALSE;

  Temp = CompData;
  Offset = 0;
  for (z = 0; z < Image->Depth; z++) {
    for (y = 0; y < Image->Height; y += 4) {
      for (x = 0; x < Image->Width; x += 4) {
        if (y >= Image->Height || x >= Image->Width)
          break;

        if (!iCheckBuffer(Temp + 1, CompData, CompSize)) {
          iTrace("**** Unexpected end of compressed data");
          iSetError(IL_INTERNAL_ERROR);
          return IL_FALSE;
        }

        //Read palette
        t1 = Colours[0] = Temp[0];
        t2 = Colours[1] = Temp[1];
        Temp += 2;

        if (t1 > t2)
          for (i = 2; i < 8; ++i)
            Colours[i] = (ILubyte)( t1 + ((t2 - t1)*(i - 1))/7 );
        else {
          for (i = 2; i < 6; ++i)
            Colours[i] = (ILubyte)(t1 + ((t2 - t1)*(i - 1))/5 );
          Colours[6] = 0;
          Colours[7] = 255;
        }

        //decompress pixel data
        CurrOffset = Offset;
        for (k = 0; k < 4; k += 2) {
          // First three bytes
          bitmask = ((ILuint)(Temp[0]) << 0) | ((ILuint)(Temp[1]) << 8) | ((ILuint)(Temp[2]) << 16);
          for (j = 0; j < 2; j++) {
            // only put pixels out < height
            if ((y + k + j) < Image->Height) {
              for (i = 0; i < 4; i++) {
                // only put pixels out < width
                if (((x + i) < Image->Width)) {
                  t1 = CurrOffset + (x + i);
                  if (t1 >= Image->SizeOfData) {
                    iTrace("**** Buffer overrun!");
                    iSetError(IL_INTERNAL_ERROR);
                    return IL_FALSE;
                  }
                  Image->Data[t1] = Colours[bitmask & 0x07];
                }
                bitmask >>= 3;
              }
              CurrOffset += Image->Bps;
            }
          }
          Temp += 3;
        }
      }
      Offset += Image->Bps*4;
    }
  }

  return IL_TRUE;
}


//This is nearly exactly the same as DecompressDXT5...
//I have to clean up this file (put common code in
//helper functions etc)
ILboolean DecompressRXGB(ILimage *Image, const void *CompData, ILuint CompSize)
{
  ILuint    x, y, z, i, j, k, Select;
  Color8888 colours[4], *col;
  ILuint    bitmask, Offset;
  ILubyte   alphas[8];
  const ILubyte *alphamask;
  ILuint    bits;
  const DxtcDXT5Data *Data = (const void *)CompData;

  if (!CompData)
    return IL_FALSE;

  imemclear(&colours, sizeof(colours));

  for (z = 0; z < Image->Depth; z++) {
    for (y = 0; y < Image->Height; y += 4) {
      for (x = 0; x < Image->Width; x += 4) {
        if (y >= Image->Height || x >= Image->Width)
          break;

        if (!iCheckBuffer(Data, CompData, CompSize)) {
          iTrace("**** Unexpected end of compressed data");
          iSetError(IL_INTERNAL_ERROR);
          return IL_FALSE;
        }

        alphas[0] = Data->alpha[0];
        alphas[1] = Data->alpha[1];
        alphamask = Data->alphamask;
        DxtcReadColors(Data->colours, colours);
        bitmask   = Data->mask;
        UInt(&bitmask);

        Data        ++;

        // Four-color block: derive the other two colors.    
        // 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
        // These 2-bit codes correspond to the 2-bit fields 
        // stored in the 64-bit block.
        colours[2].b = (2 * colours[0].b + colours[1].b + 1) / 3;
        colours[2].g = (2 * colours[0].g + colours[1].g + 1) / 3;
        colours[2].r = (2 * colours[0].r + colours[1].r + 1) / 3;
        colours[2].a = 0xFF;

        colours[3].b = (colours[0].b + 2 * colours[1].b + 1) / 3;
        colours[3].g = (colours[0].g + 2 * colours[1].g + 1) / 3;
        colours[3].r = (colours[0].r + 2 * colours[1].r + 1) / 3;
        colours[3].a = 0xFF;

        k = 0;
        for (j = 0; j < 4; j++) {
          for (i = 0; i < 4; i++, k++) {

            Select = (bitmask & (0x03U << k*2)) >> k*2;
            col = &colours[Select];

            // only put pixels out < width or height
            if (((x + i) < Image->Width) && ((y + j) < Image->Height)) {
              Offset = z * Image->SizeOfPlane + (y + j) * Image->Bps + (x + i) * Image->Bpp;
              if (Offset+2 >= Image->SizeOfData) {
                iTrace("**** Buffer overrun!");
                iSetError(IL_INTERNAL_ERROR);
                return IL_FALSE;
              }
              Image->Data[Offset + 0] = col->r;
              Image->Data[Offset + 1] = col->g;
              Image->Data[Offset + 2] = col->b;
            }
          }
        }

        // 8-alpha or 6-alpha block?    
        if (alphas[0] > alphas[1]) {    
          // 8-alpha block:  derive the other six alphas.    
          // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
          alphas[2] = (6 * alphas[0] + 1 * alphas[1] + 3) / 7;  // bit code 010
          alphas[3] = (5 * alphas[0] + 2 * alphas[1] + 3) / 7;  // bit code 011
          alphas[4] = (4 * alphas[0] + 3 * alphas[1] + 3) / 7;  // bit code 100
          alphas[5] = (3 * alphas[0] + 4 * alphas[1] + 3) / 7;  // bit code 101
          alphas[6] = (2 * alphas[0] + 5 * alphas[1] + 3) / 7;  // bit code 110
          alphas[7] = (1 * alphas[0] + 6 * alphas[1] + 3) / 7;  // bit code 111
        }
        else {
          // 6-alpha block.
          // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
          alphas[2] = (4 * alphas[0] + 1 * alphas[1] + 2) / 5;  // Bit code 010
          alphas[3] = (3 * alphas[0] + 2 * alphas[1] + 2) / 5;  // Bit code 011
          alphas[4] = (2 * alphas[0] + 3 * alphas[1] + 2) / 5;  // Bit code 100
          alphas[5] = (1 * alphas[0] + 4 * alphas[1] + 2) / 5;  // Bit code 101
          alphas[6] = 0x00;                   // Bit code 110
          alphas[7] = 0xFF;                   // Bit code 111
        }

        // Note: Have to separate the next two loops,
        //  it operates on a 6-byte system.
        // First three bytes
        bits = (ILuint)((alphamask[0]) | (alphamask[1] << 8) | (alphamask[2] << 16));
        for (j = 0; j < 2; j++) {
          for (i = 0; i < 4; i++) {
            // only put pixels out < width or height
            if (((x + i) < Image->Width) && ((y + j) < Image->Height)) {
              Offset = z * Image->SizeOfPlane + (y + j) * Image->Bps + (x + i) * Image->Bpp + 0;
              if (Offset >= Image->SizeOfData) {
                iTrace("**** Buffer overrun!");
                iSetError(IL_INTERNAL_ERROR);
                return IL_FALSE;
              }
              Image->Data[Offset] = alphas[bits & 0x07];
            }
            bits >>= 3;
          }
        }

        // Last three bytes
        bits = (ILuint)((alphamask[3]) | (alphamask[4] << 8) | (alphamask[5] << 16));
        for (j = 2; j < 4; j++) {
          for (i = 0; i < 4; i++) {
            // only put pixels out < width or height
            if (((x + i) < Image->Width) && ((y + j) < Image->Height)) {
              Offset = z * Image->SizeOfPlane + (y + j) * Image->Bps + (x + i) * Image->Bpp + 0;
              if (Offset >= Image->SizeOfData) {
                iTrace("**** Buffer overrun!");
                iSetError(IL_INTERNAL_ERROR);
                return IL_FALSE;
              }
              Image->Data[Offset] = alphas[bits & 0x07];
            }
            bits >>= 3;
          }
        }
      }
    }
  }

  return IL_TRUE;
}


ILboolean DecompressFloat(ILimage *Image, ILuint CompFormat, const void *CompData, ILuint CompSize)
{
  ILuint i, j, Size;

  switch (CompFormat)
  {
    case PF_R32F:  // Red float, green = blue = max
      Size = Image->Width * Image->Height * Image->Depth * 3;
      for (i = 0, j = 0; i < Size; i += 3, j++) {
        if (!iCheckBuffer(((ILfloat*)CompData + j), CompData, CompSize)) {
          iTrace("**** Unexpected end of compressed data");
          iSetError(IL_INTERNAL_ERROR);
          return IL_FALSE;
        }
        if (!iCheckBuffer(((ILfloat*)Image->Data + i + 2), Image->Data, Image->SizeOfData)) {
          iTrace("**** Buffer overrun");
          iSetError(IL_INTERNAL_ERROR);
          return IL_FALSE;
        }
        ((ILfloat*)Image->Data)[i  ] = ((ILfloat*)CompData)[j];
        ((ILfloat*)Image->Data)[i+1] = 1.0f;
        ((ILfloat*)Image->Data)[i+2] = 1.0f;
      }
      return IL_TRUE;

    case PF_A32B32G32R32F:  // Direct copy of float RGBA data
      if (CompSize < Image->SizeOfData) {
        iTrace("**** Unexpected end of compressed data");
        iSetError(IL_INTERNAL_ERROR);
        return IL_FALSE;
      }
      memcpy(Image->Data, CompData, IL_MIN(Image->SizeOfData, CompSize));
      return IL_TRUE;

    case PF_G32R32F:  // Red float, green float, blue = max
      Size = Image->Width * Image->Height * Image->Depth * 3;
      for (i = 0, j = 0; i < Size; i += 3, j += 2) {
        if (!iCheckBuffer(((ILfloat*)CompData + j+1), CompData, CompSize)) {
          iTrace("**** Unexpected end of compressed data");
          iSetError(IL_INTERNAL_ERROR);
          return IL_FALSE;
        }
        if (!iCheckBuffer(((ILfloat*)Image->Data + i + 2), Image->Data, Image->SizeOfData)) {
          iTrace("**** Buffer overrun");
          iSetError(IL_INTERNAL_ERROR);
          return IL_FALSE;
        }
        ((ILfloat*)Image->Data)[i  ] = ((ILfloat*)CompData)[j];
        ((ILfloat*)Image->Data)[i+1] = ((ILfloat*)CompData)[j+1];
        ((ILfloat*)Image->Data)[i+2] = 1.0f;
      }
      return IL_TRUE;

    case PF_R16F:  // Red float, green = blue = max
      // TODO: check buffer sizes
      return iConvR16ToFloat32((ILuint*)Image->Data, (const ILushort*)CompData,
        Image->Width * Image->Height * Image->Depth * Image->Bpp);

    case PF_A16B16G16R16F:  // Just convert from half to float.
      // TODO: check buffer sizes
      return iConvFloat16ToFloat32((ILuint*)Image->Data, (const ILushort*)CompData,
        Image->Width * Image->Height * Image->Depth * Image->Bpp);

    case PF_G16R16F:  // Convert from half to float, set blue = max.
      // TODO: check buffer sizes
      return iConvG16R16ToFloat32((ILuint*)Image->Data, (const ILushort*)CompData,
        Image->Width * Image->Height * Image->Depth * Image->Bpp);

    default:
      return IL_FALSE;
  }
}


void CorrectPreMult(ILimage *Image)
{
  ILuint i;

  for (i = 0; i < Image->SizeOfData; i += 4) {
    if (Image->Data[i+3] != 0) {  // Cannot divide by 0.
      Image->Data[i]   = (ILubyte)(((ILuint)Image->Data[i]   << 8) / Image->Data[i+3]);
      Image->Data[i+1] = (ILubyte)(((ILuint)Image->Data[i+1] << 8) / Image->Data[i+3]);
      Image->Data[i+2] = (ILubyte)(((ILuint)Image->Data[i+2] << 8) / Image->Data[i+3]);
    }
  }

  return;
}


ILboolean DecompressARGB(DDS_CONTEXT *ctx, ILuint CompFormat)
{
  ILuint ReadI = 0, TempBpp, i;
  ILuint RedL, RedR;
  ILuint GreenL, GreenR;
  ILuint BlueL, BlueR;
  ILuint AlphaL, AlphaR;
  ILubyte *Temp;

  ILimage *Image = ctx->Image;

  if (ctx->Has16BitComponents)
    return DecompressARGB16(ctx, CompFormat);

  if (!ctx->CompData)
    return IL_FALSE;

  if (CompFormat == PF_LUMINANCE && ctx->Head.RGBBitCount == 16 && ctx->Head.RBitMask == 0xFFFF) { //HACK
    memcpy(ctx->Image->Data, ctx->CompData, ctx->Image->SizeOfData);
    return IL_TRUE;
  }

  GetBitsFromMask(ctx->Head.RBitMask, &RedL, &RedR);
  GetBitsFromMask(ctx->Head.GBitMask, &GreenL, &GreenR);
  GetBitsFromMask(ctx->Head.BBitMask, &BlueL, &BlueR);
  GetBitsFromMask(ctx->Head.RGBAlphaBitMask, &AlphaL, &AlphaR);
  Temp = ctx->CompData;
  TempBpp = ctx->Head.RGBBitCount / 8;

  for (i = 0; i < Image->SizeOfData; i += Image->Bpp) {

    //@TODO: This is SLOOOW...
    //but the old version crashed in release build under
    //winxp (and xp is right to stop this code - I always
    //wondered that it worked the old way at all)
    if (Image->SizeOfData - i < 4) { //less than 4 byte to write?
      if (TempBpp == 3) //this branch is extra-SLOOOW
        ReadI = (ILuint)(Temp[0] | (Temp[1] << 8) | (Temp[2] << 16));
      else if (TempBpp == 1)
        ReadI = Temp[0];
      else if (TempBpp == 2)
        ReadI = (ILuint)(Temp[0] | (Temp[1] << 8));
    } else
      ReadI = (ILuint)(Temp[0] | (Temp[1] << 8) | (Temp[2] << 16) | (Temp[3] << 24));
    Temp += TempBpp;

    Image->Data[i] = (ILubyte)(((ReadI & ctx->Head.RBitMask) >> RedR) << RedL);

    if (Image->Bpp >= 3) {
      Image->Data[i+1] = (ILubyte)(((ReadI & ctx->Head.GBitMask) >> GreenR) << GreenL);
      Image->Data[i+2] = (ILubyte)(((ReadI & ctx->Head.BBitMask) >> BlueR) << BlueL);

      if (Image->Bpp == 4) {
        Image->Data[i+3] = (ILubyte)(((ReadI & ctx->Head.RGBAlphaBitMask) >> AlphaR) << AlphaL);
        if (AlphaL >= 7) {
          Image->Data[i+3] = Image->Data[i+3] ? 0xFF : 0x00;
        }
        else if (AlphaL >= 4) {
          Image->Data[i+3] = (ILubyte)(Image->Data[i+3] | (Image->Data[i+3] >> 4));
        }
      }
    }
    else if (Image->Bpp == 2) {
      Image->Data[i+1] = (ILubyte)(((ReadI & ctx->Head.RGBAlphaBitMask) >> AlphaR) << AlphaL);
      if (AlphaL >= 7) {
        Image->Data[i+1] = Image->Data[i+1] ? 0xFF : 0x00;
      }
      else if (AlphaL >= 4) {
        Image->Data[i+1] = (ILubyte)(Image->Data[i+1] | (Image->Data[i+3] >> 4));
      }
    }
  }

  return IL_TRUE;
}


// This function simply counts how many contiguous bits are in the mask.
static ILuint CountBitsFromMask(ILuint Mask)
{
  ILuint    i, TestBit = 0x01, Count = 0;
  ILboolean FoundBit = IL_FALSE;

  for (i = 0; i < 32; i++, TestBit <<= 1) {
    if (Mask & TestBit) {
      if (!FoundBit)
        FoundBit = IL_TRUE;
      Count++;
    }
    else if (FoundBit)
      return Count;
  }

  return Count;
}


// Same as DecompressARGB, but it works on images with more than 8 bits
//  per channel, such as a2r10g10b10 and a2b10g10r10.
ILboolean DecompressARGB16(DDS_CONTEXT *ctx, ILuint CompFormat)
{
  ILuint ReadI = 0, TempBpp, i;
  ILuint RedL, RedR;
  ILuint GreenL, GreenR;
  ILuint BlueL, BlueR;
  ILuint AlphaL, AlphaR;
  ILuint RedPad, GreenPad, BluePad, AlphaPad;
  ILubyte *Temp;
  ILushort *UShortImageData;

  (void)CompFormat;

  if (!ctx->CompData)
    return IL_FALSE;

  GetBitsFromMask(ctx->Head.RBitMask, &RedL, &RedR);
  GetBitsFromMask(ctx->Head.GBitMask, &GreenL, &GreenR);
  GetBitsFromMask(ctx->Head.BBitMask, &BlueL, &BlueR);
  GetBitsFromMask(ctx->Head.RGBAlphaBitMask, &AlphaL, &AlphaR);
  RedPad   = 16 - CountBitsFromMask(ctx->Head.RBitMask);
  GreenPad = 16 - CountBitsFromMask(ctx->Head.GBitMask);
  BluePad  = 16 - CountBitsFromMask(ctx->Head.BBitMask);
  AlphaPad = 16 - CountBitsFromMask(ctx->Head.RGBAlphaBitMask);

  RedL = RedL + RedPad;
  GreenL = GreenL + GreenPad;
  BlueL = BlueL + BluePad;
  AlphaL = AlphaL + AlphaPad;

  Temp = ctx->CompData;
  TempBpp = ctx->Head.RGBBitCount / 8;
  UShortImageData = (void*)ctx->Image->Data;

  for (i = 0; i < ctx->Image->SizeOfData / 2; i += ctx->Image->Bpp) {

    //@TODO: This is SLOOOW...
    //but the old version crashed in release build under
    //winxp (and xp is right to stop this code - I always
    //wondered that it worked the old way at all)
    if (ctx->Image->SizeOfData - i < 4) { //less than 4 byte to write?
      if (TempBpp == 3)  //this branch is extra-SLOOOW
        ReadI = (ILuint)(Temp[0] | (Temp[1]<<8) | (Temp[2]<< 16));
      else if (TempBpp == 1)
        ReadI = Temp[0];
      else if (TempBpp == 2)
        ReadI = (ILuint)(Temp[0] | (Temp[1] << 8));
    } else
      ReadI = (ILuint)(Temp[0] | (Temp[1] << 8) | (Temp[2] << 16) | (Temp[3] << 24));
    Temp += TempBpp;

    UShortImageData[i+2] = (ILushort)(((ReadI & ctx->Head.RBitMask) >> RedR) << RedL);

    if (ctx->Image->Bpp >= 3) {
      UShortImageData[i+1] = (ILushort)(((ReadI & ctx->Head.GBitMask) >> GreenR) << GreenL);
      UShortImageData[i  ] = (ILushort)(((ReadI & ctx->Head.BBitMask) >> BlueR ) << BlueL);

      if (ctx->Image->Bpp == 4) {
        UShortImageData[i+3] = (ILushort)(((ReadI & ctx->Head.RGBAlphaBitMask) >> AlphaR) << AlphaL);
        if (AlphaL >= 7) {
          UShortImageData[i+3] = UShortImageData[i+3] ? 0xFF : 0x00;
        }
        else if (AlphaL >= 4) {
          UShortImageData[i+3] = UShortImageData[i+3] | (UShortImageData[i+3] >> 4);
        }
      }
    }
    else if (ctx->Image->Bpp == 2) {
      UShortImageData[i+1] = (ILushort)(((ReadI & ctx->Head.RGBAlphaBitMask) >> AlphaR) << AlphaL);
      if (AlphaL >= 7) {
        UShortImageData[i+1] = UShortImageData[i+1] ? 0xFF : 0x00;
      }
      else if (AlphaL >= 4) {
        UShortImageData[i+1] = UShortImageData[i+1] | (ctx->Image->Data[i+3] >> 4);
      }
    }
  }

  return IL_TRUE;
}


// @TODO:  Look at using the BSF/BSR operands for inline ASM here.
void GetBitsFromMask(ILuint Mask, ILuint *ShiftLeft, ILuint *ShiftRight)
{
  ILuint Temp, i;

  if (Mask == 0) {
    *ShiftLeft = *ShiftRight = 0;
    return;
  }

  Temp = Mask;
  for (i = 0; i < 32; i++, Temp >>= 1) {
    if (Temp & 1)
      break;
  }
  *ShiftRight = i;

  // Temp is preserved, so use it again:
  for (i = 0; i < 8; i++, Temp >>= 1) {
    if (!(Temp & 1))
      break;
  }
  *ShiftLeft = 8 - i;

  return;
}


//
//
// DXT extension code
//
//
/*
static ILubyte* ILAPIENTRY iGetSurfaceDxtcData(ILimage* image)
{
  if (image == NULL) {
    iSetError(IL_INTERNAL_ERROR);
    return NULL;
  }
  return image->DxtcData;
}
*/

static void iFreeSurfaceDxtcData(ILimage* image)
{
  if (image != NULL && image->DxtcData != NULL) {
    ifree(image->DxtcData);
    image->DxtcData = NULL;
    image->DxtcSize = 0;
    image->DxtcFormat = IL_DXT_NO_COMP;
  }
}

/*
static void iFreeImageDxtcData(ILimage* image)
{
  while(image) {
    ILimage *Image = image;
    image = image->Next;

    while(Image) {
      iFreeSurfaceDxtcData(Image);
      Image = Image->Mipmaps;
    }
  }
}
*/

/*
 * This assumes DxtcData, DxtcFormat, width, height, and depth are valid
 */
ILboolean iDxtcDataToSurface(ILimage* image)
{
  ILuint CompFormat;
  DDS_CONTEXT ctx;

  if (image == NULL || image->DxtcData == NULL) {
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }

  if (!(image->DxtcFormat == IL_DXT1 || image->DxtcFormat == IL_DXT2
    || image->DxtcFormat == IL_DXT3
    || image->DxtcFormat == IL_DXT4
    || image->DxtcFormat == IL_DXT5)) {
    iSetError(IL_INVALID_PARAM); //TODO
    return IL_FALSE;
  }

  //@TODO: is this right for all dxt formats? works for
  //  DXT1, 3, 5
  image->Bpp = 4;
  image->Bpc = 1;
  image->Bps = image->Width*image->Bpp*image->Bpc;
  image->SizeOfPlane = image->Height*image->Bps;
  image->Format = IL_RGBA;
  image->Type = IL_UNSIGNED_BYTE;

  if (image->SizeOfData != image->SizeOfPlane*image->Depth) {
    image->SizeOfData = image->Depth*image->SizeOfPlane;
    if (image->Data != NULL)
      ifree(image->Data);
    image->Data = NULL;
  }

  if (image->Data == NULL) {
    image->Data = (ILubyte*) ialloc(image->SizeOfData);
  }

  ctx.Image     = image;
  ctx.BaseImage = image;
  ctx.Width     = image->Width;
  ctx.Height    = image->Height;
  ctx.Depth     = image->Depth;

  switch(image->DxtcFormat)
  {
    case IL_DXT1: CompFormat = PF_DXT1; break;
    case IL_DXT2: CompFormat = PF_DXT2; break;
    case IL_DXT3: CompFormat = PF_DXT3; break;
    case IL_DXT4: CompFormat = PF_DXT4; break;
    case IL_DXT5: CompFormat = PF_DXT5; break;
    default:
      return IL_FALSE;
  }
  ctx.CompData = image->DxtcData;
  ctx.CompSize = image->DxtcSize;

  DdsDecompress(&ctx, CompFormat); //globals suck...fix this // BP: fix'd

  //@TODO: origin should be set in Decompress()...
  image->Origin = IL_ORIGIN_UPPER_LEFT;
  return IL_TRUE;
}


ILboolean iDxtcDataToImage(ILimage* image)
{
  ILboolean ret = IL_TRUE;

  while (image) {
    ILimage *mipMap = image->Mipmaps;
    while(mipMap) {
      if (!iDxtcDataToSurface(mipMap))
        ret = IL_FALSE;
      mipMap = mipMap->Mipmaps;
    }

    image = image->Next;
  }

  return ret;
}


ILboolean iSurfaceToDxtcData(ILimage* image, ILenum Format)
{
  ILuint Size;
  void* Data;
  iFreeSurfaceDxtcData(image);

  Size = ilGetDXTCData(NULL, 0, Format);
  if (Size == 0) {
    return IL_FALSE;
  }

  Data = ialloc(Size);

  if (Data == NULL)
    return IL_FALSE;

  ilGetDXTCData(Data, Size, Format);

  //These have to be after the call to ilGetDXTCData()
  image->DxtcData = (ILubyte*) Data;
  image->DxtcFormat = Format;
  image->DxtcSize = Size;

  return IL_TRUE;
}

ILboolean iImageToDxtcData(ILimage *image, ILenum Format) {
  ILboolean ret = IL_TRUE;

  while (image) {
    ILimage *mipMap = image->Mipmaps;
    while(mipMap) {
      if (!iSurfaceToDxtcData(mipMap, Format))
        ret = IL_FALSE;
      mipMap = mipMap->Mipmaps;
    }
    image = image->Next;
  }

  return ret;
}


//works like iTexImage(), ie. destroys mipmaps etc (which sucks, but
//is consistent. There should be a ilTexSurface() and ilTexSurfaceDxtc()
//functions as well, but for now this is sufficient)
ILboolean iTexImageDxtc(ILimage* image, ILuint w, ILuint h, ILuint d, ILenum DxtFormat, const ILubyte* data)
{
  ILimage* Image = image;

  ILuint xBlocks, yBlocks, BlockSize, LineSize, DataSize;


  //The next few lines are copied from iTexImage() and iInitImage() -
  //should be factored in more reusable functions...
  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  ////

  // Not sure if we should be getting rid of the palette...
  if (Image->Pal.Palette && Image->Pal.PalSize && Image->Pal.PalType != IL_PAL_NONE) {
    ifree(Image->Pal.Palette);
  }

  // These are set NULL later by the memset call.
  iCloseImage(Image->Mipmaps);
  iCloseImage(Image->Next);
  iCloseImage(Image->Faces);
  iCloseImage(Image->Layers);

  if (Image->Profile)  ifree(Image->Profile);
  if (Image->DxtcData) ifree(Image->DxtcData);
  if (Image->Data)   ifree(Image->Data);

  ////

  memset(Image, 0, sizeof(ILimage));
  Image->Width     = w;
  Image->Height    = h;
  Image->Depth     = d;

  //TODO: What about origin with dxtc data?
  Image->Origin    = IL_ORIGIN_LOWER_LEFT;
  Image->Pal.PalType = IL_PAL_NONE;

    // Allocate DXT data buffer
  xBlocks = (w + 3)/4;
  yBlocks = (h + 3)/4;
  if (DxtFormat == IL_DXT1)
    BlockSize = 8;
  else
    BlockSize = 16;

  LineSize = xBlocks * BlockSize;

  DataSize = yBlocks * LineSize * d;

  Image->DxtcFormat  = DxtFormat;
  Image->DxtcSize    = DataSize;
  Image->DxtcData    = (ILubyte*) ialloc(DataSize);

  if (Image->DxtcData == NULL) {
    return IL_FALSE;
  }

  if (data != NULL)
    memcpy(Image->DxtcData, data, DataSize);

  return IL_TRUE;
}

/* ------------------------------------------------------------------- */

static void iFlipColorBlock(ILubyte *data)
{
    ILubyte tmp;

    tmp = data[4];
    data[4] = data[7];
    data[7] = tmp;

    tmp = data[5];
    data[5] = data[6];
    data[6] = tmp;
}

static void iFlipSimpleAlphaBlock(ILushort *data)
{
  ILushort tmp;

  tmp = data[0];
  data[0] = data[3];
  data[3] = tmp;

  tmp = data[1];
  data[1] = data[2];
  data[2] = tmp;
}

static void iComplexAlphaHelper(ILubyte* Data)
{
  ILushort tmp[2];

  //one 4 pixel line is 12 bit, copy each line into
  //a ushort, swap them and copy back
  tmp[0] = (Data[0] | (Data[1] << 8)) & 0xfff;
  tmp[1] = ((Data[1] >> 4) | (Data[2] << 4)) & 0xfff;

  Data[0] = (ILubyte)( tmp[1] );
  Data[1] = (ILubyte)((tmp[1] >> 8) | (tmp[0] << 4));
  Data[2] = (ILubyte)( tmp[0] >> 4);
}

static void iFlipComplexAlphaBlock(ILubyte *Data)
{
  ILubyte tmp[3];
  Data += 2; //Skip 'palette'

  //swap upper two rows with lower two rows
  memcpy(tmp, Data, 3);
  memcpy(Data, Data + 3, 3);
  memcpy(Data + 3, tmp, 3);

  //swap 1st with 2nd row, 3rd with 4th
  iComplexAlphaHelper(Data);
  iComplexAlphaHelper(Data + 3);
}

static void iFlipDxt1(ILubyte* data, ILuint count)
{
  ILuint i;

  for (i = 0; i < count; ++i) {
    iFlipColorBlock(data);
    data += 8; //advance to next block
  }
}

static void iFlipDxt3(ILubyte* data, ILuint count)
{
  ILuint i;
  for (i = 0; i < count; ++i) {
    iFlipSimpleAlphaBlock((ILushort*)(void*)data);
    iFlipColorBlock(data + 8);
    data += 16; //advance to next block
  }
}

static void iFlipDxt5(ILubyte* data, ILuint count)
{
  ILuint i;
  for (i = 0; i < count; ++i) {
    iFlipComplexAlphaBlock(data);
    iFlipColorBlock(data + 8);
    data += 16; //advance to next block
  }
}

static void iFlip3dc(ILubyte* data, ILuint count)
{
  ILuint i;
  for (i = 0; i < count; ++i) {
    iFlipComplexAlphaBlock(data);
    iFlipComplexAlphaBlock(data + 8);
    data += 16; //advance to next block
  }
}


void iFlipSurfaceDxtcData(ILimage* image) {
  ILuint y, z;
  ILuint BlockSize, LineSize;
  ILubyte *Temp, *Runner, *Top, *Bottom;
  ILuint numXBlocks, numYBlocks;
  void (*FlipBlocks)(ILubyte* data, ILuint count);

  if (image == NULL || image->DxtcData == NULL) {
    iSetError(IL_INVALID_PARAM);
    return;
  }

  numXBlocks = (image->Width + 3)/4;
  numYBlocks = (image->Height + 3)/4;
  
  switch (image->DxtcFormat)
  {
    case IL_DXT1:
      BlockSize = 8;
      FlipBlocks = iFlipDxt1;
      break;
    case IL_DXT2:
    case IL_DXT3:
      BlockSize = 16;
      FlipBlocks = iFlipDxt3;
      break;
    case IL_DXT4:
    case IL_DXT5:
    case IL_RXGB:
      BlockSize = 16;
      FlipBlocks = iFlipDxt5;
      break;
    case IL_3DC:
      BlockSize = 16;
      FlipBlocks = iFlip3dc;
      break;
    default:
      iSetError(IL_INVALID_PARAM);
      return;
  }
  
  LineSize = numXBlocks * BlockSize;
  Temp = (ILubyte*) ialloc(LineSize);

  if (Temp == NULL)
      return;

  Runner = image->DxtcData;
  for (z = 0; z < image->Depth; ++z) {
    Top = Runner;
    Bottom = Runner + (numYBlocks - 1)*LineSize;
      
    for (y = 0; y < numYBlocks/2; ++y) {
      //swap block row
      memcpy(Temp, Top, LineSize);
      memcpy(Top, Bottom, LineSize);
      memcpy(Bottom, Temp, LineSize);


      //swap blocks
      FlipBlocks(Top, numXBlocks);
      FlipBlocks(Bottom, numXBlocks);

      Top += LineSize;
      Bottom -= LineSize;
    }

    //middle line
    if (numYBlocks % 2 != 0)
      FlipBlocks(Top, numXBlocks);

    Runner += LineSize * numYBlocks;
  }

  ifree(Temp);
}

/**********************************************************************/

static void iInvertDxt3Alpha(ILubyte *data)
{
  ILint i;

  for (i = 0; i < 8; ++i) {
    /*
    ILubyte b, t1, t2;
    b = data[i];

    t1 = b & 0xf;
    t1 = 15 - t1;
    t2 = b >> 4;
    t2 = 15 - t2;

    data[i] = (t2 << 4) | t1;
    */
    //simpler:
    data[i] = ~data[i];
  }
}

static void iInvertDxt5Alpha(ILubyte *data)
{
  ILubyte a0, a1;
  ILint i, j;
  const ILubyte map1[] = { 1, 0, 7, 6, 5, 4, 3, 2 };
  const ILubyte map2[] = { 1, 0, 5, 4, 3, 2, 7, 6 };


  a0 = data[0];
  a1 = data[1];

  //a0 > a1 <=> 255 - a0 < 255 - a1. Because of this,
  //a1 and a2 have to be swapped, and the indices
  //have to be changed as well.

  //invert and swap alpha
  data[0] = 255 - a1;
  data[1] = 255 - a0;
  data += 2;

  //fix indices
  for (i = 0; i < 6; i += 3) {
    ILuint in  = (ILuint)(data[i] | (data[i+1] << 8) | (data[i+2] << 16));
    ILuint out = 0;

    for (j = 0; j < 24; j += 3) {
      ILubyte b = (in >> j) & 0x7;

      if (a0 > a1)
        b = map1[b];
      else
        b = map2[b];

      out |= (ILuint)(b << j);
    }

    data[i  ] = (ILubyte)(out);
    data[i+1] = (ILubyte)(out >> 8);
    data[i+2] = (ILubyte)(out >> 16);
  }
}


ILboolean iInvertSurfaceDxtcDataAlpha(ILimage* image)
{
  ILuint i;
  ILuint BlockSize;
  ILubyte *Runner;
  ILuint numXBlocks, numYBlocks, numBlocks;
  void (*InvertAlpha)(ILubyte* data);

  if (image == NULL || image->DxtcData == NULL) {
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }

  numXBlocks = (image->Width + 3)/4;
  numYBlocks = (image->Height + 3)/4;
  numBlocks = numXBlocks*numYBlocks*image->Depth;
  BlockSize = 16;

  switch (image->DxtcFormat)
  {
    case IL_DXT3:
      InvertAlpha = iInvertDxt3Alpha;
      break;
    case IL_DXT5:
      InvertAlpha = iInvertDxt5Alpha;
      break;
    default:
      //DXT2/4 are not supported yet because nobody
      //uses them anyway and I would have to change
      //the color blocks as well...
      //DXT1 is not supported because DXT1 alpha is
      //seldom used and it's not easily invertable.
      iSetError(IL_INVALID_PARAM);
      return IL_FALSE;
  }

  Runner = image->DxtcData;
  for (i = 0; i < numBlocks; ++i, Runner += BlockSize) {
    InvertAlpha(Runner);
  }

  return IL_TRUE;
}

static ILconst_string iFormatExtsDDS[] = { 
  IL_TEXT("dds"), 
  NULL 
};

ILformat iFormatDDS = { 
  /* .Validate = */ iIsValidDds, 
  /* .Load     = */ iLoadDdsInternal, 
  /* .Save     = */ iSaveDdsInternal, 
  /* .Exts     = */ iFormatExtsDDS
};

#endif//IL_NO_DDS
