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
    case IL_EXIF_TYPE_DWORD:
    case IL_EXIF_TYPE_IFD:       Size = Entry->Length * 4; break;
    case IL_EXIF_TYPE_RATIONAL:  Size = Entry->Length * 8; break;
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
      ILuint Back = SIOtell(io);
      Offset = Entry.Offset;
      while(Offset) {
        Offset = iExifReadDir(Image, Start, Offset, Entry.Tag, BigEndian);
      }
      SIOseek(io, Back, IL_SEEK_SET);
    } else {
      ILuint Back = SIOtell(io);
      void *Data = ialloc(Size + 1);
      if (!Data) {
        return 0;
      }

      if (Size <= 4) {
        Entry.Offset = SIOtell(io) - Start - 4;
      }

      SIOseek(io, Start + Entry.Offset, IL_SEEK_SET);
      if (SIOread(io, Data, 1, Size) != Size) {
        ifree(Data);
        return 0;
      }

      if (BigEndian) {
        switch(Entry.Type) {
          case IL_EXIF_TYPE_WORD:       BigUShort(Data); break;          
          case IL_EXIF_TYPE_DWORD:      BigUInt(Data); break;          
          case IL_EXIF_TYPE_RATIONAL:   BigUInt(Data); BigUInt(((ILuint*)Data)+1); break;
          case IL_EXIF_TYPE_SWORD:      BigShort(Data); break;          
          case IL_EXIF_TYPE_SDWORD:     BigInt(Data); break;          
          case IL_EXIF_TYPE_SRATIONAL:  BigInt(Data); BigInt(((ILint*)Data)+1); break;
          case IL_EXIF_TYPE_FLOAT:      BigFloat(Data); break;          
          case IL_EXIF_TYPE_DOUBLE:     BigDouble(Data); break;          
        }
      } else {
        switch(Entry.Type) {
          case IL_EXIF_TYPE_WORD:       UShort(Data); break;          
          case IL_EXIF_TYPE_DWORD:      UInt(Data); break;          
          case IL_EXIF_TYPE_RATIONAL:   UInt(Data); UInt(((ILuint*)Data)+1); break;
          case IL_EXIF_TYPE_SWORD:      Short(Data); break;          
          case IL_EXIF_TYPE_SDWORD:     Int(Data); break;          
          case IL_EXIF_TYPE_SRATIONAL:  Int(Data); Int(((ILint*)Data)+1); break;
          case IL_EXIF_TYPE_FLOAT:      Float(Data); break;          
          case IL_EXIF_TYPE_DOUBLE:     Double(Data); break;          
        }
      }

      ((ILubyte*)Data)[Size] = 0;

      iSetMetadata(Image, IFD, Entry.Tag, Entry.Type, Entry.Length, Size, Data);
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
  ILmeta *Meta;
  ILboolean BigEndian;
  ILuint ifd = 0;

  if (SIOread(io, hdr, 1, 4) != 4) return IL_FALSE;
  if (memcmp(hdr, "MM\0\x2a", 4) && memcmp(hdr, "II\x2a\0", 4)) return IL_FALSE;

  // clear old exif data first
  Meta = Image->MetaTags;
  while(Meta) {
    ILmeta *NextMeta = Meta->Next;
    ifree(Meta->Data);
    ifree(Meta->String);
    ifree(Meta);
    Meta = NextMeta;
  }
  Image->MetaTags = NULL;

  BigEndian = (memcmp(hdr, "MM\0\x2a", 4) == 0);

  if (SIOread(io, &Offset, 1, 4) != 4) return IL_FALSE;
  if (BigEndian) {
    BigUInt(&Offset);
  } else {
    UInt(&Offset);
  }

  while(Offset) {
    Offset = iExifReadDir(Image, Start, Offset, ifd++, BigEndian);
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
  ILmeta *  Meta          = Image->MetaTags;
  ILushort Num, TmpShort;

  ILuint    DataOffset = 0, TmpUint;

  // find out directory sizes
  Meta = Image->MetaTags;
  while (Meta) {
    if (Meta->IFD == IL_TIFF_IFD0) {
      IFD0Count ++;
      IFD0DirSize += 12;
      if (Meta->Size > 4) IFD0DataSize += Meta->Size;
    } else if (Meta->IFD == IL_TIFF_IFD_EXIF) {
      ExifCount ++;
      ExifDirSize += 12;
      if (Meta->Size > 4) ExifDataSize += Meta->Size;
    } else if (Meta->IFD == IL_TIFF_IFD_GPS) {
      GPSCount ++;
      GPSDirSize += 12;
      if (Meta->Size > 4) GPSDataSize += Meta->Size;
    } else if (Meta->IFD == IL_TIFF_IFD_INTEROP) {
      InteropCount ++;
      InteropDirSize += 12;
      if (Meta->Size > 4) InteropDataSize += Meta->Size;
    }
    Meta = Meta->Next;
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
  Meta = Image->MetaTags;
  while (Meta) {
    if (Meta->IFD == IL_TIFF_IFD0) {
      SIOwrite(io, &Meta->ID, 2, 1);
      SIOwrite(io, &Meta->Type, 2, 1);
      SIOwrite(io, &Meta->Length, 4, 1);

      if (Meta->Size > 4) {
        SIOwrite(io, &DataOffset, 4, 1);
        DataOffset += Meta->Size;
      } else {
        SIOwrite(io, Meta->Data, Meta->Size, 1);
        SIOpad(io, 4-Meta->Size);
      }
    } 
    Meta = Meta->Next;
  }

  if (ExifCount > 0) {
    TmpShort = IL_TIFF_IFD_EXIF;    SIOwrite(io, &TmpShort, 2, 1);
    TmpShort = IL_EXIF_TYPE_IFD;  SIOwrite(io, &TmpShort, 2, 1);
    TmpUint  = 1;                   SIOwrite(io, &TmpUint, 4, 1);
    TmpUint  = DataOffset;      SIOwrite(io, &TmpUint, 4, 1);
  }

  if (GPSCount > 0) {
    TmpShort = IL_TIFF_IFD_GPS;     SIOwrite(io, &TmpShort, 2, 1);
    TmpShort = IL_EXIF_TYPE_IFD;  SIOwrite(io, &TmpShort, 2, 1);
    TmpUint  = 1;                   SIOwrite(io, &TmpUint, 4, 1);
    TmpUint  = DataOffset + ExifDataSize + ExifDirSize;   SIOwrite(io, &TmpUint, 4, 1);
  }

  if (InteropCount) {
    TmpShort = IL_TIFF_IFD_INTEROP;     SIOwrite(io, &TmpShort, 2, 1);
    TmpShort = IL_EXIF_TYPE_IFD;      SIOwrite(io, &TmpShort, 2, 1);
    TmpUint  = 1;                       SIOwrite(io, &TmpUint, 4, 1);
    TmpUint  = DataOffset + ExifDataSize + ExifDirSize + 
               GPSDataSize + GPSDirSize;   SIOwrite(io, &TmpUint, 4, 1);
  }
  TmpUint  = 0; SIOwrite(io, &TmpUint, 4, 1);

  Meta = Image->MetaTags;
  while (Meta) {
    if (Meta->IFD == IL_TIFF_IFD0) {
      if (Meta->Size > 4) {
        SIOwrite(io, Meta->Data, Meta->Size, 1);
      }
    } 
    Meta = Meta->Next;
  }

  if (ExifCount > 0) {
    // write exif ifd
    Num = ExifCount;
    DataOffset += ExifDirSize;

    SIOwrite(io, &Num, 2, 1);
    Meta = Image->MetaTags;
    while (Meta) {
      if (Meta->IFD == IL_TIFF_IFD_EXIF) {
        SIOwrite(io, &Meta->ID, 2, 1);
        SIOwrite(io, &Meta->Type, 2, 1);
        SIOwrite(io, &Meta->Length, 4, 1);

        if (Meta->Size > 4) {
          SIOwrite(io, &DataOffset, 4, 1);
          DataOffset += Meta->Size;
        } else {
          SIOwrite(io, Meta->Data, Meta->Size, 1);
          SIOpad(io, 4-Meta->Size);
        }
      } 
      Meta = Meta->Next;
    }
    TmpUint  = 0; SIOwrite(io, &TmpUint, 4, 1);
    Meta = Image->MetaTags;
    while (Meta) {
      if (Meta->IFD == IL_TIFF_IFD_EXIF) {
        if (Meta->Size > 4) {
          SIOwrite(io, Meta->Data, Meta->Size, 1);
        }
      } 
      Meta = Meta->Next;
    }
  }

  if (GPSCount > 0) {
    // write gps ifd
    Num = GPSCount;
    DataOffset += GPSDirSize;

    SIOwrite(io, &Num, 2, 1);
    Meta = Image->MetaTags;
    while (Meta) {
      if (Meta->IFD == IL_TIFF_IFD_GPS) {
        SIOwrite(io, &Meta->ID, 2, 1);
        SIOwrite(io, &Meta->Type, 2, 1);
        SIOwrite(io, &Meta->Length, 4, 1);

        if (Meta->Size > 4) {
          SIOwrite(io, &DataOffset, 4, 1);
          DataOffset += Meta->Size;
        } else {
          SIOwrite(io, Meta->Data, Meta->Size, 1);
          SIOpad(io, 4-Meta->Size);
        }
      } 
      Meta = Meta->Next;
    }
    TmpUint  = 0; SIOwrite(io, &TmpUint, 4, 1);

    Meta = Image->MetaTags;
    while (Meta) {
      if (Meta->IFD == IL_TIFF_IFD_GPS) {
        if (Meta->Size > 4) {
          SIOwrite(io, Meta->Data, Meta->Size, 1);
        }
      } 
      Meta = Meta->Next;
    }
  }

  if (InteropCount > 0) {
    // write interop ifd
    Num = InteropCount;
    DataOffset += InteropDirSize;

    SIOwrite(io, &Num, 2, 1);
    Meta = Image->MetaTags;
    while (Meta) {
      if (Meta->IFD == IL_TIFF_IFD_INTEROP) {
        SIOwrite(io, &Meta->ID, 2, 1);
        SIOwrite(io, &Meta->Type, 2, 1);
        SIOwrite(io, &Meta->Length, 4, 1);

        if (Meta->Size > 4) {
          SIOwrite(io, &DataOffset, 4, 1);
          DataOffset += Meta->Size;
        } else {
          SIOwrite(io, Meta->Data, Meta->Size, 1);
          SIOpad(io, 4-Meta->Size);
        }
      } 
      Meta = Meta->Next;
    }
    TmpUint  = 0; SIOwrite(io, &TmpUint, 4, 1);

    Meta = Image->MetaTags;
    while (Meta) {
      if (Meta->IFD == IL_TIFF_IFD_INTEROP) {
        if (Meta->Size > 4) {
          SIOwrite(io, Meta->Data, Meta->Size, 1);
        }
      } 
      Meta = Meta->Next;
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
