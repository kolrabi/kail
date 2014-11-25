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

#include "il_jp2.h"
#include "il_png.h"

static struct {
  const char *ID;
  ILboolean   HasAlpha;
  ILuint      Size;
} ICNSFormats[] = {

  { "it32", IL_FALSE,  128 }, //    128x128 24-bit
  { "t8mk", IL_TRUE,   128 }, //    128x128 alpha mask
  { "ih32", IL_FALSE,   48 }, //     48x 48 24-bit
  { "h8mk", IL_TRUE,    48 }, //     48x 48 alpha mask
  { "il32", IL_FALSE,   32 }, //     32x 32 24-bit
  { "l8mk", IL_TRUE,    32 }, //     32x 32 alpha mask
  { "is32", IL_FALSE,   16 }, //     16x 16 24-bit
  { "s8mk", IL_TRUE,    16 }, //     16x 16 alpha mask

#if !defined(IL_NO_JP2) || !defined(IL_NO_PNG)

  { "ic14", IL_FALSE,  512 }, // 256x256@2x JPEG2000 / PNG encoded
  { "ic13", IL_FALSE,  128 }, //  64x 64@2x JPEG2000 / PNG encoded
  { "ic12", IL_FALSE,   64 }, //  32x 32@2x JPEG2000 / PNG encoded
  { "ic11", IL_FALSE,   32 }, //  16x 16@2x JPEG2000 / PNG encoded
  { "ic10", IL_FALSE, 1024 }, //1024x1024   JPEG2000 / PNG encoded
  { "ic09", IL_FALSE,  512 }, // 512x512  x JPEG2000 / PNG encoded
  { "ic08", IL_FALSE,  256 }, // 256x256    JPEG2000 / PNG encoded
  { "ic07", IL_FALSE,  128 }, // 128x128    JPEG2000 / PNG encoded
  { "icp6", IL_FALSE,   64 }, //  64x 64    JPEG2000 / PNG encoded
  { "icp5", IL_FALSE,   32 }, //  32x 32    JPEG2000 / PNG encoded
  { "icp4", IL_FALSE,   16 }, //  16x 16    JPEG2000 / PNG encoded

#endif

  { NULL,   IL_FALSE,    0 }

  /*
   */
};

// Internal function to get the header and check it.
static ILboolean iIsValidIcns(SIO* io) {
  char Sig[4];
  ILuint Start = SIOtell(io);
  ILuint Read = SIOread(io, Sig, 1, 4);
  SIOseek(io, Start, IL_SEEK_SET);  // Go ahead and restore to previous state

  return Read == 4 && memcmp(Sig, "icns", 4) == 0;  // First 4 bytes have to be 'icns'.
}

// Internal function used to load the icon.
static ILboolean iLoadIcnsInternal(ILimage* image) {
  ICNSHEAD  Header;
  ICNSDATA  Entry;
  ILimage   *Image = NULL;
  ILboolean BaseCreated = IL_FALSE;
  SIO       *io;
  ILboolean FoundAny = IL_FALSE;

  if (image == NULL)
  {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  io = &image->io;

  SIOread(io, Header.Head, 4, 1);
  Header.Size = GetBigUInt(io);

  if (strncmp(Header.Head, "icns", 4)) {
    // First 4 bytes have to be 'icns'.
    iSetError(IL_INVALID_FILE_HEADER);
    return IL_FALSE;
  }

  while (SIOtell(io) < Header.Size && !SIOeof(io))
  {
    ILuint i, EntryEnd;
    ILboolean ok = IL_TRUE;

    SIOread(io, Entry.ID, 4, 1);

    if (!isalnum(Entry.ID[0]) || !isalnum(Entry.ID[1]) || !isalnum(Entry.ID[2]) || !isalnum(Entry.ID[3])) {
      iTrace("**** Non-ASCII icon type found");
      iSetError(IL_INVALID_FILE_HEADER);
      return IL_FALSE;
    }

    Entry.Size = GetBigUInt(io);

    EntryEnd = SIOtell(io) + Entry.Size - 8;

    for (i=0; ICNSFormats[i].ID; i++) {
      if (!strncmp(Entry.ID, ICNSFormats[i].ID, 4)) {
        if (!iIcnsReadData(image, &BaseCreated, ICNSFormats[i].HasAlpha, ICNSFormats[i].Size, &Entry, &Image)) {
          ok = IL_FALSE;
        } else {
          FoundAny = IL_TRUE;
        }
        break;
      }
    }

    if (!ICNSFormats[i].ID || !ok) {
      // Not a valid format or one that we can use
      iTrace("Unknown or unsupported icon type '%c%c%c%c'. Skipping...", Entry.ID[0], Entry.ID[1], Entry.ID[2], Entry.ID[3]);
    }
    SIOseek(io, EntryEnd, IL_SEEK_SET);
  }

  return FoundAny;
}

ILboolean iIcnsReadData(ILimage* image, ILboolean *BaseCreated, ILboolean IsAlpha, ILuint Width, ICNSDATA *Entry, ILimage **Image)
{
  ILuint  Position = 0, RLEPos = 0, Channel, DataStart;
  ILuint  i;
  ILubyte   RLERead, *Data = NULL;
  ILimage   *TempImage = NULL;
  ILboolean ImageAlreadyExists = IL_FALSE;
  SIO *io;

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

  io = &image->io;

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

  DataStart = SIOtell(io);
  SIOread(io, Data, Entry->Size - 8, 1);  // Size includes the header

  if (IsAlpha)  // Alpha is never compressed.
  {
    if (Entry->Size - 8 != Width * Width) {
      ifree(Data);
      return IL_FALSE;
    }

    for (i = 0; i < Width * Width; i++) {
      TempImage->Data[(i * 4) + 3] = Data[i];
    }
  }
  else if (!strncmp((const char*)Data+4, "jP\040\040", 4))  // JPEG2000 encoded - uses JasPer
  {
#ifndef IL_NO_JP2
    ILboolean JP2ok;
    SIOseek(io, DataStart, IL_SEEK_SET);
    TempImage->io = *io;

    JP2ok = iLoadJp2Internal(TempImage);
    if (TempImage != image)
      TempImage->io.handle = NULL;

    if (!JP2ok) {
      iGetError();
      ifree(Data);
      return IL_FALSE;
    }
#else  // Cannot handle this size.
    iTrace("**** Cannot load JPEG2000 encoded sub icon, IL was built without JPEG2000 support.")
    iCloseImage(TempImage);
    return IL_FALSE;
#endif//IL_NO_JP2
  }
  else if (!strncmp((const char*)Data, "\x89PNG", 4))  // PNG encoded
  {
#ifndef IL_NO_PNG
    ILboolean PNGok;
    SIOseek(io, DataStart, IL_SEEK_SET);
    TempImage->io = *io;

    PNGok = iLoadPngInternal(TempImage);
    if (TempImage != image)
      TempImage->io.handle = NULL;

    if (!PNGok) {
      iGetError();
      ifree(Data);
      return IL_FALSE;
    }
#else  // Cannot handle this size.
    iTrace("**** Cannot load PNG encoded sub icon, IL was built without PNG support.")
    iCloseImage(TempImage);
    return IL_FALSE;
#endif//IL_NO_JP2
  }
  else  // RGB data
  {
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
        while (Position < Width * Width && RLEPos < Entry->Size-8)
        {
          RLERead = Data[RLEPos];
          RLEPos++;

          if (RLERead >= 128)
          {
            for (i = 0; i < (ILuint)RLERead - 125 && (Position + i) < Width * Width; i++)
            {
              TempImage->Data[Channel + (Position + i) * 4] = Data[RLEPos];
            }
            RLEPos++;
            Position += RLERead - 125;
          }
          else
          {
            for (i = 0; i < (ILuint)RLERead + 1 && (Position + i) < Width * Width; i++)
            {
              if (RLEPos+i>=Entry->Size - 8) {
                iTrace("**** Unexpected end of data");
                break;
              }
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
