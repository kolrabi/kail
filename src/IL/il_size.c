//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 01/30/2009
//
// Filename: src-IL/src/il_size.c
//
// Description: Determines the size of output files for lump writing.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"

static ILboolean iSizeResize(SIO *io, ILuint newSize) {
  if (newSize > io->lumpAlloc) {
    ILubyte *newLump = (ILubyte*)ialloc(newSize + 1024);
    if (!newLump) 
      return IL_FALSE;

    memcpy(newLump, io->lump, io->lumpSize);
    ifree(io->lump);
    io->lump = newLump;
    io->lumpAlloc = newSize + 1024;
  }

  if (newSize > io->lumpSize)
    io->lumpSize = newSize;

  return IL_TRUE;
}

void ILAPIENTRY iSizeClose(ILHANDLE h)
{
  SIO *io = (SIO*)h;
  if (io && io->lump && io->lumpAlloc) {
    ifree(io->lump);
    io->lumpAlloc = 0;
  }
}

//! Fake seek function
ILint ILAPIENTRY iSizeSeek(ILHANDLE h, ILint64 Offset, ILuint Mode) {
  SIO *io = (SIO*)h;
  switch (Mode)
  {
    case IL_SEEK_SET:
      io->lumpPos = (ILuint)Offset;
      if (io->lumpPos > io->lumpSize)
        io->lumpSize = io->lumpPos;
      break;

    case IL_SEEK_CUR:
      io->lumpPos = (ILuint)(io->lumpPos + Offset);
      break;

    case IL_SEEK_END:
      io->lumpPos = (ILuint)(io->lumpSize + Offset);  // Offset should be negative in this case.
      break;

    default:
      iSetError(IL_INTERNAL_ERROR);  // Should never happen!
      return -1;  // Error code
  }

  if (!iSizeResize(io, io->lumpPos))
    return -1;

  return 0;  // Code for success
}

ILuint ILAPIENTRY iSizeTell(ILHANDLE h) {
  SIO *io = (SIO*)h;
  return io->lumpPos;
}

ILint ILAPIENTRY iSizePutc(ILubyte Char, ILHANDLE h) {
  SIO *io = (SIO*)h;

  if (!iSizeResize(io, io->lumpPos + 1))
    return -1;

  ((ILubyte*)io->lump)[io->lumpPos] = Char;
  io->lumpPos ++;

  return Char;
}

ILint ILAPIENTRY iSizeGetc(ILHANDLE h) {
  SIO *io = (SIO*)h;

  if (io->lumpPos >= io->lumpSize) 
    return -1;

  return ((ILubyte*)io->lump)[io->lumpPos++];
}

ILuint ILAPIENTRY iSizeWrite(const void *Buffer, ILuint Size, ILuint Number, ILHANDLE h) {
  SIO *io = (SIO*)h;

  if (!iSizeResize(io, io->lumpPos + Size * Number))
    return 0;

  memcpy(io->lumpPos + (ILubyte*)io->lump, Buffer, Size*Number);

  io->lumpPos += Size * Number;
  return Number;
}

ILuint ILAPIENTRY iSizeRead(ILHANDLE h, void *Buffer, ILuint Size, ILuint Number) {
  SIO *io = (SIO*)h;
  ILuint Count = 0;

  if (io->lumpPos > io->lumpSize)
    return 0;

  Count = (io->lumpSize - io->lumpPos) / Size;
  if (Count > Number) Count = Number;
  memcpy(Buffer, io->lumpPos + (ILubyte*)io->lump, Count * Size);

  io->lumpPos += Size * Count;
  return Count;
}

// While it might be tempting to optimize this for uncompressed files, there are two reasons
// while this may not be a good idea:
// 1. With the current solution, changes in the file handler will be reflected immediately without additional 
//    effort in the size computation
// 2. The uncompressed file handlers are usually not a performance concern here, considering that no data
//    is actually written to a file
ILuint iDetermineSize(ILimage *Image, ILenum Type) {
  SIO io;
  ILuint size;

  if (!Image)
    return 0;

  io = Image->io;
  Image->io.handle = NULL;    // or iSetOutputFake will close it
  iSetOutputFake(Image);      // Sets iputc, iwrite, etc. to functions above.
  iSaveFuncs2(Image, Type);
  size = Image->io.lumpSize;
  SIOclose(&Image->io);
  Image->io = io;
  return size;
}
