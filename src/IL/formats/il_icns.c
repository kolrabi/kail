//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2001-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_icns.c
//
// Description: Reads from a Mac OS X icon (.icns) file.
//    Credit for the format of .icns files goes to
//    http://www.macdisk.com/maciconen.php3 and
//    http://ezix.org/project/wiki/MacOSXIcons
//
//-----------------------------------------------------------------------------

//@TODO: Put iSetError calls in when errors occur.
//@TODO: Should we clear the alpha channel just in case there isn't one in the file?
//@TODO: Checks on iread

#include "il_internal.h"
#ifndef IL_NO_ICNS
#include "il_icns.h"

#ifndef IL_NO_JP2
#include "il_jp2.h"

  #if defined(_WIN32) && defined(IL_USE_PRAGMA_LIBS)
    #if defined(_MSC_VER) || defined(__BORLANDC__)
      #ifndef _DEBUG
        #pragma comment(lib, "libjasper.lib")
      #else
        #pragma comment(lib, "libjasper-d.lib")
      #endif
    #endif
  #endif
#endif//IL_NO_JP2


// Internal function to get the header and check it.
static ILboolean iIsValidIcns(SIO* io) {
  char Sig[4];
  ILuint Start = SIOtell(io);
  ILuint read = SIOread(io, Sig, 1, 4);
  SIOseek(io, Start, IL_SEEK_SET);  // Go ahead and restore to previous state

  return read == 4 && memcmp(Sig, "icns", 4) == 0;  // First 4 bytes have to be 'icns'.
}

// Internal function used to load the icon.
static ILboolean iLoadIcnsInternal(ILimage* image) {
  ICNSHEAD  Header;
  ICNSDATA  Entry;
  ILimage   *Image = NULL;
  ILboolean BaseCreated = IL_FALSE;
  SIO       *io;

  if (image == NULL)
  {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  io = &image->io;
  SIOread(io, Header.Head, 4, 1);
  Header.Size = GetBigUInt(&image->io);

  if (strncmp(Header.Head, "icns", 4))  // First 4 bytes have to be 'icns'.
    return IL_FALSE;

  while (SIOtell(io) < Header.Size && !SIOeof(io))
  {
    SIOread(io, Entry.ID, 4, 1);
    Entry.Size = GetBigUInt(&image->io);

    if (!strncmp(Entry.ID, "it32", 4))  // 128x128 24-bit
    {
      if (iIcnsReadData(image, &BaseCreated, IL_FALSE, 128, &Entry, &Image) == IL_FALSE)
        goto icns_error;
    }
    else if (!strncmp(Entry.ID, "t8mk", 4))  // 128x128 alpha mask
    {
      if (iIcnsReadData(image, &BaseCreated, IL_TRUE, 128, &Entry, &Image) == IL_FALSE)
        goto icns_error;
    }
    else if (!strncmp(Entry.ID, "ih32", 4))  // 48x48 24-bit
    {
      if (iIcnsReadData(image, &BaseCreated, IL_FALSE, 48, &Entry, &Image) == IL_FALSE)
        goto icns_error;
    }
    else if (!strncmp(Entry.ID, "h8mk", 4))  // 48x48 alpha mask
    {
      if (iIcnsReadData(image, &BaseCreated, IL_TRUE, 48, &Entry, &Image) == IL_FALSE)
        goto icns_error;
    }
    else if (!strncmp(Entry.ID, "il32", 4))  // 32x32 24-bit
    {
      if (iIcnsReadData(image, &BaseCreated, IL_FALSE, 32, &Entry, &Image) == IL_FALSE)
        goto icns_error;
    }
    else if (!strncmp(Entry.ID, "l8mk", 4))  // 32x32 alpha mask
    {
      if (iIcnsReadData(image, &BaseCreated, IL_TRUE, 32, &Entry, &Image) == IL_FALSE)
        goto icns_error;
    }
    else if (!strncmp(Entry.ID, "is32", 4))  // 16x16 24-bit
    {
      if (iIcnsReadData(image, &BaseCreated, IL_FALSE, 16, &Entry, &Image) == IL_FALSE)
        goto icns_error;
    }
    else if (!strncmp(Entry.ID, "s8mk", 4))  // 16x16 alpha mask
    {
      if (iIcnsReadData(image, &BaseCreated, IL_TRUE, 16, &Entry, &Image) == IL_FALSE)
        goto icns_error;
    }
#ifndef IL_NO_JP2
    else if (!strncmp(Entry.ID, "ic09", 4))  // 512x512 JPEG2000 encoded - Uses JasPer
    {
      if (iIcnsReadData(image, &BaseCreated, IL_FALSE, 512, &Entry, &Image) == IL_FALSE)
        goto icns_error;
    }
    else if (!strncmp(Entry.ID, "ic08", 4))  // 256x256 JPEG2000 encoded - Uses JasPer
    {
      if (iIcnsReadData(image, &BaseCreated, IL_FALSE, 256, &Entry, &Image) == IL_FALSE)
        goto icns_error;
    }
#endif//IL_NO_JP2
    else  // Not a valid format or one that we can use
    {
      image->io.seek(image->io.handle, Entry.Size - 8, IL_SEEK_CUR);
    }
  }

  return IL_TRUE;

icns_error:
  return IL_FALSE;
}

ILboolean iIcnsReadData(ILimage* image, ILboolean *BaseCreated, ILboolean IsAlpha, ILuint Width, ICNSDATA *Entry, ILimage **Image)
{
  ILuint  Position = 0, RLEPos = 0, Channel;
  ILuint  i;
  ILubyte   RLERead, *Data = NULL;
  ILimage   *TempImage = NULL;
  ILboolean ImageAlreadyExists = IL_FALSE;

  // The .icns format stores the alpha and RGB as two separate images, so this
  //  checks to see if one exists for that particular size.  Unfortunately,
  //  there is no guarantee that they are in any particular order.
  if (*BaseCreated && image != NULL)
  {
    TempImage = image;
    while (TempImage != NULL)
    {
      if ((ILuint)Width == TempImage->Width)
      {
        ImageAlreadyExists = IL_TRUE;
        break;
      }
      TempImage = TempImage->Next;
    }
  }

  Data = (ILubyte*) ialloc(Entry->Size - 8);
  if (Data == NULL)
    return IL_FALSE;

  if (!ImageAlreadyExists)
  {
    if (!*BaseCreated)  // Create base image
    {
      iTexImage(image, Width, Width, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);
      image->Origin = IL_ORIGIN_UPPER_LEFT;
      *Image = image;
      *BaseCreated = IL_TRUE;
    }
    else  // Create next image in list
    {
      (*Image)->Next = iNewImage(Width, Width, 1, 4, 1);
      *Image = (*Image)->Next;
      (*Image)->Format = IL_RGBA;
      (*Image)->Origin = IL_ORIGIN_UPPER_LEFT;
    }

    TempImage = *Image;
  }

  if (IsAlpha)  // Alpha is never compressed.
  {
    image->io.read(image->io.handle, Data, Entry->Size - 8, 1);  // Size includes the header
    if (Entry->Size - 8 != Width * Width)
    {
      ifree(Data);
      return IL_FALSE;
    }

    for (i = 0; i < Width * Width; i++)
    {
      TempImage->Data[(i * 4) + 3] = Data[i];
    }
  }
  else if (Width == 256 || Width == 512)  // JPEG2000 encoded - uses JasPer
  {
#ifndef IL_NO_JP2
    image->io.read(image->io.handle, Data, Entry->Size - 8, 1);  // Size includes the header
    if (ilLoadJp2LInternal(TempImage, Data, Entry->Size - 8) == IL_FALSE)
    {
      ifree(Data);
      iSetError(IL_LIB_JP2_ERROR);
      return IL_TRUE;
    }
#else  // Cannot handle this size.
    iSetError(IL_LIB_JP2_ERROR);  //@TODO: Handle this better...just skip the data.
    return IL_FALSE;
#endif//IL_NO_JP2
  }
  else  // RGB data
  {
    image->io.read(image->io.handle, Data, Entry->Size - 8, 1);  // Size includes the header
    if (Width == 128)
      RLEPos += 4;  // There are an extra 4 bytes here of zeros.
    //@TODO: Should we check to make sure they are all 0?

    if (Entry->Size - 8 == Width * Width * 4) // Uncompressed
    {
      //memcpy(TempImage->Data, Data, Entry->Size);
      for (i = 0; i < Width * Width; i++, Position += 4)
      {
        TempImage->Data[i * 4 + 0] = Data[Position+1];
        TempImage->Data[i * 4 + 1] = Data[Position+2];
        TempImage->Data[i * 4 + 2] = Data[Position+3];
      }
    }
    else  // RLE decoding
    {
      for (Channel = 0; Channel < 3; Channel++)
      {
        Position = 0;
        while (Position < Width * Width)
        {
          RLERead = Data[RLEPos];
          RLEPos++;

          if (RLERead >= 128)
          {
            for (i = 0; i < RLERead - 125 && (Position + i) < Width * Width; i++)
            {
              TempImage->Data[Channel + (Position + i) * 4] = Data[RLEPos];
            }
            RLEPos++;
            Position += RLERead - 125;
          }
          else
          {
            for (i = 0; i < RLERead + 1 && (Position + i) < Width * Width; i++)
            {
              TempImage->Data[Channel + (Position + i) * 4] = Data[RLEPos + i];
            }
            RLEPos += RLERead + 1;
            Position += RLERead + 1;
          }
        }
      }
    }
  }

  ifree(Data);
  return IL_TRUE;
}

static ILconst_string iFormatExtsICNS[] = { 
  IL_TEXT("icns"), 
  NULL 
};

ILformat iFormatICNS = { 
  /* .Validate = */ iIsValidIcns, 
  /* .Load     = */ iLoadIcnsInternal, 
  /* .Save     = */ NULL, 
  /* .Exts     = */ iFormatExtsICNS
};

#endif//IL_NO_ICNS
