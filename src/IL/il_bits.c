//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 08/14/2004
//
// Filename: src-IL/src/il_bits.c
//
// Description: Implements a file class that reads/writes bits directly.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_bits.h"

BITFILE *bitfile(SIO *io) {
  BITFILE *ToReturn = NULL;

  if (io != NULL && io->handle) {
    ToReturn = ioalloc(BITFILE);
    if (ToReturn != NULL) {
      ToReturn->io          = io;
      ToReturn->BitPos      = SIOtell(io) << 3;
      ToReturn->ByteBitOff  = 8;
      ToReturn->Buff        = 0;
    }
  }

  return ToReturn;
}


// Closes an open BITFILE and frees memory for it.
ILint bclose(BITFILE *BitFile) {
  if (BitFile == NULL || BitFile->io == NULL)
    return IL_EOF;

  ifree(BitFile);
  return 0;
}

// Returns the current bit position of a BITFILE.
ILuint btell(BITFILE *BitFile) {
  return BitFile->BitPos;
}

// Seeks in a BITFILE just like fseek for FILE.
ILint bseek(BITFILE *BitFile, ILint Offset, ILuint Mode) {
  ILuint Len;

  if (BitFile == NULL || BitFile->io == NULL)
    return 1;

  switch (Mode) {
    case IL_SEEK_SET:
      if (!SIOseek(BitFile->io, Offset >> 3, Mode)) {
        BitFile->BitPos     = (ILuint)Offset;
        BitFile->ByteBitOff = BitFile->BitPos % 8;
      }
      break;
    case IL_SEEK_CUR:
      if (!SIOseek(BitFile->io, Offset >> 3, Mode)) {
        BitFile->BitPos     = (ILuint)((ILint)BitFile->BitPos+Offset);
        BitFile->ByteBitOff = BitFile->BitPos % 8;
      }
      break;
    case IL_SEEK_END:
      SIOseek(BitFile->io, 0, IL_SEEK_END);
      Len = SIOtell(BitFile->io);
      SIOseek(BitFile->io, 0, IL_SEEK_SET);

      if (!SIOseek(BitFile->io, Offset >> 3, Mode)) {
        BitFile->BitPos     = (ILuint)((ILint)(Len << 3) + Offset);
        BitFile->ByteBitOff = BitFile->BitPos % 8;
      }
      break;

    default:
      return 1;
  }

  return 0;
}

// hehe, "bread".  It reads data into Buffer from the BITFILE, just like fread for FILE.
ILuint bread(void *Buffer, ILuint Size, ILuint Number, BITFILE *BitFile) {
  // Note that this function is somewhat useless: In binary image
  // formats, there are some pad bits after each scanline. This
  // function does not take that into account, so you must use bseek to
  // skip the calculated value of bits.

  ILuint  BuffPos = 0, Count = Size * Number;

  while (BuffPos < Count) {
    if (BitFile->ByteBitOff < 0 || BitFile->ByteBitOff > 7) {
      BitFile->ByteBitOff = 7;
      if (SIOread(BitFile->io, &BitFile->Buff, 1, 1) != 1)  // Reached eof or error...
        return BuffPos;
    }

    *((ILubyte*)(Buffer) + BuffPos) = (ILubyte)!!(BitFile->Buff & (1 << BitFile->ByteBitOff));

    BuffPos++;
    BitFile->ByteBitOff--;
  }

  return BuffPos;
}

/*
// Reads bits and puts the first bit in the file as the highest in the return value.
static ILuint breadVal(ILuint NumBits, BITFILE *BitFile) {
  ILuint  BuffPos = 0;
  ILuint  Buffer = 0;

  // Only returning up to 32 bits at one time
  if (NumBits > 32) {
    iSetError(IL_INTERNAL_ERROR);
    return 0;
  }

  while (BuffPos < NumBits) {
    Buffer <<= 1;
    if (BitFile->ByteBitOff < 0 || BitFile->ByteBitOff > 7) {
      BitFile->ByteBitOff = 7;
      if (SIOread(BitFile->io, &BitFile->Buff, 1, 1) != 1)  // Reached eof or error...
        return BuffPos;
    }

    Buffer = Buffer + (ILubyte)!!(BitFile->Buff & (1 << BitFile->ByteBitOff));

    BuffPos++;
    BitFile->ByteBitOff--;
  }

  return BuffPos;
}
*/
