#include "il_internal.h"

#include "pack_push.h"
typedef struct {
  ILushort Tag;
  ILushort Type;
  ILuint   Length;
  ILuint   Offset;
} ExifDirEntry;
#include "pack_pop.h"

static ILuint iExifGetEntrySize(const ExifDirEntry *Entry) {
  ILuint Size = Entry->Length;
  switch (Entry->Type) {
    case IL_EXIF_TYPE_BYTE:     Size = Entry->Length; break;
    case IL_EXIF_TYPE_ASCII:    Size = Entry->Length; break;
    case IL_EXIF_TYPE_WORD:     Size = Entry->Length * 2; break;
    case IL_EXIF_TYPE_DWORD:    Size = Entry->Length * 4; break;
    case IL_EXIF_TYPE_RATIONAL: Size = Entry->Length * 8; break;
    case IL_EXIF_TYPE_SBYTE:     Size = Entry->Length; break;
    case IL_EXIF_TYPE_SWORD:     Size = Entry->Length * 2; break;
    case IL_EXIF_TYPE_SDWORD:    Size = Entry->Length * 4; break;
    case IL_EXIF_TYPE_SRATIONAL: Size = Entry->Length * 8; break;
    }
  return Size;
}

static ILboolean iExifReadDirEntry(SIO *io, ExifDirEntry *Entry, ILboolean BigEndian) {
  if (SIOread(io, Entry, 1, sizeof(ExifDirEntry) ) != sizeof(ExifDirEntry)) {
    return IL_FALSE;
  }

  if (BigEndian) {
    BigUShort(&Entry->Tag);
    BigUShort(&Entry->Type);
    BigUInt  (&Entry->Length);
    BigUInt  (&Entry->Offset);
  } else {
    UShort(&Entry->Tag);
    UShort(&Entry->Type);
    UInt  (&Entry->Length);
    UInt  (&Entry->Offset);
  }

  return IL_TRUE;
}

static ILuint iExifReadDir(ILimage *Image, ILuint Start, ILuint Offset, ILenum IFD, ILboolean BigEndian) {
  SIO *io = &Image->io;
  ILushort Num;
  ILint i;

  SIOseek(io, Offset + Start, IL_SEEK_SET);

  if (SIOread(io, &Num, 1, 2) != 2) {
    iTrace("**** could not read dir entry num @%08x", Offset + Start);
    return 0;
  }
  if (BigEndian) {
    BigUShort(&Num);
  } else {
    UShort   (&Num);
  }

  for (i = 0; i<Num; i++) {
    ExifDirEntry Entry;
    ILuint Size;

    if (!iExifReadDirEntry(io, &Entry, BigEndian)) return 0;

    Size = iExifGetEntrySize(&Entry);

    if (Entry.Tag == IL_TIFF_IFD_EXIF || Entry.Tag == IL_TIFF_IFD_GPS || Entry.Tag == IL_TIFF_IFD_INTEROP) {
      iTrace("---- %04x", Entry.Tag);
      ILuint Back = SIOtell(io);
      Offset = Entry.Offset;
      while(Offset) {
        Offset = iExifReadDir(Image, Start, Offset, Entry.Tag, BigEndian);
      }
      SIOseek(io, Back, IL_SEEK_SET);
    } else {
      if (Size <= 4) {
        Entry.Offset = SIOtell(io) - Start - 4;
      }

      ILuint Back = SIOtell(io);
      ILexif *Exif = ioalloc(ILexif);
      if (!Exif) return 0;

      Exif->Data = ialloc(Size + 1);
      if (!Exif->Data) {
        ifree(Exif);
        return 0;
      }

      Exif->IFD = IFD;
      Exif->ID  = Entry.Tag;
      Exif->Type = Entry.Type;
      Exif->Size = Size;
      Exif->Length = Entry.Length;
      Exif->Next = NULL;

      SIOseek(io, Start + Entry.Offset, IL_SEEK_SET);
      if (SIOread(io, Exif->Data, 1, Size) != Size) {
        ifree(Exif->Data);
        ifree(Exif);
        return 0;
      }

      if (BigEndian) {
        switch(Entry.Type) {
          case IL_EXIF_TYPE_WORD:       BigUShort(Exif->Data); break;          
          case IL_EXIF_TYPE_DWORD:      BigUInt(Exif->Data); break;          
          case IL_EXIF_TYPE_RATIONAL:   BigUInt(Exif->Data); BigUInt(((ILuint*)Exif->Data)+1); break;
          case IL_EXIF_TYPE_SWORD:      BigShort(Exif->Data); break;          
          case IL_EXIF_TYPE_SDWORD:     BigInt(Exif->Data); break;          
          case IL_EXIF_TYPE_SRATIONAL:  BigInt(Exif->Data); BigInt(((ILint*)Exif->Data)+1); break;
          case IL_EXIF_TYPE_FLOAT:      BigFloat(Exif->Data); break;          
          case IL_EXIF_TYPE_DOUBLE:     BigDouble(Exif->Data); break;          
        }
      } else {
        switch(Entry.Type) {
          case IL_EXIF_TYPE_WORD:       UShort(Exif->Data); break;          
          case IL_EXIF_TYPE_DWORD:      UInt(Exif->Data); break;          
          case IL_EXIF_TYPE_RATIONAL:   UInt(Exif->Data); UInt(((ILuint*)Exif->Data)+1); break;
          case IL_EXIF_TYPE_SWORD:      Short(Exif->Data); break;          
          case IL_EXIF_TYPE_SDWORD:     Int(Exif->Data); break;          
          case IL_EXIF_TYPE_SRATIONAL:  Int(Exif->Data); Int(((ILint*)Exif->Data)+1); break;
          case IL_EXIF_TYPE_FLOAT:      Float(Exif->Data); break;          
          case IL_EXIF_TYPE_DOUBLE:     Double(Exif->Data); break;          
        }
      }

      ((ILubyte*)Exif->Data)[Size] = 0;

      if (Image->ExifTags == NULL) {
        Image->ExifTags = Exif;
      } else {
        ILexif *ImageExif = Image->ExifTags;
        while(ImageExif->Next != NULL) ImageExif = ImageExif->Next;
        ImageExif->Next = Exif;
      }
      SIOseek(io, Back, IL_SEEK_SET);
    }
  }

  if (SIOread(io, &Offset, 1, 4) != 4) return 0;
  if (BigEndian) {
    BigUInt(&Offset);
  } else {
    UInt(&Offset);
  }

  return Offset;
}

ILboolean iExifLoad(ILimage *Image) {
  SIO *io = &Image->io;
  ILuint Start = SIOtell(io);

  ILubyte hdr[4];
  ILuint  Offset;
  ILexif *Exif;
  ILboolean BigEndian;

  if (SIOread(io, hdr, 1, 4) != 4) return IL_FALSE;
  if (memcmp(hdr, "MM\0\x2a", 4) && memcmp(hdr, "II\x2a\0", 4)) return IL_FALSE;

  // clear old exif data first
  Exif = Image->ExifTags;
  while(Exif) {
    ILexif *NextExif = Exif->Next;
    ifree(Exif->Data);
    ifree(Exif);
    Exif = NextExif;
  }
  Image->ExifTags = NULL;

  BigEndian = (memcmp(hdr, "MM\0\x2a", 4) == 0);

  if (SIOread(io, &Offset, 1, 4) != 4) return IL_FALSE;
  if (BigEndian) {
    BigUInt(&Offset);
  } else {
    UInt(&Offset);
  }

  while(Offset) {
    Offset = iExifReadDir(Image, Start, Offset, 0, BigEndian);
  }
  return IL_TRUE;
}

static ILboolean iIsValidExif(SIO *io) {
  ILuint Start = SIOtell(io);
  ILubyte hdr[4];
  ILboolean Ok = SIOread(io, hdr, 1, 4) == 4 && (!memcmp(hdr, "MM\0\x2a", 4) || !memcmp(hdr, "II\x2a\0", 4));

  SIOseek(io, Start, IL_SEEK_SET);

  return Ok;
}

static ILboolean iLoadExifInternal(ILimage *Image) {
  return iExifLoad(Image);
}

ILboolean iExifSave(ILimage *Image) {
  SIO *io = &Image->io;
  ILushort  Version       = 42;
  ILuint    IFD0Offset    = 8;
  ILuint    IFD0Count     = 0, IFD0DirSize    = 2 + 4, IFD0DataSize     = 0;
  ILuint    ExifCount     = 0, ExifDirSize    = 0, ExifDataSize     = 0;
  ILuint    GPSCount      = 0, GPSDirSize     = 0, GPSDataSize      = 0;
  ILuint    InteropCount  = 0, InteropDirSize = 0, InteropDataSize  = 0;
  ILexif *  Exif          = Image->ExifTags;
  ILushort Num, TmpShort;

  ILuint    DataOffset = 0, TmpUint;

  // find out directory sizes
  Exif = Image->ExifTags;
  while (Exif) {
    if (Exif->IFD == IL_TIFF_IFD0) {
      IFD0Count ++;
      IFD0DirSize += 12;
      if (Exif->Size > 4) IFD0DataSize += Exif->Size;
    } else if (Exif->IFD == IL_TIFF_IFD_EXIF) {
      ExifCount ++;
      ExifDirSize += 12;
      if (Exif->Size > 4) ExifDataSize += Exif->Size;
    } else if (Exif->IFD == IL_TIFF_IFD_GPS) {
      GPSCount ++;
      GPSDirSize += 12;
      if (Exif->Size > 4) GPSDataSize += Exif->Size;
    } else if (Exif->IFD == IL_TIFF_IFD_INTEROP) {
      InteropCount ++;
      InteropDirSize += 12;
      if (Exif->Size > 4) InteropDataSize += Exif->Size;
    }
    Exif = Exif->Next;
  }

  if (ExifCount   > 0)    { IFD0Count++;  IFD0DirSize += 12; ExifDirSize += 2 + 4; }
  if (GPSCount    > 0)    { IFD0Count++;  IFD0DirSize += 12; GPSDirSize += 2 + 4; }
  if (InteropCount > 0)   { IFD0Count++;  IFD0DirSize += 12; InteropDirSize += 2 + 4; }

#ifdef __BIG_ENDIAN__ 
  SIOputs(io, "MM");
#else
  SIOputs(io, "II");
#endif
  SIOwrite(io, &Version, 1, 2);
  SIOwrite(io, &IFD0Offset, 1, 4);

  // write ifd0
  DataOffset = IFD0Offset + IFD0DirSize;
  Num = IFD0Count;

  SIOwrite(io, &Num, 2, 1);
  Exif = Image->ExifTags;
  while (Exif) {
    if (Exif->IFD == IL_TIFF_IFD0) {
      SIOwrite(io, &Exif->ID, 2, 1);
      SIOwrite(io, &Exif->Type, 2, 1);
      SIOwrite(io, &Exif->Length, 4, 1);

      if (Exif->Size > 4) {
        SIOwrite(io, &DataOffset, 4, 1);
        DataOffset += Exif->Size;
      } else {
        SIOwrite(io, Exif->Data, Exif->Size, 1);
        SIOpad(io, 4-Exif->Size);
      }
    } 
    Exif = Exif->Next;
  }

  if (ExifCount > 0) {
    TmpShort = IL_TIFF_IFD_EXIF;    SIOwrite(io, &TmpShort, 2, 1);
    TmpShort = IL_EXIF_TYPE_DWORD;  SIOwrite(io, &TmpShort, 2, 1);
    TmpUint  = 1;                   SIOwrite(io, &TmpUint, 4, 1);
    TmpUint  = DataOffset;      SIOwrite(io, &TmpUint, 4, 1);
  }

  if (GPSCount > 0) {
    TmpShort = IL_TIFF_IFD_GPS;     SIOwrite(io, &TmpShort, 2, 1);
    TmpShort = IL_EXIF_TYPE_DWORD;  SIOwrite(io, &TmpShort, 2, 1);
    TmpUint  = 1;                   SIOwrite(io, &TmpUint, 4, 1);
    TmpUint  = DataOffset + ExifDataSize + ExifDirSize;   SIOwrite(io, &TmpUint, 4, 1);
  }

  if (InteropCount) {
    TmpShort = IL_TIFF_IFD_INTEROP;     SIOwrite(io, &TmpShort, 2, 1);
    TmpShort = IL_EXIF_TYPE_DWORD;      SIOwrite(io, &TmpShort, 2, 1);
    TmpUint  = 1;                       SIOwrite(io, &TmpUint, 4, 1);
    TmpUint  = DataOffset + ExifDataSize + ExifDirSize + 
               GPSDataSize + GPSDirSize;   SIOwrite(io, &TmpUint, 4, 1);
  }
  TmpUint  = 0; SIOwrite(io, &TmpUint, 4, 1);

  Exif = Image->ExifTags;
  while (Exif) {
    if (Exif->IFD == IL_TIFF_IFD0) {
      if (Exif->Size > 4) {
        SIOwrite(io, Exif->Data, Exif->Size, 1);
      }
    } 
    Exif = Exif->Next;
  }

  if (ExifCount > 0) {
    // write exif ifd
    Num = ExifCount;
    DataOffset += ExifDirSize;

    SIOwrite(io, &Num, 2, 1);
    Exif = Image->ExifTags;
    while (Exif) {
      if (Exif->IFD == IL_TIFF_IFD_EXIF) {
        SIOwrite(io, &Exif->ID, 2, 1);
        SIOwrite(io, &Exif->Type, 2, 1);
        SIOwrite(io, &Exif->Length, 4, 1);

        if (Exif->Size > 4) {
          SIOwrite(io, &DataOffset, 4, 1);
          DataOffset += Exif->Size;
        } else {
          SIOwrite(io, Exif->Data, Exif->Size, 1);
          SIOpad(io, 4-Exif->Size);
        }
      } 
      Exif = Exif->Next;
    }
    TmpUint  = 0; SIOwrite(io, &TmpUint, 4, 1);
    Exif = Image->ExifTags;
    while (Exif) {
      if (Exif->IFD == IL_TIFF_IFD_EXIF) {
        if (Exif->Size > 4) {
          SIOwrite(io, Exif->Data, Exif->Size, 1);
        }
      } 
      Exif = Exif->Next;
    }
  }

  if (GPSCount > 0) {
    // write gps ifd
    Num = GPSCount;
    DataOffset += GPSDirSize;

    SIOwrite(io, &Num, 2, 1);
    Exif = Image->ExifTags;
    while (Exif) {
      if (Exif->IFD == IL_TIFF_IFD_GPS) {
        SIOwrite(io, &Exif->ID, 2, 1);
        SIOwrite(io, &Exif->Type, 2, 1);
        SIOwrite(io, &Exif->Length, 4, 1);

        if (Exif->Size > 4) {
          SIOwrite(io, &DataOffset, 4, 1);
          DataOffset += Exif->Size;
        } else {
          SIOwrite(io, Exif->Data, Exif->Size, 1);
          SIOpad(io, 4-Exif->Size);
        }
      } 
      Exif = Exif->Next;
    }
    TmpUint  = 0; SIOwrite(io, &TmpUint, 4, 1);

    Exif = Image->ExifTags;
    while (Exif) {
      if (Exif->IFD == IL_TIFF_IFD_GPS) {
        if (Exif->Size > 4) {
          SIOwrite(io, Exif->Data, Exif->Size, 1);
        }
      } 
      Exif = Exif->Next;
    }
  }

  if (InteropCount > 0) {
    // write interop ifd
    Num = InteropCount;
    DataOffset += InteropDirSize;

    SIOwrite(io, &Num, 2, 1);
    Exif = Image->ExifTags;
    while (Exif) {
      if (Exif->IFD == IL_TIFF_IFD_INTEROP) {
        SIOwrite(io, &Exif->ID, 2, 1);
        SIOwrite(io, &Exif->Type, 2, 1);
        SIOwrite(io, &Exif->Length, 4, 1);

        if (Exif->Size > 4) {
          SIOwrite(io, &DataOffset, 4, 1);
          DataOffset += Exif->Size;
        } else {
          SIOwrite(io, Exif->Data, Exif->Size, 1);
          SIOpad(io, 4-Exif->Size);
        }
      } 
      Exif = Exif->Next;
    }
    TmpUint  = 0; SIOwrite(io, &TmpUint, 4, 1);

    Exif = Image->ExifTags;
    while (Exif) {
      if (Exif->IFD == IL_TIFF_IFD_INTEROP) {
        if (Exif->Size > 4) {
          SIOwrite(io, Exif->Data, Exif->Size, 1);
        }
      } 
      Exif = Exif->Next;
    }
  }
  return IL_TRUE;
}

static ILboolean iSaveExifInternal(ILimage *Image) {
  return iExifSave(Image);
}

ILconst_string iFormatExtsEXIF[] = { 
  IL_TEXT("exif"),
  NULL 
};

ILformat iFormatEXIF = { 
  /* .Validate = */ iIsValidExif, 
  /* .Load     = */ iLoadExifInternal, 
  /* .Save     = */ iSaveExifInternal, 
  /* .Exts     = */ iFormatExtsEXIF
};
