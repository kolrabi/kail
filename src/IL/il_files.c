//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 01/04/2009
//
// Filename: src-IL/src/il_files.c
//
// Description: File handling for DevIL
//
//-----------------------------------------------------------------------------


#define __FILES_C
#include "il_internal.h"
#include <stdarg.h>

// Lump read functions
ILboolean ILAPIENTRY iEofLump     (ILHANDLE h);
ILint     ILAPIENTRY iGetcLump    (ILHANDLE h);
ILuint    ILAPIENTRY iReadLump    (ILHANDLE h, void *Buffer, const ILuint Size, const ILuint Number);
ILint     ILAPIENTRY iSeekLump    (ILHANDLE h, ILint Offset, ILuint Mode);
ILuint    ILAPIENTRY iTellLump    (ILHANDLE h);
ILint     ILAPIENTRY iPutcLump    (ILubyte Char, ILHANDLE h);
ILint     ILAPIENTRY iWriteLump   (const void *Buffer, ILuint Size, ILuint Number, ILHANDLE h);

// "Fake" size functions, used to determine the size of write lumps
// Definitions are in il_size.cpp
ILint     ILAPIENTRY iSizeSeek    (ILHANDLE h, ILint Offset, ILuint Mode);
ILuint    ILAPIENTRY iSizeTell    (ILHANDLE h);
ILint     ILAPIENTRY iSizePutc    (ILubyte Char, ILHANDLE h);
ILint     ILAPIENTRY iSizeWrite   (const void *Buffer, ILuint Size, ILuint Number, ILHANDLE h);

// Next 7 functions are the default read functions

ILHANDLE ILAPIENTRY iDefaultOpenR(ILconst_string FileName) {
#ifndef _UNICODE
  ILHANDLE File = (ILHANDLE)fopen((char*)FileName, "rb");
  return File;
#else
  // Windows has a different function, _wfopen, to open UTF16 files,
  //  whereas Linux just uses fopen for its UTF8 files.
  #ifdef _WIN32
    return (ILHANDLE)_wfopen(FileName, L"rb");
  #else
    size_t length = wcstombs(NULL, FileName, 0);
    char    tmp[length+1];
    length = wcstombs(tmp, FileName, length);
    tmp[length] = 0;
    return (ILHANDLE)fopen(tmp, "rb");
  #endif
#endif//UNICODE
}


void ILAPIENTRY iDefaultClose(ILHANDLE Handle) {
  fclose((FILE*)Handle);
  return;
}


ILboolean ILAPIENTRY iDefaultEof(ILHANDLE Handle) {
  // Find out the filesize for checking for the end of file
  ILuint OrigPos = iDefaultTell(Handle);
  iDefaultSeek(Handle, 0, SEEK_END);
  
  ILuint FileSize = iDefaultTell(Handle);
  iDefaultSeek(Handle, OrigPos, SEEK_SET);
  clearerr((FILE*)Handle);

  if (OrigPos >= FileSize)
    return IL_TRUE;

  return IL_FALSE;
}


ILint ILAPIENTRY iDefaultGetc(ILHANDLE Handle) {
  ILubyte Val;

  Val = 0;
  if (iDefaultRead(Handle, &Val, 1, 1) != 1) {
    return IL_EOF;
  }

  return Val;
}


// @todo: change this back to have the handle in the last argument, then
// fread can be passed directly to ilSetRead, and iDefaultRead is not needed
ILuint ILAPIENTRY iDefaultRead(ILHANDLE Handle, void *Buffer, ILuint Size, ILuint Number) {
  ILuint res = fread(Buffer, Size, Number, (FILE*) Handle);
  return res;
}


ILint ILAPIENTRY iDefaultSeek(ILHANDLE Handle, ILint Offset, ILuint Mode) {
  ILuint res = fseek((FILE*)Handle, Offset, Mode);
  clearerr((FILE*)Handle);
  return res;
}


ILuint ILAPIENTRY iDefaultTell(ILHANDLE Handle) {
  ILuint res = ftell((FILE*)Handle);
  return res;
}


ILHANDLE ILAPIENTRY iDefaultOpenW(ILconst_string FileName) {
#ifndef _UNICODE
  return (ILHANDLE)fopen((char*)FileName, "wb");
#else
  // Windows has a different function, _wfopen, to open UTF16 files,
  //  whereas Linux just uses fopen.
  #ifdef _WIN32
    return (ILHANDLE)_wfopen(FileName, L"wb");
  #else
    size_t length = wcstombs(NULL, FileName, 0);
    char    tmp[length+1];
    length = wcstombs(tmp, FileName, length);
    tmp[length] = 0;
    return (ILHANDLE)fopen(tmp, "wb");
  #endif
#endif//UNICODE
}


void ILAPIENTRY iDefaultCloseW(ILHANDLE Handle) {
  fclose((FILE*)Handle);
  return;
}


ILint ILAPIENTRY iDefaultPutc(ILubyte Char, ILHANDLE Handle) {
  return fputc(Char, (FILE*)Handle);
}


ILint ILAPIENTRY iDefaultWrite(const void *Buffer, ILuint Size, ILuint Number, ILHANDLE Handle) {
  return (ILint)fwrite(Buffer, Size, Number, (FILE*)Handle);
}

void iSetRead(ILimage *Image, fOpenProc aOpen, fCloseProc aClose, fEofProc aEof, fGetcProc aGetc, 
  fReadProc aRead, fSeekProc aSeek, fTellProc aTell) {
  Image->io.openReadOnly   = aOpen;
  Image->io.close          = aClose;
  Image->io.eof            = aEof;
  Image->io.getchar        = aGetc;
  Image->io.read           = aRead;
  Image->io.seek           = aSeek;
  Image->io.tell           = aTell;
}

void iResetRead(ILimage *image) {
  iSetRead(image, iDefaultOpenR, iDefaultClose, iDefaultEof, iDefaultGetc, 
        iDefaultRead, iDefaultSeek, iDefaultTell);
}

void iSetWrite(ILimage *Image, fOpenProc Open, fCloseProc Close, fPutcProc Putc, fSeekProc Seek, 
  fTellProc Tell, fWriteProc Write) {
  Image->io.openWrite      = Open;
  Image->io.close          = Close;
  Image->io.putchar        = Putc;
  Image->io.write          = Write;
  Image->io.seek           = Seek;
  Image->io.tell           = Tell;
}

void iResetWrite(ILimage *image) {
  iSetWrite(image, iDefaultOpenW, iDefaultClose, iDefaultPutc,
        iDefaultSeek, iDefaultTell, iDefaultWrite);
}

// Tells DevIL that we're reading from a file, not a lump
void ILAPIENTRY iSetInputFile(ILimage *image, ILHANDLE File)
{
  if (image != NULL) {
    if (image->io.handle != NULL) {
      if (image->io.close == NULL) {
        iTrace("**** Image already has an open file but no close function. Possible leak.");
      } else {
        SIOclose(&image->io);
      }
    }

    image->io.handle          = File;
    image->io.ReadFileStart   = image->io.tell(image->io.handle);
  }
}

void iSetInputLumpIO(SIO *io, const void *Lump, ILuint Size) {
  io->eof         = iEofLump;
  io->getchar     = iGetcLump;
  io->read        = iReadLump;
  io->seek        = iSeekLump;
  io->tell        = iTellLump;
  io->lump        = Lump;
  io->lumpPos     = 0;
  io->lumpSize    = Size;
  io->WriteFileStart  = 
  io->ReadFileStart   = 0;
  io->handle      = (ILHANDLE)io;
}

// Tells DevIL that we're reading from a lump, not a file
void ILAPIENTRY iSetInputLump(ILimage *image, const void *Lump, ILuint Size)
{
  if (image != NULL) {
    if (image->io.handle != NULL) {
      if (image->io.close == NULL) {
        iTrace("**** Image already has an open file but no close function. Possible leak.");
      } else {
        SIOclose(&image->io);
      }
    }

    iSetInputLumpIO(&image->io, Lump, Size);

  }
}


// Tells DevIL that we're writing to a file, not a lump
void ILAPIENTRY iSetOutputFile(ILimage *image, ILHANDLE File) {
  if (image != NULL) {
    if (image->io.handle != NULL) {
      if (image->io.close == NULL) {
        iTrace("**** Image already has an open file but no close function. Possible leak.");
      } else {
        SIOclose(&image->io);
      }
    }

    image->io.handle = File;
    image->io.WriteFileStart  = image->io.tell(image->io.handle);
  }
}


// This is only called by ilDetermineSize.  Associates iputchar, etc. with
//  "fake" writing functions in il_size.c.
void iSetOutputFake(ILimage *image)
{
  if (image != NULL) {
    if (image->io.handle != NULL) {
      if (image->io.close == NULL) {
        iTrace("**** Image already has an open file but no close function. Possible leak.");
      } else {
        SIOclose(&image->io);
      }
    }

    image->io.putchar = iSizePutc;
    image->io.seek = iSizeSeek;
    image->io.tell = iSizeTell;
    image->io.write = iSizeWrite;
    image->io.handle = (ILHANDLE)&image->io;
    image->io.lumpSize = 0;
    image->io.lumpPos = 0;
    image->io.WriteFileStart  = 0;
  }
}


// Tells DevIL that we're writing to a lump, not a file
void ILAPIENTRY iSetOutputLump(ILimage *image, void *Lump, ILuint Size)
{
  // In this case, ilDetermineSize is currently trying to determine the
  //  output buffer size.  It already has the write functions it needs.
  if (Lump == NULL || image == NULL)
    return;

  if (image->io.handle != NULL) {
    if (image->io.close == NULL) {
      iTrace("**** Image already has an open file but no close function. Possible leak.");
    } else {
      SIOclose(&image->io);
    }
  }

  image->io.putchar  = iPutcLump;
  image->io.seek = iSeekLump;
  image->io.tell = iTellLump;
  image->io.write = iWriteLump;
  image->io.lump = Lump;
  image->io.lumpPos = 0;
  image->io.lumpSize = Size;
  image->io.WriteFileStart  = 0;
  image->io.handle = (ILHANDLE)&image->io;
}

ILuint64 iGetLumpPos(ILimage *Image) {
  if (!Image || Image->io.lump == NULL)
    return 0;
  return Image->io.lumpPos;
}

//
// The rest of the functions following in this file are quite
//  self-explanatory, except where commented.
//

ILboolean ILAPIENTRY iEofLump(ILHANDLE h)
{
  SIO *io = (SIO*)h;
  if (io->lumpSize != 0)
    return (io->lumpPos >= io->lumpSize);
  return IL_FALSE;
}

ILint ILAPIENTRY iGetcLump(ILHANDLE h)
{
  // If ReadLumpSize is 0, don't even check to see if we've gone past the bounds.
  SIO *io = (SIO*)h;
  if (io->lumpSize > 0) {
    if (io->lumpPos + 1 > io->lumpSize) {
      io->lumpPos--;
      iSetError(IL_FILE_READ_ERROR);
      return IL_EOF;
    }
  }

  return *((ILubyte*)io->lump + io->lumpPos++);
}


ILuint ILAPIENTRY iReadLump(ILHANDLE h, void *Buffer, const ILuint Size, const ILuint Number)
{
  SIO *io = (SIO*)h;
  ILuint i, ByteSize = IL_MIN( Size*Number, io->lumpSize-io->lumpPos);

  for (i = 0; i < ByteSize; i++) {
    *((ILubyte*)Buffer + i) = *((ILubyte*)io->lump + io->lumpPos + i);
    if (io->lumpSize > 0) {  // ReadLumpSize is too large to care about apparently
      if (io->lumpPos + i > io->lumpSize) {
        io->lumpPos += i;
        if (i != Number)
          iSetError(IL_FILE_READ_ERROR);
        return i;
      }
    }
  }

  io->lumpPos += i;
  if (Size != 0)
    i /= Size;
  if (i != Number)
    iSetError(IL_FILE_READ_ERROR);
  return i;
}

// Returns 1 on error, 0 on success
ILint ILAPIENTRY iSeekLump(ILHANDLE h, ILint Offset, ILuint Mode)
{
  SIO *io = (SIO*)h;

  switch (Mode)
  {
    case IL_SEEK_SET:
      if (Offset > (ILint)io->lumpSize)
        return 1;
      io->lumpPos = Offset;
      break;

    case IL_SEEK_CUR:
      if (io->lumpPos + Offset > io->lumpSize || io->lumpPos+Offset < 0)
        return 1;
      io->lumpPos += Offset;
      break;

    case IL_SEEK_END:
      if (Offset > 0)
        return 1;
      // Should we use >= instead?
      if (abs(Offset) > (ILint)io->lumpSize)  // If ReadLumpSize == 0, too bad
        return 1;
      io->lumpPos = io->lumpSize + Offset;
      break;

    default:
      return 1;
  }

  return 0;
}


ILint ILAPIENTRY iPutcLump(ILubyte Char, ILHANDLE h)
{
  SIO *io = (SIO*)h;
  if (io->lumpPos >= io->lumpSize)
    return IL_EOF;  // IL_EOF
  *((ILubyte*)(io->lump) + io->lumpPos++) = Char;
  return Char;
}


ILint ILAPIENTRY iWriteLump(const void *Buffer, ILuint Size, ILuint Number, ILHANDLE h)
{
  SIO *io = (SIO*)h;
  ILuint SizeBytes = Size * Number;
  ILuint i = 0;

  for (; i < SizeBytes; i++) {
    if (io->lumpSize > 0) {
      if (io->lumpPos + i >= io->lumpSize) {  // Should we use > instead?
        iSetError(IL_FILE_WRITE_ERROR);
        io->lumpPos += i;
        return i;
      }
    }

    *((ILubyte*)io->lump + io->lumpPos + i) = *((ILubyte*)Buffer + i);
  }

  io->lumpPos += SizeBytes;
  
  return SizeBytes;
}


ILuint ILAPIENTRY iTellLump(ILHANDLE h)
{
  SIO *io = (SIO*)h;
  return io->lumpPos;
}
