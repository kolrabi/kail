//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 02/14/2009
//
// Filename: src-IL/src/il_dicom.c
//
// Description: Reads from a Digital Imaging and Communications in Medicine
//        (DICOM) file.  Specifications can be found at 
//                http://en.wikipedia.org/wiki/Dicom.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_DICOM

#include "pack_push.h"
typedef struct DICOMHEAD
{
  ILubyte   Signature[4];
  ILuint    Version;
  ILuint    Width;
  ILuint    Height;
  ILuint    Depth;
  ILuint    Samples;
  ILuint    BitsAllocated;
  ILuint    BitsStored;
  ILuint    DataLen;
  ILboolean BigEndian;
  ILenum    Encoding;

  // For DevIL use only
  ILenum  Format;
  ILenum  Type;
} DICOMHEAD;
#include "pack_pop.h"

static ILboolean  SkipElement(SIO *io, DICOMHEAD *Header, ILushort GroupNum, ILushort ElementNum);
static ILboolean  GetNumericValue(SIO *io, DICOMHEAD *Header, ILushort GroupNum, void *Number);
static ILboolean  GetUID(SIO *io, ILubyte *UID, ILuint MaxLen);
static ILushort   GetGroupNum(SIO *io,DICOMHEAD *Header);
static ILushort   GetShort(SIO *io,DICOMHEAD *Header, ILushort GroupNum);
static ILuint     GetInt(SIO *io,DICOMHEAD *Header, ILushort GroupNum);
static ILfloat    GetFloat(SIO *io,DICOMHEAD *Header, ILushort GroupNum);


// Internal function used to get the DICOM header from the current file.
static ILboolean iGetDicomHead(SIO* io, DICOMHEAD *Header) {
  ILushort  GroupNum;
  ILushort  ElementNum;
  ILboolean ReachedData = IL_FALSE;
  ILubyte   Var2, UID[65];

  // Signature should be "DICM" at position 128.
  SIOseek(io, 128, IL_SEEK_SET);
  if (SIOread(io, Header->Signature, 1, 4) != 4)
    return IL_FALSE;

//@TODO: What about the case when we are reading an image with Big Endian data?

  do {
    GroupNum = GetGroupNum(io, Header);
    ElementNum = GetShort(io, Header, GroupNum);

    switch (GroupNum)
    {
      case 0x02:
        switch (ElementNum)
        {
          /*case 0x01:  // Version number
            if (!GetNumericValue(io, &Header->Version))
              return IL_FALSE;
            if (Header->Version != 0x0100)
              return IL_FALSE;
            break;*/

          case 0x10:
            //@TODO: Look at pg. 60 of 07_05pu.pdf (PS 3.5) for more UIDs.
            if (!GetUID(io, UID, 64))
              return IL_FALSE;
            if (!strncmp((const char*)UID, "1.2.840.10008.1.2.2", 64))  // Explicit big endian
              Header->BigEndian = IL_TRUE;
            else if (!strncmp((const char*)UID, "1.2.840.10008.1.2.1", 64))  // Explicit little endian
              Header->BigEndian = IL_FALSE;
            else if (!strncmp((const char*)UID, "1.2.840.10008.1.2", 64))  // Implicit little endian
              Header->BigEndian = IL_FALSE;
            else 
              return IL_FALSE;  // Unrecognized UID.
            break;

          default:
            if (!SkipElement(io, Header, GroupNum, ElementNum))  // We do not understand this entry, so we just skip it.
              return IL_FALSE;
        }
        break;

      case 0x28:
        switch (ElementNum)
        {
          case 0x02:  // Samples per pixel
            if (!GetNumericValue(io, Header, GroupNum, &Header->Samples))
              return IL_FALSE;
            break;

          case 0x08:  // Number of frames, or depth
            if (!GetNumericValue(io, Header, GroupNum, &Header->Depth))
              return IL_FALSE;
            break;

          case 0x10:  // The number of rows
            if (!GetNumericValue(io, Header, GroupNum, &Header->Height))
              return IL_FALSE;
            break;

          case 0x11:  // The number of columns
            if (!GetNumericValue(io, Header, GroupNum, &Header->Width))
              return IL_FALSE;
            break;

          case 0x100:  // Bits allocated per sample
            if (!GetNumericValue(io, Header, GroupNum, &Header->BitsAllocated))
              return IL_FALSE;
            break;

          case 0x101:  // Bits stored per sample - Do we really need this information?
            if (!GetNumericValue(io, Header, GroupNum, &Header->BitsStored))
              return IL_FALSE;
            break;

          default:
            if (!SkipElement(io, Header, GroupNum, ElementNum))  // We do not understand this entry, so we just skip it.
              return IL_FALSE;
        }
        break;

      case 0x7FE0:
        switch (ElementNum)
        {
          case 0x10:  // This element is the actual pixel data.  We are done with the header here.
            if (SIOgetc(io) != 'O')  // @TODO: Can we assume that this is always 'O'?
              return IL_FALSE;
            Var2 = (ILubyte)SIOgetc(io);
            if (Var2 != 'B' && Var2 != 'W' && Var2 != 'F')  // 'OB', 'OW' and 'OF' accepted for this element.
              return IL_FALSE;
            GetLittleUShort(io);  // Skip the 2 reserved bytes.
            Header->DataLen = GetInt(io, Header, GroupNum);//GetLittleUInt();
            ReachedData = IL_TRUE;
            break;
          default:
            if (!SkipElement(io, Header, GroupNum, ElementNum))  // We do not understand this entry, so we just skip it.
              return IL_FALSE;
        }
        break;

      case 0xFFFF:
        // read error
        return IL_FALSE;

      default:
        if (!SkipElement(io, Header, GroupNum, ElementNum))  // We do not understand this entry, so we just skip it.
          return IL_FALSE;
    }
  } while (!SIOeof(io) && !ReachedData);

  if (SIOeof(io))
    return IL_FALSE;

  // Some DICOM images do not have the depth (number of frames) field.
  if (Header->Depth == 0)
    Header->Depth = 1;

  switch (Header->BitsAllocated)
  {
    case 8:
      Header->Type = IL_UNSIGNED_BYTE;
      break;
    case 16:
      Header->Type = IL_UNSIGNED_SHORT;
      break;
    case 32:
      Header->Type = IL_FLOAT;  //@TODO: Is this ever an integer?
      break;
    default:  //@TODO: Any other types we can deal with?
      return IL_FALSE;
  }

  // Cannot handle more than 4 channels in an image.
  if (Header->Samples > 4)
    return IL_FALSE;
  Header->Format = iGetFormatBpp((ILubyte)Header->Samples);

  return IL_TRUE;
}

// Internal function used to check if the HEADER is a valid DICOM header.
static ILboolean iCheckDicom(DICOMHEAD *Header) {
  // Always has the signature "DICM" at position 0x80.
  if (strncmp((const char*)Header->Signature, "DICM", 4))
    return IL_FALSE;
  // Does not make sense to have any dimension = 0.
  if (Header->Width == 0 || Header->Height == 0 || Header->Depth == 0)
    return IL_FALSE;
  // We can only deal with images that have byte-aligned data.
  //@TODO: Take care of any others?
  if (Header->BitsAllocated % 8)
    return IL_FALSE;
  // Check for an invalid format being set (or not set at all).
  if (iGetBppFormat(Header->Format) == 0)
    return IL_FALSE;
  // Check for an invalid type being set (or not set at all).
  if (iGetBpcType(Header->Type) == 0)
    return IL_FALSE;
  return IL_TRUE;
}


// Internal function to get the header and check it.
static ILboolean iIsValidDicom(SIO* io) {
  char Sig[4];
  ILuint Start = SIOtell(io);
  ILuint Read;

  SIOseek(io, 128, IL_SEEK_CUR);
  Read = SIOread(io, Sig, 1, 4);
  SIOseek(io, Start, IL_SEEK_SET);

  return Read == 4 && memcmp(Sig, "DICM", 4) == 0;
}

static ILboolean SkipElement(SIO *io, DICOMHEAD *Header, ILushort GroupNum, ILushort ElementNum) {
  ILubyte VR1 = 0, VR2 = 0;
  ILuint  ValLen;

  // 2 byte character string telling what type this element is ('OB', 'UI', etc.)
  VR1 = (ILubyte)SIOgetc(io);
  VR2 = (ILubyte)SIOgetc(io);

  if (SIOeof(io)) {
    return IL_FALSE;
  }

  if ((VR1 == 'O' && VR2 == 'B') || (VR1 == 'O' && VR2 == 'W') || (VR1 == 'O' && VR2 == 'F') ||
      (VR1 == 'S' && VR2 == 'Q') || (VR1 == 'U' && VR2 == 'T') || (VR1 == 'U' && VR2 == 'N')) {
    // These all have a different format than the other formats, since they can be up to 32 bits long.
    GetLittleUShort(io);  // Values reserved, we do not care about them.
    ValLen = GetInt(io, Header, GroupNum);//GetLittleUInt();  // Length of the rest of the element
    if (ValLen % 2)  // This length must be even, according to the specs.
      return IL_FALSE;
    if (ElementNum != 0x00)  // Element numbers of 0 tell the size of the full group, so we do not skip this.
                 //  @TODO: We could use this to skip groups that we do not care about.
      if (SIOseek(io, ValLen, IL_SEEK_CUR))
        return IL_FALSE;
  }
  else {
    // These have a length of 16 bits.
    ValLen = GetShort(io, Header, GroupNum);//GetLittleUShort();
    //if (ValLen % 2)  // This length must be even, according to the specs.
    //  ValLen++;  // Add the extra byte to seek.
    //if (ElementNum != 0x00)  // Element numbers of 0 tell the size of the full group, so we do not skip this.
                 //  @TODO: We could use this to skip groups that we do not care about.
      if (SIOseek(io, ValLen, IL_SEEK_CUR))
        return IL_FALSE;
  }

  return IL_TRUE;
}


static ILushort GetGroupNum(SIO* io, DICOMHEAD *Header) {
  ILushort GroupNum;

  if (SIOread(io, &GroupNum, 1, 2) != 2) {
    return 0xFFFF;
  }

  // The 0x02 group is always little endian.
  if (GroupNum == 0x02) {
    UShort(&GroupNum);
    return GroupNum;
  }
  // Now we have to swizzle it if it is not 0x02.
  if (Header->BigEndian){
    BigUShort(&GroupNum);
  } else{
    UShort(&GroupNum);
  }

  return GroupNum;
}


static ILushort GetShort(SIO *io, DICOMHEAD *Header, ILushort GroupNum) {
  ILushort Num;

  if (SIOread(io, &Num, 1, 2) != 2) {
    return 0xFFFF;
  }

  // The 0x02 group is always little endian.
  if (GroupNum == 0x02) {
    UShort(&Num);
    return Num;
  }
  // Now we have to swizzle it if it is not 0x02.
  if (Header->BigEndian){
    BigUShort(&Num);
  } else {
    UShort(&Num);
  }

  return Num;
}


static ILuint GetInt(SIO *io, DICOMHEAD *Header, ILushort GroupNum) {
  ILuint Num;

  if (SIOread(io, &Num, 1, 4) != 4) {
    return ~0U;
  }

  // The 0x02 group is always little endian.
  if (GroupNum == 0x02) {
    UInt(&Num);
    return Num;
  }
  // Now we have to swizzle it if it is not 0x02.
  if (Header->BigEndian) {
    BigUInt(&Num);
  } else {
    UInt(&Num);
  }

  return Num;
}


static ILfloat GetFloat(SIO *io, DICOMHEAD *Header, ILushort GroupNum) {
  ILfloat Num;

  if (SIOread(io, &Num, 1, 4) != 4)
    return 0.0f;

  // The 0x02 group is always little endian.
  if (GroupNum == 0x02) {
    Float(&Num);
    return Num;
  }
  // Now we have to swizzle it if it is not 0x02.
  if (Header->BigEndian) {
    BigFloat(&Num);
  } else {
    Float(&Num);
  }

  return Num;
}


static ILboolean GetNumericValue(SIO *io, DICOMHEAD *Header, ILushort GroupNum, void *Number) {
  ILubyte   VR1, VR2;
  ILushort  ValLen;

  // 2 byte character string telling what type this element is ('OB', 'UI', etc.)
  VR1 = (ILubyte)SIOgetc(io);
  VR2 = (ILubyte)SIOgetc(io);

  if (SIOeof(io))
    return IL_FALSE;

  if (VR1 == 'U' && VR2 == 'S') {  // Unsigned short
    ValLen = (ILushort)GetShort(io, Header, GroupNum);//GetLittleUShort();
    if (ValLen != 2)  // Must always be 2 for short ('US')
      return IL_FALSE;
    *((ILushort*)Number) = GetShort(io, Header, GroupNum);//GetLittleUShort();
    return IL_TRUE;
  }
  if (VR1 == 'U' && VR2 == 'L') {  // Unsigned long
    ValLen = (ILushort)GetInt(io, Header, GroupNum);//GetLittleUInt();
    if (ValLen != 4)  // Must always be 4 for long ('UL')
      return IL_FALSE;
    *((ILuint*)Number) = GetInt(io, Header, GroupNum);
    return IL_TRUE;
  }
  if (VR1 == 'S' && VR2 == 'S') {  // Signed short
    ValLen = (ILushort)GetShort(io, Header, GroupNum);
    if (ValLen != 2)  // Must always be 2 for short ('US')
      return IL_FALSE;
    *((ILushort*)Number) = GetShort(io, Header, GroupNum);
    return IL_TRUE;
  }
  if (VR1 == 'S' && VR2 == 'L') {  // Signed long
    ValLen = (ILushort)GetInt(io, Header, GroupNum);
    if (ValLen != 4)  // Must always be 4 for long ('UL')
      return IL_FALSE;
    *((ILuint*)Number) = GetInt(io, Header, GroupNum);
    return IL_TRUE;
  }

  return IL_FALSE;
}


static ILboolean GetUID(SIO *io, ILubyte *UID, ILuint MaxLen) {
  ILubyte   VR1, VR2;
  ILushort  ValLen;

  // 2 byte character string telling what type this element is ('OB', 'UI', etc.)
  VR1 = (ILubyte)SIOgetc(io);
  VR2 = (ILubyte)SIOgetc(io);

  if (SIOeof(io)) 
    return IL_FALSE;

  if (VR1 != 'U' || VR2 != 'I')  // 'UI' == UID
    return IL_FALSE;

  ValLen = GetLittleUShort(io);
  if (ValLen > MaxLen) ValLen = MaxLen;

  if (SIOread(io, UID, ValLen, 1) != 1)
    return IL_FALSE;
  UID[64] = 0;  // Just to make sure that our string is terminated.

  return IL_TRUE;
}


// Internal function used to load the DICOM.
static ILboolean iLoadDicomInternal(ILimage* image) {
  DICOMHEAD Header;
  ILuint    i;
  ILushort  TempS, *ShortPtr;
  ILfloat   TempF, *FloatPtr;
  ILboolean Swizzle = IL_FALSE;
  SIO *     io;

  if (image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  io = &image->io;

  // Clear the header to all 0s to make checks later easier.
  memset(&Header, 0, sizeof(DICOMHEAD));

  if (!iGetDicomHead(io, &Header)) {
    iSetError(IL_INVALID_FILE_HEADER);
    return IL_FALSE;
  }

  if (!iCheckDicom(&Header))
    return IL_FALSE;

  if (!iTexImage(image, Header.Width, Header.Height, Header.Depth, iGetBppFormat(Header.Format), Header.Format, Header.Type, NULL))
    return IL_FALSE;
  //@TODO: Find out if the origin is always in the upper left.
  image->Origin = IL_ORIGIN_UPPER_LEFT;
  // Header.DataLen may be larger than SizeOfData, since it has to be padded with a NULL if it is not an even length,
  //   so we just test to make sure it is at least large enough.
  //@TODO: Do this check before iTexImage call.
  if (Header.DataLen < image->SizeOfData) {
    iSetError(IL_INVALID_FILE_HEADER);
    return IL_FALSE;
  }

  // We may have to swap the order of the data.
#ifdef WORDS_BIGENDIAN
      if (!Header.BigEndian) {
        if (Header.Format == IL_RGB)
          Header.Format = IL_BGR;
        else if (Header.Format == IL_RGBA)
          Swizzle = IL_TRUE;
      }
#else  // Little endian
      if (Header.BigEndian) {
        if (Header.Format == IL_RGB)
          Header.Format = IL_BGR;
        if (Header.Format == IL_RGBA)
          Swizzle = IL_TRUE;
      }
#endif

  switch (Header.Type)
  {
    case IL_UNSIGNED_BYTE:
      if (SIOread(io, image->Data, image->SizeOfData, 1) != 1)
        return IL_FALSE;

      // Swizzle the data from ABGR to RGBA.
      if (Swizzle) {
        ILuint *UIntPtr = (ILuint*)(void*)image->Data;
        for (i = 0; i < image->SizeOfData; i += 4) {
          iSwapUInt(UIntPtr + i);
        }
      }
      break;

    case IL_UNSIGNED_SHORT:
      ShortPtr = (ILushort*)(void*)image->Data;
      for (i = 0; i < image->SizeOfData; i += 2) {
        ShortPtr[i] = GetShort(io, &Header, 0);//GetLittleUShort();
      }

      // Swizzle the data from ABGR to RGBA.
      if (Swizzle) {
        for (i = 0; i < image->SizeOfData / 2; i += 4) {
          TempS = ShortPtr[i];
          ShortPtr[i  ] = ShortPtr[i+3];
          ShortPtr[i+3] = TempS;
        }
      }
      break;

    case IL_FLOAT:
      FloatPtr = (ILfloat*)(void*)image->Data;
      for (i = 0; i < image->SizeOfData; i += 4) {
        FloatPtr[i] = GetFloat(io, &Header, 0);//GetLittleFloat();
      }

      // Swizzle the data from ABGR to RGBA.
      if (Swizzle) {
        for (i = 0; i < image->SizeOfData / 4; i += 4) {
          TempF = FloatPtr[i];
          FloatPtr[i] = FloatPtr[i+3];
          FloatPtr[i+3] = TempF;
        }
      }
      break;
  }

  return IL_TRUE;
}

static ILconst_string iFormatExtsDICOM[] = { 
  IL_TEXT("dcm"), 
  IL_TEXT("dicom"), 
  NULL 
};

ILformat iFormatDICOM = { 
  /* .Validate = */ iIsValidDicom, 
  /* .Load     = */ iLoadDicomInternal, 
  /* .Save     = */ NULL, 
  /* .Exts     = */ iFormatExtsDICOM
};


#endif//IL_NO_DICOM
