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
ILboolean	ILAPIENTRY iEofLump 		(ILHANDLE h);
ILint			ILAPIENTRY iGetcLump 		(ILHANDLE h);
ILuint		ILAPIENTRY iReadLump 		(ILHANDLE h, void *Buffer, const ILuint Size, const ILuint Number);
ILint			ILAPIENTRY iSeekLump 		(ILHANDLE h, ILint64 Offset, ILuint Mode);
ILuint		ILAPIENTRY iTellLump 		(ILHANDLE h);
ILint			ILAPIENTRY iPutcLump 		(ILubyte Char, ILHANDLE h);
ILint			ILAPIENTRY iWriteLump 	(const void *Buffer, ILuint Size, ILuint Number, ILHANDLE h);

// "Fake" size functions, used to determine the size of write lumps
// Definitions are in il_size.cpp
ILint			ILAPIENTRY iSizeSeek 		(ILHANDLE h, ILint64 Offset, ILuint Mode);
ILuint		ILAPIENTRY iSizeTell 		(ILHANDLE h);
ILint			ILAPIENTRY iSizePutc 		(ILubyte Char, ILHANDLE h);
ILint			ILAPIENTRY iSizeWrite 	(const void *Buffer, ILuint Size, ILuint Number, ILHANDLE h);

// Next 7 functions are the default read functions

ILHANDLE ILAPIENTRY iDefaultOpenR(ILconst_string FileName)
{
#ifndef _UNICODE
	return (ILHANDLE)fopen((char*)FileName, "rb");
#else
	// Windows has a different function, _wfopen, to open UTF16 files,
	//  whereas Linux just uses fopen for its UTF8 files.
	#ifdef _WIN32
		return (ILHANDLE)_wfopen(FileName, L"rb");
	#else
		size_t length = wcstombs(FileName, NULL, 0);
		char 		tmp[length+1];
		length = wcstombs(FileName, tmp, length);
		tmp[length] = 0;
		return (ILHANDLE)fopen(tmp, "rb");
	#endif
#endif//UNICODE
}


void ILAPIENTRY iDefaultCloseR(ILHANDLE Handle)
{
	fclose((FILE*)Handle);
	return;
}


ILboolean ILAPIENTRY iDefaultEof(ILHANDLE Handle)
{
	(void)Handle;

	ILuint OrigPos, FileSize;

	// Find out the filesize for checking for the end of file
	OrigPos = iCurImage->io.tell(iCurImage->io.handle);
	iCurImage->io.seek(iCurImage->io.handle, 0, IL_SEEK_END);
	FileSize = iCurImage->io.tell(iCurImage->io.handle);
	iCurImage->io.seek(iCurImage->io.handle, OrigPos, IL_SEEK_SET);

	if (iCurImage->io.tell(iCurImage->io.handle) >= FileSize)
		return IL_TRUE;
	return IL_FALSE;
}


ILint ILAPIENTRY iDefaultGetc(ILHANDLE Handle)
{
	(void)Handle;

	ILint Val;

	Val = 0;
	if (iCurImage->io.read(iCurImage->io.handle, &Val, 1, 1) != 1)
		return IL_EOF;
	return Val;
}


// @todo: change this back to have the handle in the last argument, then
// fread can be passed directly to ilSetRead, and iDefaultRead is not needed
ILuint ILAPIENTRY iDefaultRead(ILHANDLE h, void *Buffer, ILuint Size, ILuint Number)
{
	return (ILint)fread(Buffer, Size, Number, (FILE*) h);
}


ILint ILAPIENTRY iDefaultSeek(ILHANDLE Handle, ILint64 Offset, ILuint Mode)
{
	return fseek((FILE*)Handle, Offset, Mode);
}


ILuint ILAPIENTRY iDefaultTell(ILHANDLE Handle)
{
	return ftell((FILE*)Handle);
}


ILHANDLE ILAPIENTRY iDefaultOpenW(ILconst_string FileName)
{
#ifndef _UNICODE
	return (ILHANDLE)fopen((char*)FileName, "wb");
#else
	// Windows has a different function, _wfopen, to open UTF16 files,
	//  whereas Linux just uses fopen.
	#ifdef _WIN32
		return (ILHANDLE)_wfopen(FileName, L"wb");
	#else
		size_t length = wcstombs(FileName, NULL, 0);
		char 		tmp[length+1];
		length = wcstombs(FileName, tmp, length);
		tmp[length] = 0;
		return (ILHANDLE)fopen(tmp, "wb");
	#endif
#endif//UNICODE
}


void ILAPIENTRY iDefaultCloseW(ILHANDLE Handle)
{
	fclose((FILE*)Handle);
	return;
}


ILint ILAPIENTRY iDefaultPutc(ILubyte Char, ILHANDLE Handle)
{
	return fputc(Char, (FILE*)Handle);
}


ILint ILAPIENTRY iDefaultWrite(const void *Buffer, ILuint Size, ILuint Number, ILHANDLE Handle)
{
	return (ILint)fwrite(Buffer, Size, Number, (FILE*)Handle);
}


void ILAPIENTRY ilResetRead()
{
	ilSetRead(iDefaultOpenR, iDefaultCloseR, iDefaultEof, iDefaultGetc, 
				iDefaultRead, iDefaultSeek, iDefaultTell);
	return;
}


void ILAPIENTRY ilResetWrite()
{
	ilSetWrite(iDefaultOpenW, iDefaultCloseW, iDefaultPutc,
				iDefaultSeek, iDefaultTell, iDefaultWrite);
	return;
}


//! Allows you to override the default file-reading functions.
void ILAPIENTRY ilSetRead(fOpenProc aOpen, fCloseProc aClose, fEofProc aEof, fGetcProc aGetc, 
	fReadProc aRead, fSeekProc aSeek, fTellProc aTell)
{
	if (iCurImage != NULL) {
		memset(&iCurImage->io, 0, sizeof(SIO));
		iCurImage->io.openReadOnly    = aOpen;
		iCurImage->io.close   = aClose;
		iCurImage->io.eof   = aEof;
		iCurImage->io.getchar  = aGetc;
		iCurImage->io.read  = aRead;
		iCurImage->io.seek = aSeek;
		iCurImage->io.tell = aTell;
	}

	return;
}

//! Allows you to override the default file-reading functions.
void ILAPIENTRY ilSetReadF(ILHANDLE File, fOpenProc aOpen, fCloseProc aClose, fEofProc aEof, fGetcProc aGetc, 
	fReadProc aRead, fSeekProc aSeek, fTellProc aTell)
{
	if (iCurImage != NULL) {
		memset(&iCurImage->io, 0, sizeof(SIO));
		iCurImage->io.openReadOnly    = aOpen;
		iCurImage->io.close   = aClose;
		iCurImage->io.eof   = aEof;
		iCurImage->io.getchar  = aGetc;
		iCurImage->io.read  = aRead;
		iCurImage->io.seek = aSeek;
		iCurImage->io.tell = aTell;
		iCurImage->io.handle = File;
		iCurImage->io.ReadFileStart = iCurImage->io.tell(iCurImage->io.handle);
	}

	return;
}

//! Allows you to override the default file-writing functions.
void ILAPIENTRY ilSetWrite(fOpenProc Open, fCloseProc Close, fPutcProc Putc, fSeekProc Seek, 
	fTellProc Tell, fWriteProc Write)
{
	if (iCurImage != NULL) {
		memset(&iCurImage->io, 0, sizeof(SIO));
		iCurImage->io.openWrite = Open;
		iCurImage->io.close = Close;
		iCurImage->io.putchar  = Putc;
		iCurImage->io.write = Write;
		iCurImage->io.seek  = Seek;
		iCurImage->io.tell  = Tell;
	}
}

//! Allows you to override the default file-writing functions.
void ILAPIENTRY ilSetWriteF(ILHANDLE File, fOpenProc Open, fCloseProc Close, fPutcProc Putc, fSeekProc Seek, 
	fTellProc Tell, fWriteProc Write)
{
	if (iCurImage != NULL) {
		memset(&iCurImage->io, 0, sizeof(SIO));
		iCurImage->io.openWrite = Open;
		iCurImage->io.close 		= Close;
		iCurImage->io.putchar 	= Putc;
		iCurImage->io.write 		= Write;
		iCurImage->io.seek  		= Seek;
		iCurImage->io.tell  		= Tell;
		iCurImage->io.handle 		= File;
	}
}

// Tells DevIL that we're reading from a file, not a lump
void iSetInputFile(ILHANDLE File)
{
	if (iCurImage != NULL) {
		memset(&iCurImage->io, 0, sizeof(SIO));
		iCurImage->io.eof  = iDefaultEof;
		iCurImage->io.getchar = iDefaultGetc;
		iCurImage->io.read = iDefaultRead;
		iCurImage->io.seek = iDefaultSeek;
		iCurImage->io.tell = iDefaultTell;
		iCurImage->io.handle = File;
		iCurImage->io.ReadFileStart = iCurImage->io.tell(iCurImage->io.handle);
	}
}


// Tells DevIL that we're reading from a lump, not a file
void iSetInputLump(const void *Lump, ILuint Size)
{
	if (iCurImage != NULL) {
		memset(&iCurImage->io, 0, sizeof(SIO));
		iCurImage->io.eof  = iEofLump;
		iCurImage->io.getchar = iGetcLump;
		iCurImage->io.read = iReadLump;
		iCurImage->io.seek = iSeekLump;
		iCurImage->io.tell = iTellLump;
		iCurImage->io.lump = Lump;
		iCurImage->io.lumpPos = 0;
		iCurImage->io.lumpSize = Size;
	}
}


// Tells DevIL that we're writing to a file, not a lump
void iSetOutputFile(ILHANDLE File)
{
	if (iCurImage != NULL) {
		memset(&iCurImage->io, 0, sizeof(SIO));
		iCurImage->io.eof  = iEofLump;
		iCurImage->io.putchar  = iDefaultPutc;
		iCurImage->io.seek = iDefaultSeek;
		iCurImage->io.tell = iDefaultTell;
		iCurImage->io.write = iDefaultWrite;
		iCurImage->io.handle = File;
	}
}


// This is only called by ilDetermineSize.  Associates iputchar, etc. with
//  "fake" writing functions in il_size.c.
void iSetOutputFake(void)
{
	if (iCurImage != NULL) {
		memset(&iCurImage->io, 0, sizeof(SIO));
		iCurImage->io.putchar = iSizePutc;
		iCurImage->io.seek = iSizeSeek;
		iCurImage->io.tell = iSizeTell;
		iCurImage->io.write = iSizeWrite;
	}
}


// Tells DevIL that we're writing to a lump, not a file
void iSetOutputLump(void *Lump, ILuint Size)
{
	// In this case, ilDetermineSize is currently trying to determine the
	//  output buffer size.  It already has the write functions it needs.
	if (Lump == NULL || iCurImage == NULL)
		return;

	iCurImage->io.putchar  = iPutcLump;
	iCurImage->io.seek = iSeekLump;
	iCurImage->io.tell = iTellLump;
	iCurImage->io.write = iWriteLump;
	iCurImage->io.lump = Lump;
	iCurImage->io.lumpPos = 0;
	iCurImage->io.lumpSize = Size;
}


ILuint64 ILAPIENTRY ilGetLumpPos()
{
	if (iCurImage->io.lump != 0)
		return iCurImage->io.lumpPos;
	return 0;
}


ILuint ILAPIENTRY ilprintf(const char *Line, ...)
{
	char	Buffer[2048];  // Hope this is large enough
	va_list	VaLine;
	ILuint	i;

	va_start(VaLine, Line);
	vsprintf(Buffer, Line, VaLine);
	va_end(VaLine);

	i = ilCharStrLen(Buffer);
	iCurImage->io.write(Buffer, 1, i, iCurImage->io.handle);

	return i;
}


// To pad zeros where needed...
void ipad(ILuint NumZeros)
{
	ILuint i = 0;
	for (; i < NumZeros; i++)
		iCurImage->io.putchar(0, iCurImage->io.handle);
	return;
}


//
// The rest of the functions following in this file are quite
//	self-explanatory, except where commented.
//

// Next 12 functions are the default write functions

ILboolean ILAPIENTRY iEofFile(ILHANDLE h)
{
	return feof((FILE*)h);
}


ILboolean ILAPIENTRY iEofLump(ILHANDLE h)
{
	if (iCurImage->io.lumpSize != 0)
		return (iCurImage->io.lumpPos >= iCurImage->io.lumpSize);
	return IL_FALSE;
}


ILint ILAPIENTRY iGetcLump(ILHANDLE h)
{
	// If ReadLumpSize is 0, don't even check to see if we've gone past the bounds.
	if (iCurImage->io.lumpSize > 0) {
		if (iCurImage->io.lumpPos + 1 > iCurImage->io.lumpSize) {
			iCurImage->io.lumpPos--;
			ilSetError(IL_FILE_READ_ERROR);
			return IL_EOF;
		}
	}

	return *((ILubyte*)iCurImage->io.lump + iCurImage->io.lumpPos++);
}


ILuint ILAPIENTRY iReadLump(ILHANDLE h, void *Buffer, const ILuint Size, const ILuint Number)
{
	ILuint i, ByteSize = IL_MIN( Size*Number, iCurImage->io.lumpSize-iCurImage->io.lumpPos);

	for (i = 0; i < ByteSize; i++) {
		*((ILubyte*)Buffer + i) = *((ILubyte*)iCurImage->io.lump + iCurImage->io.lumpPos + i);
		if (iCurImage->io.lumpSize > 0) {  // ReadLumpSize is too large to care about apparently
			if (iCurImage->io.lumpPos + i > iCurImage->io.lumpSize) {
				iCurImage->io.lumpPos += i;
				if (i != Number)
					ilSetError(IL_FILE_READ_ERROR);
				return i;
			}
		}
	}

	iCurImage->io.lumpPos += i;
	if (Size != 0)
		i /= Size;
	if (i != Number)
		ilSetError(IL_FILE_READ_ERROR);
	return i;
}


ILint ILAPIENTRY iSeekFile(ILHANDLE h, ILint64 Offset, ILuint Mode)
{
	if (Mode == IL_SEEK_SET)
		Offset += iCurImage->io.ReadFileStart;  // This allows us to use IL_SEEK_SET in the middle of a file.
	return fseek((FILE*) iCurImage->io.handle, Offset, Mode);
}


// Returns 1 on error, 0 on success
ILint ILAPIENTRY iSeekLump(ILHANDLE h, ILint64 Offset, ILuint Mode)
{
	switch (Mode)
	{
		case IL_SEEK_SET:
			if (Offset > (ILint)iCurImage->io.lumpSize)
				return 1;
			iCurImage->io.lumpPos = Offset;
			break;

		case IL_SEEK_CUR:
			if (iCurImage->io.lumpPos + Offset > iCurImage->io.lumpSize || iCurImage->io.lumpPos+Offset < 0)
				return 1;
			iCurImage->io.lumpPos += Offset;
			break;

		case IL_SEEK_END:
			if (Offset > 0)
				return 1;
			// Should we use >= instead?
			if (abs(Offset) > (ILint)iCurImage->io.lumpSize)  // If ReadLumpSize == 0, too bad
				return 1;
			iCurImage->io.lumpPos = iCurImage->io.lumpSize + Offset;
			break;

		default:
			return 1;
	}

	return 0;
}


ILHANDLE ILAPIENTRY iGetFile(void)
{
	return iCurImage->io.handle;
}


const ILubyte* ILAPIENTRY iGetLump(void) {
	return (ILubyte*)iCurImage->io.lump;
}



// Next 4 functions are the default write functions

ILint ILAPIENTRY iPutcFile(ILubyte Char, ILHANDLE h)
{
	return putc(Char, (FILE*)iCurImage->io.handle);
}


ILint ILAPIENTRY iPutcLump(ILubyte Char, ILHANDLE h)
{
	if (iCurImage->io.lumpPos >= iCurImage->io.lumpSize)
		return IL_EOF;  // IL_EOF
	*((ILubyte*)(iCurImage->io.lump) + iCurImage->io.lumpPos++) = Char;
	return Char;
}


ILint ILAPIENTRY iWriteFile(const void *Buffer, ILuint Size, ILuint Number, ILHANDLE h)
{
	(void)h;
	ILuint NumWritten;
	NumWritten =fwrite(Buffer, Size, Number, (FILE*) iCurImage->io.handle);
	if (NumWritten != Number) {
		ilSetError(IL_FILE_WRITE_ERROR);
		return 0;
	}
	return NumWritten;
}


ILint ILAPIENTRY iWriteLump(const void *Buffer, ILuint Size, ILuint Number, ILHANDLE h)
{
	(void)h;
	ILuint SizeBytes = Size * Number;
	ILuint i = 0;

	for (; i < SizeBytes; i++) {
		if (iCurImage->io.lumpSize > 0) {
			if (iCurImage->io.lumpPos + i >= iCurImage->io.lumpSize) {  // Should we use > instead?
				ilSetError(IL_FILE_WRITE_ERROR);
				iCurImage->io.lumpPos += i;
				return i;
			}
		}

		*((ILubyte*)iCurImage->io.lump + iCurImage->io.lumpPos + i) = *((ILubyte*)Buffer + i);
	}

	iCurImage->io.lumpPos += SizeBytes;
	
	return SizeBytes;
}


ILuint ILAPIENTRY iTellFile(ILHANDLE h)
{
	return ftell((FILE*) h);
}


ILuint ILAPIENTRY iTellLump(ILHANDLE h)
{
	(void)h;
	return iCurImage->io.lumpPos;
}
