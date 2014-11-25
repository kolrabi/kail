//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_utx.cpp
//
// Description: Reads from an Unreal and Unreal Tournament Texture (.utx) file.
//        Specifications can be found at
//        http://wiki.beyondunreal.com/Legacy:Package_File_Format.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_UTX
#include "il_utx.h"

static ILboolean iLoadUtxInternal(ILimage *);

static ILboolean GetUtxHead(SIO *io, UTXHEADER *Header) {
  if (SIOread(io, Header, sizeof(*Header), 1) != 1)
    return IL_FALSE;

  UInt  (&Header->Signature);
  UShort(&Header->Version);
  UShort(&Header->LicenseMode);
  UInt  (&Header->Flags);
  UInt  (&Header->NameCount);
  UInt  (&Header->NameOffset);
  UInt  (&Header->ExportCount);
  UInt  (&Header->ExportOffset);
  UInt  (&Header->ImportCount);
  UInt  (&Header->ImportOffset);

  return IL_TRUE;
}

static ILboolean CheckUtxHead(const UTXHEADER *Header) {
  // This signature signifies a UTX file.
  if (Header->Signature != 0x9E2A83C1)
    return IL_FALSE;
  // Unreal uses 61-63, and Unreal Tournament uses 67-69.
  if ((Header->Version < 61 || Header->Version > 69))
    return IL_FALSE;
  return IL_TRUE;
}

static ILboolean iIsValidUtx(SIO *io) {
  ILuint    Start = SIOtell(io);
  UTXHEADER Head;
  ILboolean HasHead = GetUtxHead(io, &Head);

  SIOseek(io, Start, IL_SEEK_SET);

  return HasHead && CheckUtxHead(&Head);
}

// Gets a name variable from the file.  Keep in mind that the return value must be freed.
static char *GetUtxName(SIO *io, UTXHEADER *Header)
{
  char *  Name = NULL;
  ILubyte Length = 0;
    ILuint Pos;

  // New style (Unreal Tournament) name.  This has a byte at the beginning telling
  //  how long the string is (plus terminating 0), followed by the terminating 0. 
  if (Header->Version >= 64) {
    Length        = (ILubyte)SIOgetc(io);
    Name          = (char*)ialloc(Length+1);
    Name[0]       = 0;
    Name[Length]  = 0;

    if (SIOread(io, Name, Length, 1) != 1) {
      ifree(Name);
      return NULL;
    }

    return Name;
  }

  // Old style (Unreal) name.  This string length is unknown, but it is terminated
  //  by a 0.
  Pos = SIOtell(io);

  while (!SIOeof(io) && SIOgetc(io) > 0)
    Length++;

  Name          = (char*)ialloc(Length+1);
  Name[0]       = 0;
  Name[Length]  = 0;

  SIOseek(io, Pos, IL_SEEK_SET);
  if (SIOread(io, Name, Length, 1) != 1) {
    ifree(Name);
    return NULL;
  }

  // skip terminating 0
  SIOgetc(io);
  return Name;
}

static ILboolean GetUtxNameTable(SIO *io, UTXENTRYNAME *NameEntries, UTXHEADER *Header) {
  ILuint  NumRead;

  // Go to the name table.
  SIOseek(io, Header->NameOffset, IL_SEEK_SET);

  // Read in the name table.
  for (NumRead = 0; NumRead < Header->NameCount; NumRead++) {
    NameEntries[NumRead].Name = GetUtxName(io, Header);
    if (!NameEntries[NumRead].Name)
      break;
    NameEntries[NumRead].Flags = GetLittleUInt(io);
  }

  // Did not read all of the entries (most likely GetUtxName failed).
  if (NumRead < Header->NameCount) {
    iSetError(IL_INVALID_FILE_HEADER);
    return IL_FALSE;
  }

  return IL_TRUE;
}


// This following code is from http://wiki.beyondunreal.com/Legacy:Package_File_Format/Data_Details.
/// <summary>Reads a compact integer from the FileReader.
/// Bytes read differs, so do not make assumptions about
/// physical data being read from the stream. (If you have
/// to, get the difference of FileReader.BaseStream.Position
/// before and after this is executed.)</summary>
/// <returns>An "uncompacted" signed integer.</returns>
/// <remarks>FileReader is a System.IO.BinaryReader mapped
/// to a file. Also, there may be better ways to implement
/// this, but this is fast, and it works.</remarks>
static ILint UtxReadCompactInteger(SIO *io) {
  int output = 0;
  ILboolean sign = IL_FALSE;
  int i;
  ILubyte x;

  for(i = 0; i < 5; i++)
  {
    x = (ILubyte)SIOgetc(io);
    // First byte
    if(i == 0)
    {
      // Bit: X0000000
      if((x & 0x80) > 0)
              sign = IL_TRUE;
      // Bits: 00XXXXXX
      output |= (x & 0x3F);
      // Bit: 0X000000
      if((x & 0x40) == 0)
              break;
    }
    // Last byte
    else if(i == 4)
    {
      // Bits: 000XXXXX -- the 0 bits are ignored
      // (hits the 32 bit boundary)
      output |= (x & 0x1F) << (6 + (3 * 7));
    }
    // Middle bytes
    else
    {
      // Bits: 0XXXXXXX
      output |= (x & 0x7F) << (6 + ((i - 1) * 7));
      // Bit: X0000000
      if((x & 0x80) == 0)
              break;
    }
  }
  // multiply by negative one here, since the first 6+ bits could be 0
  if (sign)
          output *= -1;
  return output;
}

static void ChangeObjectReference(ILint *ObjRef, ILboolean *IsImported) {
  if (*ObjRef < 0) {
    *IsImported = IL_TRUE;
    *ObjRef = -*ObjRef - 1;
  }
  else if (*ObjRef > 0) {
    *IsImported = IL_FALSE;
    *ObjRef = *ObjRef - 1;  // This is an object reference, so we have to do this conversion.
  }
  else {
    *ObjRef = -1;  // "NULL" pointer
  }
}

static ILboolean GetUtxExportTable(SIO *io, UTXEXPORTTABLE *ExportTable, UTXHEADER *Header) {
  ILuint i;

  // Go to the name table.
  SIOseek(io, Header->ExportOffset, IL_SEEK_SET);

  for (i = 0; i < Header->ExportCount; i++) {
    ExportTable[i].Class        = UtxReadCompactInteger(io);
    ExportTable[i].Super        = UtxReadCompactInteger(io);
    ExportTable[i].Group        = GetLittleInt(io);
    ExportTable[i].ObjectName   = UtxReadCompactInteger(io);
    ExportTable[i].ObjectFlags  = GetLittleUInt(io);
    ExportTable[i].SerialSize   = UtxReadCompactInteger(io);
    ExportTable[i].SerialOffset = UtxReadCompactInteger(io);

    ChangeObjectReference(&ExportTable[i].Class, &ExportTable[i].ClassImported);
    ChangeObjectReference(&ExportTable[i].Super, &ExportTable[i].SuperImported);
    ChangeObjectReference(&ExportTable[i].Group, &ExportTable[i].GroupImported);
  }

  return IL_TRUE;
}

static ILboolean GetUtxImportTable(SIO *io, UTXIMPORTTABLE *ImportTable, UTXHEADER *Header)
{
  ILuint i;

  // Go to the name table.
  SIOseek(io, Header->ImportOffset, IL_SEEK_SET);

  for (i = 0; i < Header->ImportCount; i++) {
    ImportTable[i].ClassPackage = UtxReadCompactInteger(io);
    ImportTable[i].ClassName    = UtxReadCompactInteger(io);
    ImportTable[i].Package      = GetLittleInt(io);
    ImportTable[i].ObjectName   = UtxReadCompactInteger(io);

    ChangeObjectReference(&ImportTable[i].Package, &ImportTable[i].PackageImported);
  }

  return IL_TRUE;
}

static ILenum UtxFormatToDevIL(ILint Format) {
  switch (Format)
  {
    case UTX_P8:      return IL_COLOR_INDEX;
    case UTX_DXT1:    return IL_RGBA;
  }
  return IL_BGRA;  // Should never reach here.
}

static ILubyte UtxFormatToBpp(ILint Format) {
  switch (Format)
  {
    case UTX_P8:      return 1;
    case UTX_DXT1:    return 4;
  }
  return 4;  // Should never reach here.
}

// Internal function used to load the UTX.
static ILboolean iLoadUtxInternal(ILimage *image) {
  UTXHEADER   Header;
  UTXENTRYNAME *NameEntries = NULL;
  UTXEXPORTTABLE *ExportTable = NULL;
  UTXIMPORTTABLE *ImportTable = NULL;
  UTXPALETTE * Palettes = NULL;
  ILimage   *Image = image;
  ILuint    NumPal = 0, PalEntry, i;
  ILint   Name;
  ILubyte   Type;
  ILint   Val;
  ILint   Size;
  ILint   Width, Height;
  ILboolean BaseCreated = IL_FALSE, HasPal;
  ILint   Format;
  ILubyte   *CompData = NULL;
  SIO *io;

  if (image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  io = &image->io;

  if (!GetUtxHead(io, &Header))
    return IL_FALSE;

  if (!CheckUtxHead(&Header))
    return IL_FALSE;

  NameEntries = (UTXENTRYNAME*)   ialloc(sizeof(UTXENTRYNAME)   * Header.NameCount);
  ExportTable = (UTXEXPORTTABLE*) ialloc(sizeof(UTXEXPORTTABLE) * Header.ExportCount);
  ImportTable = (UTXIMPORTTABLE*) ialloc(sizeof(UTXIMPORTTABLE) * Header.ImportCount);

  if (!NameEntries || !ExportTable || !ImportTable)
    goto error;

  memset(NameEntries, 0, sizeof(UTXENTRYNAME)   * Header.NameCount);
  memset(ExportTable, 0, sizeof(UTXEXPORTTABLE) * Header.ExportCount);
  memset(ImportTable, 0, sizeof(UTXIMPORTTABLE) * Header.ImportCount);

  // We grab the name table.
  if (!GetUtxNameTable(io, NameEntries, &Header))
    goto error;

  // Then we get the export table.
  if (!GetUtxExportTable(io, ExportTable, &Header))
    goto error;

  // Then the last table is the import table.
  if (!GetUtxImportTable(io, ImportTable, &Header))
    goto error;

  // Find the number of palettes in the export table.
  for (i = 0; i < Header.ExportCount; i++) {
    if (!strcmp(NameEntries[ImportTable[ExportTable[i].Class].ObjectName].Name, "Palette"))
      NumPal++;
  }

  Palettes = (UTXPALETTE*)ialloc(sizeof(UTXPALETTE) * NumPal);
  if (!Palettes)
    goto error;

  memset(Palettes, 0, sizeof(UTXPALETTE) * NumPal);

  // Read in all of the palettes.
  NumPal = 0;
  for (i = 0; i < Header.ExportCount; i++) {
    if (!strcmp(NameEntries[ImportTable[ExportTable[i].Class].ObjectName].Name, "Palette")) {
      Palettes[NumPal].Name = (ILint)i;
      SIOseek(io, ExportTable[NumPal].SerialOffset, IL_SEEK_SET);
      Name = SIOgetc(io);  // Skip the 2.  @TODO: Can there be more in front of the palettes?
      Palettes[NumPal].Count = (ILuint)UtxReadCompactInteger(io);
      Palettes[NumPal].Pal   = (ILubyte*)ialloc(Palettes[NumPal].Count * 4);
      if (SIOread(io, Palettes[NumPal].Pal, Palettes[NumPal].Count * 4, 1) != 1)
        goto error;
      NumPal++;
    }
  }

  for (i = 0; i < Header.ExportCount; i++) {
    // Find textures in the file.
    if (!strcmp(NameEntries[ImportTable[ExportTable[i].Class].ObjectName].Name, "Texture")) {
      SIOseek(io, ExportTable[i].SerialOffset, IL_SEEK_SET);
      Width = -1;  
      Height = -1; 
      PalEntry = NumPal;
      HasPal = IL_FALSE;
      Format = -1;

      do {
        // Always starts with a compact integer that gives us an entry into the name table.
        Name = UtxReadCompactInteger(io);
        if (!strcmp(NameEntries[Name].Name, "None"))
          break;

        Type = (ILubyte)SIOgetc(io);
        Size = (Type & 0x70) >> 4;

        if (Type == 0xA2)
          SIOgetc(io);  // Byte is 1 here...

        Val = 0;
        switch (Type & 0x0F)
        {
          case 1:  // Get a single byte.
            Val = SIOgetc(io);
            break;

          case 2:  // Get integer.
            Val = GetLittleInt(io);
            break;

          case 3:  // Boolean value is in the info byte.
            SIOgetc(io);
            break;

          case 4:  // Skip flaots for right now.
            GetLittleFloat(io);
            break;

          case 5:
          case 6:  // Get a compact integer - an object reference.
            Val = UtxReadCompactInteger(io);
            Val--;
            break;

          case 10:
            Val = SIOgetc(io);
            switch (Size)
            {
              case 0:
                SIOseek(io, 1, IL_SEEK_CUR);
                break;
              case 1:
                SIOseek(io, 2, IL_SEEK_CUR);
                break;
              case 2:
                SIOseek(io, 4, IL_SEEK_CUR);
                break;
              case 3:
                SIOseek(io, 12, IL_SEEK_CUR);
                break;
            }
            break;

          default:  // Uhm...
            goto error;
        }

        //@TODO: What should we do if Name >= Header.NameCount?
        if ((ILuint)Name < Header.NameCount) {  // Don't want to go past the end of our array.
          if (!strcmp(NameEntries[Name].Name, "Palette")) {
            // If it has references to more than one palette, just use the first one.
            if (HasPal == IL_FALSE) {
              // We go through the palette list here to match names.
              for (PalEntry = 0; (ILuint)PalEntry < NumPal; PalEntry++) {
                if (Val == Palettes[PalEntry].Name) {
                  HasPal = IL_TRUE;
                  break;
                }
              }
            }
          }
          if (!strcmp(NameEntries[Name].Name, "Format")) // Not required for P8 images but can be present.
            if (Format == -1)
              Format = Val;

          if (!strcmp(NameEntries[Name].Name, "USize"))  // Width of the image
            if (Width == -1)
              Width = Val;

          if (!strcmp(NameEntries[Name].Name, "VSize"))  // Height of the image
            if (Height == -1)
              Height = Val;
        }
      } while (!SIOeof(io));

      // If the format property is not present, it is a paletted (P8) image.
      if (Format == -1)
        Format = UTX_P8;
      // Just checks for everything being proper.  If the format is P8, we check to make sure that a palette was found.
      if (Width == -1 || Height == -1 || (PalEntry == NumPal && Format != UTX_DXT1) || (Format != UTX_P8 && Format != UTX_DXT1))
        return IL_FALSE;
      if (BaseCreated == IL_FALSE) {
        BaseCreated = IL_TRUE;
        iTexImage(image, (ILuint)Width, (ILuint)Height, 1, UtxFormatToBpp(Format), UtxFormatToDevIL(Format), IL_UNSIGNED_BYTE, NULL);
        Image = image;
      }
      else {
        Image->Next = iNewImageFull((ILuint)Width, (ILuint)Height, 1, UtxFormatToBpp(Format), UtxFormatToDevIL(Format), IL_UNSIGNED_BYTE, NULL);
        if (Image->Next == NULL)
          return IL_FALSE;
        Image = Image->Next;
      }

      // Skip the mipmap count, width offset and mipmap size entries.
      //@TODO: Implement mipmaps.
      SIOseek(io, 5, IL_SEEK_CUR);
      UtxReadCompactInteger(io);

      switch (Format)
      {
        case UTX_P8:
          Image->Pal.PalSize = Palettes[PalEntry].Count * 4;
          Image->Pal.Palette = (ILubyte*)ialloc(Image->Pal.PalSize);
          if (Image->Pal.Palette == NULL)
            return IL_FALSE;
          memcpy(Image->Pal.Palette, Palettes[PalEntry].Pal, Image->Pal.PalSize);
          Image->Pal.PalType = IL_PAL_RGBA32;

          if (SIOread(io, Image->Data, Image->SizeOfData, 1) != 1)
            return IL_FALSE;
          break;

        case UTX_DXT1:
          Image->DxtcSize = IL_MAX(Image->Width * Image->Height / 2, 8);
          CompData = (ILubyte*)ialloc(Image->DxtcSize);
          if (CompData == NULL)
            return IL_FALSE;

          if (SIOread(io, CompData, Image->DxtcSize, 1) != 1) {
            ifree(CompData);
            return IL_FALSE;
          }

          // Keep a copy of the DXTC data if the user wants it.
          if (iIsEnabled(IL_KEEP_DXTC_DATA)) {
            Image->DxtcData = CompData;
            Image->DxtcFormat = IL_DXT1;
            CompData = NULL;
          }
          if (DecompressDXT1(Image, CompData, Image->DxtcSize) == IL_FALSE) {
            ifree(CompData);
            return IL_FALSE;
          }
          ifree(CompData);
          break;
      }
      Image->Origin = IL_ORIGIN_UPPER_LEFT;
    }
  }

  if (NameEntries) {
    for (i=0; i<Header.NameCount; i++)
      ifree(NameEntries[i].Name);
  }

  if (Palettes) {
    for (i=0; i<NumPal; i++)
      ifree(Palettes[i].Pal);
  }

  ifree(NameEntries);
  ifree(ImportTable);
  ifree(ExportTable);
  ifree(Palettes);
  return IL_TRUE;

error:

  if (NameEntries) {
    for (i=0; i<Header.NameCount; i++)
      ifree(NameEntries[i].Name);
  }

  if (Palettes) {
    for (i=0; i<NumPal; i++)
      ifree(Palettes[i].Pal);
  }

  ifree(NameEntries);
  ifree(ImportTable);
  ifree(ExportTable);
  ifree(Palettes);
  return IL_FALSE;  
}

static ILconst_string iFormatExtsUTX[] = { 
  IL_TEXT("utx"), 
  NULL 
};

ILformat iFormatUTX = { 
  /* .Validate = */ iIsValidUtx, 
  /* .Load     = */ iLoadUtxInternal, 
  /* .Save     = */ NULL, 
  /* .Exts     = */ iFormatExtsUTX
};

#endif//IL_NO_UTX

