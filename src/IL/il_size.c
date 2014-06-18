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

//! Fake seek function
ILint ILAPIENTRY iSizeSeek(ILHANDLE h, ILint Offset, ILuint Mode)
{
  SIO *io = (SIO*)h;
  switch (Mode)
  {
    case IL_SEEK_SET:
      io->lumpPos = Offset;
      if (io->lumpPos > io->lumpSize)
        io->lumpSize = io->lumpPos;
      break;

    case IL_SEEK_CUR:
      io->lumpPos = io->lumpPos + Offset;
      break;

    case IL_SEEK_END:
      io->lumpPos = io->lumpSize + Offset;  // Offset should be negative in this case.
      break;

    default:
      iSetError(IL_INTERNAL_ERROR);  // Should never happen!
      return -1;  // Error code
  }

  if (io->lumpPos > io->lumpSize)
    io->lumpSize = io->lumpPos;

  return 0;  // Code for success
}

ILuint ILAPIENTRY iSizeTell(ILHANDLE h)
{
  SIO *io = (SIO*)h;
  return io->lumpPos;
}

ILint ILAPIENTRY iSizePutc(ILubyte Char, ILHANDLE h)
{
  SIO *io = (SIO*)h;
  io->lumpPos++;
  if (io->lumpPos > io->lumpSize)
    io->lumpSize = io->lumpPos;
  return Char;
}

ILint ILAPIENTRY iSizeWrite(const void *Buffer, ILuint Size, ILuint Number, ILHANDLE h)
{
  SIO *io = (SIO*)h;

  (void)Buffer;

  io->lumpPos += Size * Number;
  if (io->lumpPos > io->lumpSize)
    io->lumpSize = io->lumpPos;
  return Number;
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
  iSetOutputFake(Image);  // Sets iputc, iwrite, etc. to functions above.
  iSaveFuncs2(Image, Type);
  size = Image->io.lumpSize;
  Image->io = io;
  return size;
}
