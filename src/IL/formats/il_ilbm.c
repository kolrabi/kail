//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/08/2009
//
// Filename: io-IL/io/il_ilbm.c
//
// Description: IFF ILBM file (.iff, .ilbm, .lbm) functions
//   IFF ILBM loader, ported from SDL_Image library (IMG_lbm.c)
//   http://www.libsdl.org/cgi/viewvc.cgi/trunk/SDL_image/IMG_lbm.c?view=markup
//
//   Handles Amiga ILBM and PBM images (including .lbm files saved by the PC
//   version of dpaint)
//   Handles ExtraHalfBright and HAM images.
// 
//   Adapted from SDL_image by Ben Campbell (http://scumways.com) 2009-02-23
//
//-----------------------------------------------------------------------------


// TODO: sort out the .iff extension confusion: .iff is currently handled by
// Maya IFF/CIMG handler (il_iff.c), but it should defer to this one if
// fileturns out to be an ILBM. I think the best solution would be to
// rename the IFF handler to CIMG, and create a new iff handler to act as
// a front end, passing off to either il_ilbm or il_cimg...
// For now, this handler only handles .lbm and .ilbm extenstions (but
// traditionally, .iff is more common).

/* This is a ILBM image file loading framework
   Load IFF pictures, PBM & ILBM packing methods, with or without stencil
   Written by Daniel Morais ( Daniel AT Morais DOT com ) in September 2001.
   24 bits ILBM files support added by Marc Le Douarain (http://www.multimania.com/mavati)
   in December 2002.
   EHB and HAM (specific Amiga graphic chip modes) support added by Marc Le Douarain
   (http://www.multimania.com/mavati) in December 2003.
   Stencil and colorkey fixes by David Raulo (david.raulo AT free DOT fr) in February 2004.
   Buffer overflow fix in RLE decompression by David Raulo in January 2008.
*/

#include "il_internal.h"
#ifndef IL_NO_ILBM
#include <stdlib.h>

/* Structure for an IFF picture ( BMHD = Bitmap Header ) */
#include "pack_push.h"
typedef struct
{
  ILushort  w, h;       /* width & height of the bitmap in pixels */
  ILshort   x, y;       /* screen coordinates of the bitmap */
  ILubyte   planes;     /* number of planes of the bitmap */
  ILubyte   mask;       /* mask type ( 0 => no mask ) */
  ILubyte   tcomp;      /* compression type */
  ILubyte   pad1;       /* dummy value, for padding */
  ILushort  tcolor;     /* transparent color */
  ILubyte   xAspect,    /* pixel aspect ratio */
            yAspect;
  ILshort   Lpage;      /* width of the screen in pixels */
  ILshort   Hpage;      /* height of the screen in pixels */
} BMHD;
#include "pack_pop.h"

#define MAXCOLORS 256

static ILboolean iLoadIlbmInternal(ILimage *);
static ILboolean iIsValidIlbm(SIO *io);

static ILboolean iIsValidIlbm(SIO *io)
{
  ILubyte magic[4+4+4];

  ILuint  start = SIOtell(io);
  ILuint  read = SIOread(io, magic, 1, sizeof(magic));
  SIOseek(io, start, IL_SEEK_SET);

  return read == sizeof(magic) 
      && memcmp( magic, "FORM", 4 ) == 0 
      && ( memcmp( magic + 8, "PBM ", 4 ) == 0 
        || memcmp( magic + 8, "ILBM", 4 ) == 0);
}

static ILboolean iLoadIlbmInternal(ILimage *Image)
{
  if (Image == NULL) {
      iSetError(IL_ILLEGAL_OPERATION);
      return IL_FALSE;
  }

  SIO *io = &Image->io;

  if (!iIsValidIlbm(io)) {
      iSetError(IL_INVALID_VALUE);
      return IL_FALSE;
  }

  struct { ILubyte r, g, b; } scratch_pal[MAXCOLORS];
  ILenum      format; /* IL_RGB (ham or 24bit) or IL_COLOUR_INDEX */

  ILubyte     colormap[MAXCOLORS*3], *ptr, count, color, msk;
  ILuint      i, j, bytesperline, nbplanes, plane, h;
  ILuint      remainingbytes;
  ILuint      width;

  char *      error     = NULL;

  ILboolean   isPBM     = IL_FALSE;
  ILuint      nbColors  = 0;
  ILboolean   flagHAM   = IL_FALSE;
  ILboolean   flagEHB   = 0;

  ILubyte *   MiniBuf   = NULL;

  BMHD        bmhd;
  memset( &bmhd, 0, sizeof( BMHD ) );

  ILubyte     id[4];
  if ( !SIOread( io, id, 4, 1 ) ) {
    error = "error reading IFF chunk";
    goto done;
  }

  /* Should be the size of the file minus 4+4 ( 'FORM'+size ) */
  ILuint      size      = 0;
  if ( !SIOread( io, &size, 4, 1 ) ) {
    error = "error reading IFF chunk size";
    goto done;
  }

  /* As size is not used here, no need to swap it */

  if ( memcmp( id, "FORM", 4 ) != 0 ) {
    iSetError(IL_INVALID_FILE_HEADER);
    error = "not a IFF file";
    goto done;
  }

  if ( !SIOread( io, id, 4, 1 ) ) {
    error = "error reading IFF chunk";
    goto done;
  }

  /* File format : PBM=Packed Bitmap, ILBM=Interleaved Bitmap */
  if ( !memcmp( id, "PBM ", 4 ) ) {
    isPBM = IL_TRUE;
  } else if ( memcmp( id, "ILBM", 4 ) )  {
    iSetError(IL_INVALID_FILE_HEADER);
    error = "not a IFF picture";
    goto done;
  }

  /* look for BODY chunk */
  while ( memcmp( id, "BODY", 4 ) != 0 ) {
    if ( !SIOread( io, id, 4, 1 ) ) {
      error = "error reading IFF chunk";
      goto done;
    }

    if ( !SIOread( io, &size, 4, 1 ) )  {
      error = "error reading IFF chunk size";
      goto done;
    }

    ILuint bytesloaded = 0;
    BigUInt(&size);

    if ( !memcmp( id, "BMHD", 4 ) ) { /* Bitmap header */
      bytesloaded = SIOread( io, &bmhd, 1, sizeof( BMHD ) );
      if (bytesloaded != sizeof( BMHD )) {
        error = "error reading BMHD chunk";
        goto done;
      }

      BigUShort(&bmhd.w);
      BigUShort(&bmhd.h);
      BigShort (&bmhd.x);
      BigShort (&bmhd.y);
      BigUShort(&bmhd.tcolor);
      BigShort (&bmhd.Lpage);
      BigShort (&bmhd.Hpage);
    }

    if ( !memcmp( id, "CMAP", 4 ) ) { /* palette ( Color Map ) */
      bytesloaded = SIOread( io, &colormap, 1, size );
      if (bytesloaded != size) {
        error = "error reading CMAP chunk";
        goto done;
      }

      nbColors = size / 3;
    }

    if ( !memcmp( id, "CAMG", 4 ) ) { /* Amiga ViewMode  */
      ILuint viewmodes;
      bytesloaded = SIOread( io, &viewmodes, 1, sizeof(viewmodes) );
      if (bytesloaded != sizeof(viewmodes)) {
        error = "error reading CAMG chunk";
        goto done;
      }

      bytesloaded = size; // FIXME: shouldnt it be sizeof(viewmodes)?

      BigUInt(&viewmodes);

      if ( viewmodes & 0x0800 )
        flagHAM = 1;  
      if ( viewmodes & 0x0080 )
        flagEHB = 1;
    }

    if ( memcmp( id, "BODY", 4 ) ) {
      if ( size & 1 ) ++size;     /* padding ! */
      size -= bytesloaded;

      /* skip the remaining bytes of this chunk */
      if ( size ) SIOseek( io, size, IL_SEEK_CUR );
    }
  }

  /* compute some usefull values, based on the bitmap header */

  width         =   ( bmhd.w + 15 ) & 0xFFFFFFF0;  /* Width in pixels modulo 16 */
  bytesperline  = ( ( bmhd.w + 15 ) / 16 ) * 2;
  nbplanes      =     bmhd.planes;

  if ( isPBM ) {                     /* File format : 'Packed Bitmap' */
    bytesperline *= 8;
    nbplanes      = 1;
  }

  if ( bmhd.mask & 1 )              /* There is a mask ( 'stencil' ) */
    ++nbplanes;   

  /* Allocate memory for a temporary buffer ( used for decompression/deinterleaving ) */

  MiniBuf = (ILubyte *)malloc( bytesperline * nbplanes );
  if ( !MiniBuf ) {
    iSetError( IL_OUT_OF_MEMORY );
    error = "no enough memory for temporary buffer";
    goto done;
  }

  if( bmhd.planes==24 || flagHAM==1 ) {
    format = IL_BGR;
  } else {
    format = IL_COLOUR_INDEX;
  }

  if( !ilTexImage_( Image, width, bmhd.h, 1, (format==IL_COLOUR_INDEX)?1:3, format, IL_UNSIGNED_BYTE, NULL ) )
    goto done;

  Image->Origin = IL_ORIGIN_UPPER_LEFT;

#if 0 /*  No transparent colour support in DevIL? (TODO: confirm) */
  if ( bmhd.mask & 2 )               /* There is a transparent color */
      SDL_SetColorKey( Image, SDL_ioCOLORKEY, bmhd.tcolor );
#endif

  /* Update palette informations */

  /* There is no palette in 24 bits ILBM file */
  if ( nbColors>0 && flagHAM==0 )
  {
    int nbrcolorsfinal = 1 << nbplanes;
    ptr = &colormap[0];

    for ( i=0; i<nbColors; i++ )  {
      scratch_pal[i].r = *ptr++;
      scratch_pal[i].g = *ptr++;
      scratch_pal[i].b = *ptr++;
    }

    /* Amiga EHB mode (Extra-Half-Bright) */
    /* 6 bitplanes mode with a 32 colors palette */
    /* The 32 last colors are the same but divided by 2 */
    /* Some Amiga pictures save 64 colors with 32 last wrong colors, */
    /* they shouldn't !, and here we overwrite these 32 bad colors. */
    if ( (nbColors==32 || flagEHB ) && (1<<bmhd.planes)==64 )
    {
        nbColors = 64;
        ptr = &colormap[0];
        for ( i=32; i<64; i++ ) {
          scratch_pal[i].r = (*ptr++)/2;
          scratch_pal[i].g = (*ptr++)/2;
          scratch_pal[i].b = (*ptr++)/2;
        }
    }

    /* If nbColors < 2^nbplanes, repeat the colormap */
    /* This happens when pictures have a stencil mask */
    if ( nbrcolorsfinal > (1<<bmhd.planes) ) {
      nbrcolorsfinal = (1<<bmhd.planes);
    }
    for ( i=nbColors; i < (ILuint)nbrcolorsfinal; i++ )
    {
      scratch_pal[i].r = scratch_pal[i%nbColors].r;
      scratch_pal[i].g = scratch_pal[i%nbColors].g;
      scratch_pal[i].b = scratch_pal[i%nbColors].b;
    }

    if ( !isPBM )
      ilRegisterPal( scratch_pal, 3*nbrcolorsfinal, IL_PAL_RGB24 );
  }

  /* Get the bitmap */
  for ( h=0; h < bmhd.h; h++ ) {
    /* uncompress the datas of each planes */
    for ( plane=0; plane < nbplanes; plane++ ) {
      ptr             = MiniBuf + ( plane * bytesperline );
      remainingbytes  = bytesperline;

      if ( bmhd.tcomp == 1 ) {    /* Datas are compressed */
        do {
          if ( !SIOread( io, &count, 1, 1 ) ) {
            error = "error reading BODY chunk";
            goto done;
          }

          if ( count & 0x80 ) {
            count ^= 0xFF;
            count += 2; /* now it */

            if ( ( count > remainingbytes ) 
              || !SIOread( io, &color, 1, 1 ) ) {

              if( count>remainingbytes)
                iSetError(IL_ILLEGAL_FILE_VALUE );
              
              error = "error reading BODY chunk";
              goto done;
            }
              
            memset( ptr, color, count );
          } else {
            ++count;

          if ( ( count > remainingbytes ) 
            || !SIOread( io, ptr, count, 1 ) ) {

            if( count>remainingbytes)
              iSetError(IL_ILLEGAL_FILE_VALUE );

            error = "error reading BODY chunk";
            goto done;
          }
        }

        ptr += count;
        remainingbytes -= count;

      } while ( remainingbytes > 0 );
    } else {
      if ( !SIOread( io, ptr, bytesperline, 1 ) )  {
        error = "error reading BODY chunk";
        goto done;
      }
    }
  }

  /* One line has been read, store it ! */

  ptr = Image->Data;

  if ( nbplanes==24 || flagHAM==1 ) {
    ptr += h * width * 3;
  } else {
    ptr += h * width;
  }

  if ( isPBM ) {
     /* File format : 'Packed Bitmap' */
     memcpy( ptr, MiniBuf, width );
  } else {                      
    /* We have to un-interlace the bits ! */
    if ( nbplanes!=24 && flagHAM==0 ) {
      size = ( width + 7 ) / 8;

      for ( i=0; i < size; i++ ) {
        memset( ptr, 0, 8 );

        for ( plane=0; plane < nbplanes; plane++ ) {
          color = *( MiniBuf + i + ( plane * bytesperline ) );
          msk = 0x80;

          for ( j=0; j<8; j++ ) {
            if ( ( plane + j ) <= 7 ) ptr[j] |= (ILubyte)( color & msk ) >> ( 7 - plane - j );
            else                      ptr[j] |= (ILubyte)( color & msk ) << ( plane + j - 7 );
            msk >>= 1;
          }
        }
        ptr += 8;
      }
    } else {
      ILuint finalcolor = 0;
      size = ( width + 7 ) / 8;

      /* 24 bitplanes ILBM : R0...R7,G0...G7,B0...B7 */
      /* or HAM (6 bitplanes) or HAM8 (8 bitplanes) modes */
      for ( i=0; i<width; i=i+8 ) {
        ILubyte maskBit = 0x80;

        for ( j=0; j<8; j++ ) {
          ILuint pixelcolor = 0;
          ILuint maskColor = 1;
          ILubyte dataBody;

          for ( plane=0; plane < nbplanes; plane++ ) {
            dataBody = MiniBuf[ plane*size+i/8 ];

            if ( dataBody & maskBit )
              pixelcolor = pixelcolor | maskColor;
              maskColor  = maskColor<<1;
            }

            /* HAM : 12 bits RGB image (4 bits per color component) */
            /* HAM8 : 18 bits RGB image (6 bits per color component) */
            if ( flagHAM ) {
              switch( pixelcolor>>(nbplanes-2) ) {
                case 0: /* take direct color from palette */
                  finalcolor = colormap[ pixelcolor*3 ] + (colormap[ pixelcolor*3+1 ]<<8) + (colormap[ pixelcolor*3+2 ]<<16);
                  break;
                case 1: /* modify only blue component */
                  finalcolor = finalcolor&0x00FFFF;
                  finalcolor = finalcolor | (pixelcolor<<(16+(10-nbplanes)));
                  break;
                case 2: /* modify only red component */
                  finalcolor = finalcolor&0xFFFF00;
                  finalcolor = finalcolor | pixelcolor<<(10-nbplanes);
                  break;
                case 3: /* modify only green component */
                  finalcolor = finalcolor&0xFF00FF;
                  finalcolor = finalcolor | (pixelcolor<<(8+(10-nbplanes)));
                  break;
              }
            } else {
              finalcolor = pixelcolor;
            }

#if defined( __LITTLE_ENDIAN__ )
            *ptr++ = (ILubyte)(finalcolor>>16);
            *ptr++ = (ILubyte)(finalcolor>>8);
            *ptr++ = (ILubyte)(finalcolor);
#else
            *ptr++ = (ILubyte)(finalcolor);
            *ptr++ = (ILubyte)(finalcolor>>8);
            *ptr++ = (ILubyte)(finalcolor>>16);
#endif
            maskBit = maskBit>>1;
          }
        }
      }
    }
  }

done:

  if ( MiniBuf ) free( MiniBuf );

  if ( error ) {
    iTrace("**** Error: %s", error);
    return IL_FALSE;
  }

  return IL_TRUE;
}

ILconst_string iFormatExtsILBM[] = { 
  IL_TEXT("ilbm"), 
  IL_TEXT("lbm"), 
  IL_TEXT("ham"), 
  NULL 
};

ILformat iFormatILBM = { 
  .Validate = iIsValidIlbm, 
  .Load     = iLoadIlbmInternal, 
  .Save     = NULL, 
  .Exts     = iFormatExtsILBM
};

#endif//IL_NO_ILBM

