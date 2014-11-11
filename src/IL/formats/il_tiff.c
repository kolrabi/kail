//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_tiff.c
//
// Description: Tiff (.tif) functions
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_TIF

#include "tiffio.h"

#include <time.h>
#include "il_manip.h"

#define MAGIC_HEADER1 0x4949
#define MAGIC_HEADER2 0x4D4D

ILuint iGetMetaiv(ILimage *Image, ILenum MetaID, ILint *Param);
void * iGetMetax(ILimage *Image, ILenum MetaID, ILuint *Size);

#define SetFieldWORD(ID, TID) \
  if (iGetMetaiv(BaseImage, ID, NULL) == 1) { \
    iGetMetaiv(BaseImage, ID, TempInts); \
    TIFFSetField(File, TID, (ILushort)TempInts[0]);\
  } 

#define SetFieldDWORD2(ID, TID) \
  if (iGetMetaiv(BaseImage, ID, NULL) == 2) { \
    iGetMetaiv(BaseImage, ID, TempInts); \
    TIFFSetField(File, TID, (ILuint)TempInts[0], (ILuint)TempInts[1]);\
  } 

#define SetFieldRational(ID, TID) \
  if (iGetMetaiv(BaseImage, ID, NULL) == 2) { \
    ILdouble v; \
    iGetMetaiv(BaseImage, ID, TempInts); \
    v = (double)TempInts[0] / (double)TempInts[1]; \
    TIFFSetField(File, TID, v);\
  } 

#define SetFieldString(ID, TID) \
  { \
    char *str = iGetString(image, ID); \
    if (str) { TIFFSetField(File, TID, str); ifree(str); } \
  }

#if (defined(_WIN32) || defined(_WIN64)) && defined(IL_USE_PRAGMA_LIBS)
  #if defined(_MSC_VER) || defined(__BORLANDC__)
    #ifndef _DEBUG
      #pragma comment(lib, "libtiff.lib")
    #else
      #pragma comment(lib, "libtiff-d.lib")
    #endif
  #endif
#endif


/*----------------------------------------------------------------------------*/

// No need for a separate header
static char*     iMakeString(void);
static TIFF*     iTIFFOpen(SIO *io, char *Mode);

ILboolean iExifLoad(ILimage *Image);

/*----------------------------------------------------------------------------*/

static ILboolean iIsValidTiff(SIO* io) {
  ILushort  Header1 = 0, Header2 = 0;
  ILuint    Start = SIOtell(io);
  ILboolean   bRet = IL_TRUE;

  Header1 = GetLittleUShort(io);

  if (Header1 == MAGIC_HEADER1) {
    Header2 = GetLittleUShort(io);
  } else if (Header1 == MAGIC_HEADER2) {
    Header2 = GetBigUShort(io);
  } else {
    bRet = IL_FALSE;
  }

  if (Header2 != 42)
    bRet = IL_FALSE;

  SIOseek(io, Start, IL_SEEK_SET);

  return bRet;
}

/*----------------------------------------------------------------------------*/
#ifdef __clang__
#pragma clang diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wformat-nonliteral"

static void warningHandler(const char* mod, const char* fmt, va_list ap)
{
  (void)mod;
  iTraceV(fmt, ap);
}

static void errorHandler(const char* mod, const char* fmt, va_list ap)
{
  (void)mod;
  iTraceV(fmt, ap);
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

////


// Internal function used to load the Tiff.
static ILboolean iLoadTiffInternal(ILimage* image) {
  TIFF   *tif;
  uint16   photometric, planarconfig, orientation;
  uint16   samplesperpixel, bitspersample, *sampleinfo, extrasamples;
  uint32  w, h, d, tilewidth, tilelength;
  // ILubyte  *pImageData;
  ILuint   i, ProfileLen, DirCount = 0;
  void   *Buffer;
  ILimage  *Image, *TempImage;
  ILushort si;
  ILfloat  x_position, x_resolution, y_position, y_resolution;
  SIO *    io;
  ILuint   Start, End;
  
  // to avoid high order bits garbage when used as shorts
  w = h = d = tilewidth = tilelength = 0;

  if (image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  io = &image->io;
  Start = SIOtell(io);

  TIFFSetWarningHandler(warningHandler);
  TIFFSetErrorHandler(errorHandler);

  tif = iTIFFOpen(io, "r");
  if (tif == NULL) {
    iSetError(IL_COULD_NOT_OPEN_FILE);
    return IL_FALSE;
  }

  do {
    DirCount++;
  } while (TIFFReadDirectory(tif));

  Image = NULL;
  for (i = 0; i < DirCount; i++) {
    tmsize_t linesize;

    TIFFSetDirectory(tif, (tdir_t)i);
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH,  &w);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);

    TIFFGetFieldDefaulted(tif, TIFFTAG_IMAGEDEPTH,    &d); //TODO: d is ignored...
    TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
    TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);
    TIFFGetFieldDefaulted(tif, TIFFTAG_EXTRASAMPLES,  &extrasamples, &sampleinfo);
    TIFFGetFieldDefaulted(tif, TIFFTAG_ORIENTATION,   &orientation);

    linesize = TIFFScanlineSize(tif);
    
    //added 2003-08-31
    //1 bpp tiffs are not neccessarily greyscale, they can
    //have a palette (photometric == 3)...get this information
    TIFFGetFieldDefaulted(tif, TIFFTAG_PHOTOMETRIC,  &photometric);
    TIFFGetFieldDefaulted(tif, TIFFTAG_PLANARCONFIG, &planarconfig);

    //special-case code for frequent data cases that may be read more
    //efficiently than with the TIFFReadRGBAImage() interface.
    
    //added 2004-05-12
    //Get tile sizes and use TIFFReadRGBAImage() for tiled images for now
    tilewidth = w; tilelength = h;
    TIFFGetFieldDefaulted(tif, TIFFTAG_TILEWIDTH,  &tilewidth);
    TIFFGetFieldDefaulted(tif, TIFFTAG_TILELENGTH, &tilelength);


    if (extrasamples == 0
      && samplesperpixel == 1  //luminance or palette
      && (bitspersample == 8 || bitspersample == 1 || bitspersample == 16)
      && (photometric == PHOTOMETRIC_MINISWHITE
        || photometric == PHOTOMETRIC_MINISBLACK
        || photometric == PHOTOMETRIC_PALETTE)
      && (orientation == ORIENTATION_TOPLEFT || orientation == ORIENTATION_BOTLEFT)
      && tilewidth == w && tilelength == h
      ) {
      ILubyte* strip;
      tmsize_t stripsize, j;
      uint32 y, rowsperstrip, linesread;

      //TODO: 1 bit/pixel images should not be stored as 8 bits...
      //(-> add new format)
      if (!Image) {
        ILenum type = IL_UNSIGNED_BYTE;
        if (bitspersample == 16) type = IL_UNSIGNED_SHORT;
        if (!iTexImage(image, (ILuint)w, (ILuint)h, 1, 1, IL_LUMINANCE, type, NULL)) {
          TIFFClose(tif);
          return IL_FALSE;
        }
        Image = image;
      }
      else {
        Image->Next = iNewImage((ILuint)w, (ILuint)h, 1, 1, 1);
        if (Image->Next == NULL) {
          TIFFClose(tif);
          return IL_FALSE;
        }
        Image = Image->Next;
      }

      if (photometric == PHOTOMETRIC_PALETTE) { //read palette
        uint16 *red, *green, *blue;
        //ILboolean is16bitpalette = IL_FALSE;
        ILubyte *entry;
        tmsize_t count = 1L << bitspersample;
    
        TIFFGetField(tif, TIFFTAG_COLORMAP, &red, &green, &blue);

        Image->Format = IL_COLOUR_INDEX;
        Image->Pal.PalSize = (ILuint)(count)*3;
        Image->Pal.PalType = IL_PAL_RGB24;
        Image->Pal.Palette = (ILubyte*)ialloc(Image->Pal.PalSize);
        entry = Image->Pal.Palette;
        for (j = 0; j < count; ++j) {
          entry[0] = (ILubyte)(red[j] >> 8);
          entry[1] = (ILubyte)(green[j] >> 8);
          entry[2] = (ILubyte)(blue[j] >> 8);

          entry += 3;
        }
      }

      TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
      stripsize = TIFFStripSize(tif);

      strip = (ILubyte*)ialloc((ILuint)stripsize);

      if (bitspersample == 8 || bitspersample == 16) {
        ILubyte *dat = Image->Data;
        for (y = 0; y < h; y += rowsperstrip) {
          //the last strip may contain less data if the image
          //height is not evenly divisible by rowsperstrip
          if (y + rowsperstrip > h) {
            stripsize = linesize*(tmsize_t)(h - y);
            linesread = h - y;
          }
          else
            linesread = rowsperstrip;

          if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, y, 0), strip, stripsize) == -1) {
            iSetError(IL_LIB_TIFF_ERROR);
            ifree(strip);
            TIFFClose(tif);
            return IL_FALSE;
          }

          if (photometric == PHOTOMETRIC_MINISWHITE) { //invert channel
            tmsize_t k;
            for (j = 0; (uint32)j < linesread; ++j) {
              tmsize_t t2 = j*linesize;
              //this works for 16bit images as well: the two bytes
              //making up a pixel can be inverted independently
              for (k = 0; (ILuint)k < Image->Bps; ++k)
                dat[k] = ~strip[t2 + k];
              dat += w;
            }
          }
          else {
            uint32 k;
            for(k = 0; k < linesread; ++k)
              memcpy(&Image->Data[(y + k)*Image->Bps], &strip[k*(uint32)linesize], Image->Bps);
          }
        }
      }
      else if (bitspersample == 1) {
        //TODO: add a native format to devil, so we don't have to
        //unpack the values here
        ILubyte mask, curr, *dat = Image->Data;
        uint32 k;
        tmsize_t jj, t2, sx;
        for (y = 0; y < h; y += rowsperstrip) {
          //the last strip may contain less data if the image
          //height is not evenly divisible by rowsperstrip
          if (y + rowsperstrip > h) {
            stripsize = linesize*(tmsize_t)(h - y);
            linesread = h - y;
          }
          else
            linesread = rowsperstrip;

          if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, y, 0), strip, stripsize) == -1) {
            iSetError(IL_LIB_TIFF_ERROR);
            ifree(strip);
            TIFFClose(tif);
            return IL_FALSE;
          }

          for (jj = 0; (uint32)jj < linesread; ++jj) {
            k = 0;
            sx = 0;
            t2 = jj*linesize;
            while (k < w) {
              curr = strip[t2 + sx];
              if (photometric == PHOTOMETRIC_MINISWHITE)
                curr = ~curr;
              for (mask = 0x80; mask != 0 && k < w; mask >>= 1){
                if((curr & mask) != 0)
                  dat[k] = 255;
                else
                  dat[k] = 0;
                ++k;
              }
              ++sx;
            }
            dat += w;
          }
        }
      }

      ifree(strip);

      if(orientation == ORIENTATION_TOPLEFT)
        Image->Origin = IL_ORIGIN_UPPER_LEFT;
      else if(orientation == ORIENTATION_BOTLEFT)
        Image->Origin = IL_ORIGIN_LOWER_LEFT;
    }
    //for 16bit rgb images:
    else if (extrasamples == 0
      && samplesperpixel == 3
      && (bitspersample == 8 || bitspersample == 16)
      && photometric == PHOTOMETRIC_RGB
      && planarconfig == 1
      && (orientation == ORIENTATION_TOPLEFT || orientation == ORIENTATION_BOTLEFT)
      && tilewidth == w && tilelength == h
      ) {
      ILubyte *strip; //, *dat;
      tsize_t stripsize;
      ILuint y;
      uint32 rowsperstrip, j, linesread;

      if (!Image) {
        ILenum type = IL_UNSIGNED_BYTE;
        if (bitspersample == 16) type = IL_UNSIGNED_SHORT;
        if(!iTexImage(image, (ILuint)w, (ILuint)h, 1, 3, IL_RGB, type, NULL)) {
          TIFFClose(tif);
          return IL_FALSE;
        }
        Image = image;
      }
      else {
        Image->Next = iNewImage((ILuint)w, (ILuint)h, 1, 1, 1);
        if(Image->Next == NULL) {
          TIFFClose(tif);
          return IL_FALSE;
        }
        Image = Image->Next;
      }

      TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
      stripsize = TIFFStripSize(tif);

      strip = (ILubyte*)ialloc((ILuint)stripsize);

      // dat = Image->Data;
      for (y = 0; y < h; y += rowsperstrip) {
        //the last strip may contain less data if the image
        //height is not evenly divisible by rowsperstrip
        if (y + rowsperstrip > h) {
          stripsize = linesize*(tmsize_t)(h - y);
          linesread = h - y;
        }
        else
          linesread = rowsperstrip;

        if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, y, 0), strip, stripsize) == -1) {
          iSetError(IL_LIB_TIFF_ERROR);
          ifree(strip);
          TIFFClose(tif);
          return IL_FALSE;
        }

        for(j = 0; j < linesread; ++j)
            memcpy(&Image->Data[(y + j)*Image->Bps], &strip[(tmsize_t)j*linesize], Image->Bps);
      }

      ifree(strip);
      
      if(orientation == ORIENTATION_TOPLEFT)
        Image->Origin = IL_ORIGIN_UPPER_LEFT;
      else if(orientation == ORIENTATION_BOTLEFT)
        Image->Origin = IL_ORIGIN_LOWER_LEFT;
    }/*
    else if (extrasamples == 0 && samplesperpixel == 3  //rgb
          && (bitspersample == 8 || bitspersample == 1 || bitspersample == 16)
          && photometric == PHOTOMETRIC_RGB
          && (planarconfig == PLANARCONFIG_CONTIG || planarcon
            && (orientation == ORIENTATION_TOPLEFT || orientation == ORIENTATION_BOTLEFT)
          ) {
    }
        */
    else {
        //not direclty supported format
      if(!Image) {
        if(!iTexImage(image, w, h, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL)) {
          TIFFClose(tif);
          return IL_FALSE;
        }
        Image = image;
      }
      else {
        Image->Next = iNewImage(w, h, 1, 4, 1);
        if(Image->Next == NULL) {
          TIFFClose(tif);
          return IL_FALSE;
        }
        Image = Image->Next;
      }

      if (samplesperpixel == 4) {
        TIFFGetFieldDefaulted(tif, TIFFTAG_EXTRASAMPLES, &extrasamples, &sampleinfo);
        if (!sampleinfo || sampleinfo[0] == EXTRASAMPLE_UNSPECIFIED) {
          si = EXTRASAMPLE_ASSOCALPHA;
          TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, 1, &si);
        }
      }
      /*
       if (!iResizeImage(Image, Image->Width, Image->Height, 1, 4, 1)) {
         TIFFClose(tif);
         return IL_FALSE;
       }
       */
      Image->Format = IL_RGBA;
      Image->Type = IL_UNSIGNED_BYTE;
  
      // Siigron: added u_long cast to shut up compiler warning
      //2003-08-31: changed flag from 1 (exit on error) to 0 (keep decoding)
      //this lets me view text.tif, but can give crashes with unsupported
      //tiffs...
      //2003-09-04: keep flag 1 for official version for now
      if (!TIFFReadRGBAImage(tif, Image->Width, Image->Height, (uint32*)(void*)Image->Data, 0)) {
        TIFFClose(tif);
        iSetError(IL_LIB_TIFF_ERROR);
        return IL_FALSE;
      }
      Image->Origin = IL_ORIGIN_LOWER_LEFT;  // eiu...dunno if this is right

#ifdef WORDS_BIGENDIAN //TIFFReadRGBAImage reads abgr on big endian, convert to rgba
      EndianSwapData(Image);
#endif

      /*
       The following switch() should not be needed,
       because we take care of the special cases that needed
       these conversions. But since not all special cases
       are handled right now, keep it :)
       */
      //TODO: put switch into the loop??
      TempImage = image;
      image = Image; //ilConvertImage() converts image
      switch (samplesperpixel)
      {
        case 1:
          //added 2003-08-31 to keep palettized tiffs colored
          if(photometric != 3)
            iConvertImages(Image, IL_LUMINANCE, IL_UNSIGNED_BYTE);
          else //strip alpha as tiff supports no alpha palettes
            iConvertImages(Image, IL_RGB, IL_UNSIGNED_BYTE);
          break;
          
        case 3:
          //TODO: why the ifdef??
#ifdef WORDS_LITTLEENDIAN
          iConvertImages(Image, IL_RGB, IL_UNSIGNED_BYTE);
#endif      
          break; 
          
        case 4:
          // pImageData = Image->Data;
          //removed on 2003-08-26...why was this here? libtiff should and does
          //take care of these things???
          /*      
          //invert alpha
#ifdef WORDS_LITTLEENDIAN
          pImageData += 3;
#endif      
          for (i = Image->Width * Image->Height; i > 0; i--) {
            *pImageData ^= 255;
            pImageData += 4;
          }
          */
          break;
      }
      image = TempImage;
      
    } //else not directly supported format

    if (TIFFGetField(tif, TIFFTAG_ICCPROFILE, &ProfileLen, &Buffer)) {
      if (Image->Profile && Image->ProfileSize)
        ifree(Image->Profile);
      Image->Profile = (ILubyte*)ialloc(ProfileLen);
      if (Image->Profile == NULL) {
        TIFFClose(tif);
        return IL_FALSE;
      }

      memcpy(Image->Profile, Buffer, ProfileLen);
      Image->ProfileSize = ProfileLen;

      //removed on 2003-08-24 as explained in bug 579574 on sourceforge
      //_TIFFfree(Buffer);
    }

                // dangelo: read offset from tiff tags.
                //If nothing is stored in these tags, then this must be an "uncropped" TIFF 
                //file, in which case, the "full size" width/height is the same as the 
                //"cropped" width and height
                //
                // the full size is currently not supported by DevIL
                //if (TIFFGetField(tif, TIFFTAG_PIXAR_IMAGEFULLWIDTH, &(c->full_width)) ==
                //        0)
                //    (c->full_width = c->cropped_width);
                //if (TIFFGetField(tif, TIFFTAG_PIXAR_IMAGEFULLLENGTH, &(c->full_height)) ==
                //        0)
                //    (c->full_height = c->cropped_height);

                if (TIFFGetField(tif, TIFFTAG_XPOSITION, &x_position) == 0)
                    x_position = 0;
                if (TIFFGetField(tif, TIFFTAG_XRESOLUTION, &x_resolution) == 0)
                    x_resolution = 0;
                if (TIFFGetField(tif, TIFFTAG_YPOSITION, &y_position) == 0)
                    y_position = 0;
                if (TIFFGetField(tif, TIFFTAG_YRESOLUTION, &y_resolution) == 0)
                    y_resolution = 0;

                //offset in pixels of "cropped" image from top left corner of 
                //full image (rounded to nearest integer)
                Image->OffX = (uint32) ((x_position * x_resolution) + 0.49);
                Image->OffY = (uint32) ((y_position * y_resolution) + 0.49);
                

    /*
     Image = Image->Next;
     if (Image == NULL)  // Should never happen except when we reach the end, but check anyway.
     break;
     */ 
  } //for tiff directories

  TIFFClose(tif);

  End = SIOtell(io);
  SIOseek(io, Start, IL_SEEK_SET);
  iExifLoad(Image);
  SIOseek(io, End, IL_SEEK_SET);

  return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

/////////////////////////////////////////////////////////////////////////////////////////
// Extension to load tiff files from memory
// Marco Fabbricatore (fabbrica@ai-lab.fh-furtwangen.de)
/////////////////////////////////////////////////////////////////////////////////////////

static tsize_t 
_tiffFileReadProc(thandle_t fd, tdata_t pData, tsize_t tSize)
{
  //iTrace("---- reading %d @%08x", (ILuint)tSize, SIOtell((SIO*)fd));
  return (tsize_t)SIOread((SIO*)fd, pData, 1, (ILuint)tSize);
}

/*----------------------------------------------------------------------------*/

// We have trouble sometimes reading when writing a file.  Specifically, the only time
//  I have seen libtiff call this is at the beginning of writing a tiff, before
//  anything is ever even written!  Also, we have to return 0, no matter what tSize
//  is.  If we return tSize like would be expected, then TIFFClientOpen fails.
/*static tsize_t 
_tiffFileReadProcW(thandle_t fd, tdata_t pData, tsize_t tSize)
{
  (void)fd;
  (void)pData;
  (void)tSize;
  return 0;
}*/

/*----------------------------------------------------------------------------*/

static tsize_t 
_tiffFileWriteProc(thandle_t fd, tdata_t pData, tsize_t tSize)
{
  // iTrace("---- writing %d @%08x", (ILuint)tSize, SIOtell((SIO*)fd));
  return (tsize_t)SIOwrite((SIO*)fd, pData, 1, (ILuint)tSize);
}

/*----------------------------------------------------------------------------*/

static toff_t
_tiffFileSeekProc(thandle_t fd, toff_t tOff, int whence)
{
  /* we use this as a special code, so avoid accepting it */
  if (tOff == 0xFFFFFFFF)
    return 0xFFFFFFFF;

  SIOseek((SIO*)fd, (ILint64)tOff, (ILenum)whence);
  return SIOtell((SIO*)fd);
  //return tOff;
}

/*----------------------------------------------------------------------------*/
#if 0
static toff_t
_tiffFileSeekProcW(thandle_t fd, toff_t tOff, int whence)
{
  /* we use this as a special code, so avoid accepting it */
  if (tOff == 0xFFFFFFFF)
    return 0xFFFFFFFF;

  SIOseek((SIO*)fd, tOff, whence);
  return SIOtell((SIO*)fd);
  //return tOff;
}
#endif

/*----------------------------------------------------------------------------*/

static int
_tiffFileCloseProc(thandle_t fd)
{
  (void)fd;
  return (0);
}

/*----------------------------------------------------------------------------*/

static toff_t
_tiffFileSizeProc(thandle_t fd)
{
  ILuint Offset, Size;
  
  Offset = SIOtell((SIO*)fd);
  SIOseek((SIO*)fd, 0, IL_SEEK_END);
  Size = SIOtell((SIO*)fd);
  SIOseek((SIO*)fd, Offset, IL_SEEK_SET);

  return (toff_t)Size;
}

/*----------------------------------------------------------------------------*/
#if 0 
static toff_t
_tiffFileSizeProcW(thandle_t fd)
{
  ILint Offset, Size;

  Offset = SIOtell((SIO*)fd);
  SIOseek((SIO*)fd, 0, IL_SEEK_END);

  Size = SIOtell((SIO*)fd);
  SIOseek((SIO*)fd, Offset, IL_SEEK_SET);

  return Size;
}
#endif
/*----------------------------------------------------------------------------*/

static int
_tiffDummyMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize)
{
  (void)fd;
  (void)pbase;
  (void)psize;
  return 0;
}

/*----------------------------------------------------------------------------*/

static void
_tiffDummyUnmapProc(thandle_t fd, tdata_t base, toff_t size)
{
  (void)fd;
  (void)base;
  (void)size;
  return;
}

/*----------------------------------------------------------------------------*/

static TIFF *iTIFFOpen(SIO *io, char *Mode)
{
  TIFF *tif;

  tif = TIFFClientOpen("TIFFMemFile", Mode,
            io,
            _tiffFileReadProc, _tiffFileWriteProc,
            _tiffFileSeekProc, _tiffFileCloseProc,
            _tiffFileSizeProc, _tiffDummyMapProc,
            _tiffDummyUnmapProc);
  return tif;
}

/*----------------------------------------------------------------------------*/


// @TODO:  Accept palettes!

// Internal function used to save the Tiff.
static ILboolean iSaveTiffInternal(ILimage* image)
{
  ILenum  Format;
  ILenum  Compression;
  ILuint  ixLine;
  TIFF  *File;
  ILimage *TempImage;
  ILboolean SwapColors;
  ILubyte *OldData;
  ILint   TempInts[512];
  ILimage *BaseImage = image ? image->BaseImage : NULL;
  ILuint64 Offset;

  if(image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  TIFFSetWarningHandler(warningHandler);
  TIFFSetErrorHandler(errorHandler);

  if (iGetHint(IL_COMPRESSION_HINT) == IL_USE_COMPRESSION)
    Compression = COMPRESSION_LZW;
  else
    Compression = COMPRESSION_NONE;

  if (image->Format == IL_COLOUR_INDEX) {
    if (iGetBppPal(image->Pal.PalType) == 4)  // Preserve the alpha.
      TempImage = iConvertImage(image, IL_RGBA, IL_UNSIGNED_BYTE);
    else
      TempImage = iConvertImage(image, IL_RGB, IL_UNSIGNED_BYTE);
    
    if (TempImage == NULL) {
      return IL_FALSE;
    }
  }
  else {
    TempImage = image;
  }

  // Control writing functions ourself.
  File = iTIFFOpen(&image->io, "w");
  if (File == NULL) {
    iSetError(IL_COULD_NOT_OPEN_FILE);
    return IL_FALSE;
  }

  TIFFSetField(File, TIFFTAG_IMAGEWIDTH,  TempImage->Width);
  TIFFSetField(File, TIFFTAG_IMAGELENGTH, TempImage->Height);
  TIFFSetField(File, TIFFTAG_COMPRESSION, Compression);

  if((TempImage->Format == IL_LUMINANCE) || (TempImage->Format == IL_LUMINANCE_ALPHA))
    TIFFSetField(File, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  else
    TIFFSetField(File, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

  TIFFSetField(File, TIFFTAG_BITSPERSAMPLE,     TempImage->Bpc << 3);
  TIFFSetField(File, TIFFTAG_SAMPLESPERPIXEL,   TempImage->Bpp);

  if ((TempImage->Bpp == iGetBppFormat(IL_RGBA)) ||
      (TempImage->Bpp == iGetBppFormat(IL_LUMINANCE_ALPHA)))
    TIFFSetField(File, TIFFTAG_EXTRASAMPLES, EXTRASAMPLE_ASSOCALPHA);

  TIFFSetField(File, TIFFTAG_PLANARCONFIG,      PLANARCONFIG_CONTIG);
  TIFFSetField(File, TIFFTAG_ROWSPERSTRIP,      1);

  // meta strings

  SetFieldString  (IL_META_MAKE,                  TIFFTAG_MAKE);
  SetFieldString  (IL_META_MODEL,                 TIFFTAG_MODEL);
  SetFieldString  (IL_VERSION_NUM,                TIFFTAG_SOFTWARE);
  SetFieldString  (IL_META_SOFTWARE,              TIFFTAG_SOFTWARE);
  SetFieldString  (IL_META_DOCUMENT_NAME,         TIFFTAG_DOCUMENTNAME);
  SetFieldString  (IL_META_ARTIST,                TIFFTAG_ARTIST);
  SetFieldString  (IL_META_HOST_COMPUTER,         TIFFTAG_HOSTCOMPUTER);
  SetFieldString  (IL_META_IMAGE_DESCRIPTION,     TIFFTAG_IMAGEDESCRIPTION);
  SetFieldString  (IL_META_COPYRIGHT,             TIFFTAG_COPYRIGHT);

  SetFieldDWORD2  (IL_META_PAGE_NUMBER,           TIFFTAG_PAGENUMBER);

  // SetFieldWORD    (IL_META_COMPRESSION,           TIFFTAG_COMPRESSION);

  TIFFSetField(File, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  SetFieldRational(IL_META_X_RESOLUTION,          TIFFTAG_XRESOLUTION);
  SetFieldRational(IL_META_Y_RESOLUTION,          TIFFTAG_YRESOLUTION);
  SetFieldWORD    (IL_META_RESOLUTION_UNIT,       TIFFTAG_RESOLUTIONUNIT);

  // Set the date and time string.
  TIFFSetField(File, TIFFTAG_DATETIME, iMakeString());

  // 24/4/2003
  // Orientation flag is not always supported (Photoshop, ...), orient the image data 
  // and set it always to normal view
  if (TempImage->Origin != IL_ORIGIN_UPPER_LEFT) {
    ILubyte* Data = iGetFlipped(TempImage);
    OldData = TempImage->Data;
    TempImage->Data = Data;
  }
  else
    OldData = TempImage->Data;

  Format = TempImage->Format;
  SwapColors = (Format == IL_BGR || Format == IL_BGRA);
  if (SwapColors)
    iSwapColours(TempImage);

  for (ixLine = 0; ixLine < TempImage->Height; ++ixLine) {
    if (TIFFWriteScanline(File, TempImage->Data + ixLine * TempImage->Bps, ixLine, 0) < 0) {
      TIFFClose(File);
      iSetError(IL_LIB_TIFF_ERROR);
      if (TempImage->Data != OldData) {
        ifree( TempImage->Data );
        TempImage->Data = OldData;
      }
      if (TempImage != image)
        iCloseImage(TempImage);
      return IL_FALSE;
    }
  }

  TIFFWriteDirectory(File);

  TIFFCreateEXIFDirectory(File);

  SetFieldString  (IL_META_SPECTRAL_SENSITIVITY,  EXIFTAG_SPECTRALSENSITIVITY);
  SetFieldString  (IL_META_DATETIME_ORIGINAL,     EXIFTAG_DATETIMEORIGINAL);
  SetFieldString  (IL_META_DATETIME_DIGITIZED,    EXIFTAG_DATETIMEDIGITIZED);

  SetFieldRational(IL_META_EXPOSURE_TIME,         EXIFTAG_EXPOSURETIME);
  SetFieldRational(IL_META_FSTOP,                 EXIFTAG_FNUMBER);
  SetFieldWORD    (IL_META_EXPOSURE_PROGRAM,      EXIFTAG_EXPOSUREPROGRAM);
  SetFieldRational(IL_META_SHUTTER_SPEED,         EXIFTAG_SHUTTERSPEEDVALUE);
  SetFieldRational(IL_META_APERTURE,              EXIFTAG_APERTUREVALUE);
  SetFieldRational(IL_META_BRIGHTNESS,            EXIFTAG_BRIGHTNESSVALUE);
  SetFieldRational(IL_META_EXPOSURE_BIAS,         EXIFTAG_EXPOSUREBIASVALUE);
  SetFieldRational(IL_META_MAX_APERTURE,          EXIFTAG_MAXAPERTUREVALUE);
  SetFieldRational(IL_META_SUBJECT_DISTANCE,      EXIFTAG_SUBJECTDISTANCE);
  SetFieldWORD    (IL_META_METERING_MODE,         EXIFTAG_METERINGMODE);
  SetFieldWORD    (IL_META_LIGHT_SOURCE,          EXIFTAG_LIGHTSOURCE);
  SetFieldWORD    (IL_META_FLASH,                 EXIFTAG_FLASH);
  SetFieldRational(IL_META_FOCAL_LENGTH,          EXIFTAG_FOCALLENGTH);
  SetFieldRational(IL_META_FLASH_ENERGY,          EXIFTAG_FLASHENERGY);

  SetFieldWORD    (IL_META_EXPOSURE_MODE,         EXIFTAG_EXPOSUREMODE);
  SetFieldWORD    (IL_META_WHITE_BALANCE,         EXIFTAG_WHITEBALANCE);
  SetFieldWORD    (IL_META_COLOUR_SPACE,          EXIFTAG_COLORSPACE);
  SetFieldWORD    (IL_META_SENSING_METHOD,        EXIFTAG_SENSINGMETHOD);

  {
    ILuint size;
    void *Data;

    Data = iGetMetax(BaseImage, IL_META_MAKER_NOTE, &size);
    if (Data) {
      TIFFSetField(File, EXIFTAG_MAKERNOTE, (ILushort)size, Data);
    }

    Data = iGetMetax(BaseImage, IL_META_FLASHPIX_VERSION, &size);
    if (Data) {
      TIFFSetField(File, EXIFTAG_FLASHPIXVERSION, Data);
    }

    Data = iGetMetax(BaseImage, IL_META_FILESOURCE, &size);
    if (Data) {
      TIFFSetField(File, EXIFTAG_FILESOURCE, Data);
    }
  }

  // TODO: IL_META_ISO_RATINGS, IL_META_SUBJECT_AREA

  TIFFSetField    (File, EXIFTAG_EXIFVERSION, "0220");
  
  TIFFWriteCustomDirectory(File, &Offset);

  TIFFSetDirectory(File, 0);
  TIFFSetField(File, TIFFTAG_EXIFIFD, Offset);

  // TODO: sub ifd?
  //TIFFSetDirectory(File, 1);
  //TIFFCreateDirectory (File);
  // 24/4/2003
  // Orientation flag is not always supported (Photoshop, ...), orient the image data 
  // and set it always to normal view
  // TIFFSetField(File, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  // SetFieldRational(IL_META_X_RESOLUTION,          TIFFTAG_XRESOLUTION);
  // SetFieldRational(IL_META_Y_RESOLUTION,          TIFFTAG_YRESOLUTION);
  // SetFieldWORD    (IL_META_RESOLUTION_UNIT,       TIFFTAG_RESOLUTIONUNIT);

  // TIFFWriteDirectory(File);

  // TODO: gps ifd

  if (TempImage->Data != OldData) {
    ifree(TempImage->Data);
    TempImage->Data = OldData;
  }

  if (TempImage != image)
    iCloseImage(TempImage);

  TIFFClose(File);

  return IL_TRUE;
}

/*----------------------------------------------------------------------------*/
// Makes a neat date string for the date field.
// From http://www.awaresystems.be/imaging/tiff/tifftags/datetime.html :
// The format is: "YYYY:MM:DD HH:MM:SS", with hours like those on
// a 24-hour clock, and one space character between the date and the
// time. The length of the string, including the terminating NUL, is
// 20 bytes.)
char *iMakeString()
{
  static char TimeStr[20];
  time_t    Time;
  struct tm *CurTime;

  imemclear(TimeStr, 20);

  time(&Time);
#ifdef _WIN32
  _tzset();
#endif
  CurTime = localtime(&Time);

  strftime(TimeStr, 20, "%Y:%m:%d %H:%M:%S", CurTime);
  
  return TimeStr;
}

/*----------------------------------------------------------------------------*/

static ILconst_string iFormatExtsTIF[] = { 
  IL_TEXT("tif"), 
  IL_TEXT("tiff"), 
  NULL 
};

ILformat iFormatTIF = { 
  /* .Validate = */ iIsValidTiff, 
  /* .Load     = */ iLoadTiffInternal, 
  /* .Save     = */ iSaveTiffInternal, 
  /* .Exts     = */ iFormatExtsTIF
};

#endif//IL_NO_TIF
