#include "il_internal.h"

#include "pack_push.h"
typedef struct {
  ILushort Tag;
  ILushort Type;
  ILuint   Length;
  ILuint   Offset;
} ExifDirEntry;
#include "pack_pop.h"

ILuint iExifGetEntrySize(const ExifDirEntry *Entry) {
  ILuint Size = Entry->Length;
  switch (Entry->Type) {
    case IL_EXIF_TYPE_BYTE:     Size = Entry->Length; break;
    case IL_EXIF_TYPE_ASCII:    Size = Entry->Length; break;
    case IL_EXIF_TYPE_WORD:     Size = Entry->Length * 2; break;
    case IL_EXIF_TYPE_DWORD:    Size = Entry->Length * 4; break;
    case IL_EXIF_TYPE_RATIONAL: Size = Entry->Length * 8; break;
  }
  return Size;
}

ILboolean iExifReadDirEntry(SIO *io, ExifDirEntry *Entry, ILboolean BigEndian) {
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

ILuint iExifReadDir(ILimage *Image, ILuint Start, ILuint Offset, ILenum IFD, ILboolean BigEndian) {
  SIO *io = &Image->io;
  ILushort Num;
  ILint i;

  SIOseek(io, Offset + Start, IL_SEEK_SET);

  if (SIOread(io, &Num, 1, 2) != 2) return 0;
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

    if (Size <= 4) {
      Entry.Offset = SIOtell(io) - Start - 4;
    }

    if (Entry.Tag == IL_TIFF_IFD_EXIF || Entry.Tag == IL_TIFF_IFD_GPS || Entry.Tag == IL_TIFF_IFD_INTEROP) {
      ILuint Back = SIOtell(io);
      Offset = Entry.Offset;
      while(Offset) {
        Offset = iExifReadDir(Image, Start, Offset, Entry.Tag, BigEndian);
      }
      SIOseek(io, Back, IL_SEEK_SET);
    } else {
      // TODO: Interoperability IFD, GPS IFD
      ILuint Back = SIOtell(io);
      ILexif *Exif = ioalloc(ILexif);
      if (!Exif) return 0;

      Exif->Data = ialloc(Size);
      if (!Exif->Data) {
        ifree(Exif);
        return 0;
      }

      Exif->IFD = IFD;
      Exif->ID  = Entry.Tag;
      Exif->Type = Entry.Type;
      Exif->Size = Size;
      Exif->Next = Image->ExifTags;

      SIOseek(io, Start + Entry.Offset, IL_SEEK_SET);
      if (SIOread(io, Exif->Data, 1, Size) != Size) {
        ifree(Exif->Data);
        ifree(Exif);
        return 0;
      }

      Image->ExifTags = Exif;
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

  ILubyte hdr[8];
  ILuint  Offset;
  ILexif *Exif;
  ILboolean BigEndian;

  // clear old exif data first
  Exif = Image->ExifTags;
  while(Exif) {
    ILexif *NextExif = Exif->Next;
    ifree(Exif->Data);
    ifree(Exif);
    Exif = NextExif;
  }
  Image->ExifTags = NULL;

  if (SIOread(io, hdr, 1, 4) != 4) return IL_FALSE;
  if (memcmp(hdr, "MM\0\x2a", 4) && memcmp(hdr, "II\x2a\0", 4)) return IL_FALSE;

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
