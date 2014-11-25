//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_exr.cc
//
// Description: Reads from an OpenEXR (.exr) file using the OpenEXR library.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_EXR

#ifndef HAVE_CONFIG_H // We are probably on a Windows box .
//#define OPENEXR_DLL
#define HALF_EXPORTS
#endif //HAVE_CONFIG_H

#include "il_exr.h"
#include <OpenEXR/ImfRgba.h>
#include <OpenEXR/ImfArray.h>
#include <OpenEXR/ImfRgbaFile.h>
#include <stdexcept>

#if (defined(_WIN32) || defined(_WIN64)) && defined(IL_USE_PRAGMA_LIBS)
	#if defined(_MSC_VER) || defined(__BORLANDC__)
		#ifndef _DEBUG
			#pragma comment(lib, "openexr.lib")
		#else
			#pragma comment(lib, "openexr-d.lib")
		#endif
	#endif
#endif

static ILboolean iCheckExr(EXRHEAD *Header);

// Internal function used to get the EXR header from the current file.
static void iGetExrHead(SIO *io, EXRHEAD *Header) {
	Header->MagicNumber = GetLittleUInt(io);
	Header->Version 		= GetLittleUInt(io);
}


// Internal function to get the header and check it.
static ILboolean iIsValidExr(SIO *io) {
	EXRHEAD Head;
	ILuint Start = SIOtell(io);

	iGetExrHead(io, &Head);
	SIOseek(io, Start, IL_SEEK_SET);
	
	return iCheckExr(&Head);
}


// Internal function used to check if the HEADER is a valid EXR header.
static ILboolean iCheckExr(EXRHEAD *Header) {
	// The file magic number (signature) is 0x76, 0x2f, 0x31, 0x01
	if (Header->MagicNumber != 0x01312F76)
		return IL_FALSE;

	// The only valid version so far is version 2.  The upper value has
	//  to do with tiling.
	if (Header->Version != 0x002 && Header->Version != 0x202)
		return IL_FALSE;

	return IL_TRUE;
}


// Nothing to do here in the constructor.
ilIStream::ilIStream(SIO *_io) : Imf::IStream("N/A"), io(_io) {
}

bool ilIStream::read(char c[], int n) {
	return SIOread(this->io, c, 1, n) == (ILuint)n;
}

Imf::Int64 ilIStream::tellg() {
	return SIOtell(this->io);
}

// Note that there is no return value here, even though there probably should be.
void ilIStream::seekg(Imf::Int64 Pos) {
	SIOseek(this->io, Pos, IL_SEEK_SET);  // I am assuming this is seeking from the beginning.
}

void ilIStream::clear() {
}

using namespace Imath;
using namespace Imf;
using namespace std;

static ILboolean iLoadExrInternal(ILimage *iCurImage) {
	if (!iCurImage) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Array<Rgba> pixels;
	Box2i dataWindow;
	ILfloat *FloatData;
  int dw, dh, dx, dy;
	Rgba a;

	try {
		ilIStream File(&iCurImage->io);
		RgbaInputFile in(File);

	  dataWindow       = in.dataWindow();
	  /* pixelAspectRatio = */ in.pixelAspectRatio();
 
		dw = dataWindow.max.x - dataWindow.min.x + 1;
	  dh = dataWindow.max.y - dataWindow.min.y + 1;
	  dx = dataWindow.min.x;
	  dy = dataWindow.min.y;

	  pixels.resizeErase (dw * dh);
		in.setFrameBuffer (pixels - dx - dy * dw, 1, dw);

		in.readPixels (dataWindow.min.y, dataWindow.max.y);

		if (iTexImage(iCurImage, dw, dh, 1, 4, IL_RGBA, IL_FLOAT, NULL) == IL_FALSE)
			return IL_FALSE;

		// Determine where the origin is in the original file.
		if (in.lineOrder() == INCREASING_Y)
			iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;
		else
			iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

  } catch (const exception &e) {
		// If some of the pixels in the file cannot be read,
		// print an error message, and return a partial image
		// to the caller.
		iSetError(IL_LIB_EXR_ERROR);  // Could I use something a bit more descriptive based on e?
		(void)e;  // Prevent the compiler from yelling at us about this being unused.
		return IL_FALSE;
  }

	// Better to access FloatData instead of recasting everytime.
	FloatData = (ILfloat*)iCurImage->Data;

	for (int i = 0; i < dw * dh; i++)
	{
		// Too much data lost
		//iCurImage->Data[i * 4 + 0] = (ILubyte)(pixels[i].r.bits() >> 8);
		//iCurImage->Data[i * 4 + 1] = (ILubyte)(pixels[i].g.bits() >> 8);
		//iCurImage->Data[i * 4 + 2] = (ILubyte)(pixels[i].b.bits() >> 8);
		//iCurImage->Data[i * 4 + 3] = (ILubyte)(pixels[i].a.bits() >> 8);

		// The images look kind of washed out with this.
		//((ILshort*)(iCurImage->Data))[i * 4 + 0] = pixels[i].r.bits();
		//((ILshort*)(iCurImage->Data))[i * 4 + 1] = pixels[i].g.bits();
		//((ILshort*)(iCurImage->Data))[i * 4 + 2] = pixels[i].b.bits();
		//((ILshort*)(iCurImage->Data))[i * 4 + 3] = pixels[i].a.bits();

		// This gives the best results, since no data is lost.
		FloatData[i * 4]     = pixels[i].r;
		FloatData[i * 4 + 1] = pixels[i].g;
		FloatData[i * 4 + 2] = pixels[i].b;
		FloatData[i * 4 + 3] = pixels[i].a;
	}

	return IL_TRUE;
}

// Nothing to do here in the constructor.
ilOStream::ilOStream(SIO *_io) : Imf::OStream("N/A"), io(_io) {
}

void ilOStream::write(const char c[], int n) {
	if (SIOwrite(this->io, c, 1, n) != (ILuint)n)
		throw std::length_error("SIOwrite() failed"); // TODO: create own exception?
}

Imf::Int64 ilOStream::tellp() {
	return SIOtell(this->io);
}

// Note that there is no return value here, even though there probably should be.
void ilOStream::seekp(Imf::Int64 Pos) {
	SIOseek(this->io, Pos, IL_SEEK_SET);  // I am assuming this is seeking from the beginning.
}

static ILboolean iSaveExrInternal(ILimage *iCurImage) {
	Imath::Box2i DataWindow(Imath::V2i(0, 0), Imath::V2i(iCurImage->Width-1, iCurImage->Height-1));
	Imf::LineOrder Order;

	if (iCurImage->Origin == IL_ORIGIN_LOWER_LEFT)
		Order = DECREASING_Y;
	else
		Order = INCREASING_Y;

	Imf::Header Head(iCurImage->Width, iCurImage->Height, DataWindow, 1, Imath::V2f (0, 0), 1, Order);

	ilOStream File(&iCurImage->io);
	Imf::RgbaOutputFile Out(File, Head);
	ILimage *TempImage = iCurImage;

	//@TODO: Can we always assume that Rgba is packed the same?
	Rgba *HalfData = (Rgba*)ialloc(TempImage->Width * TempImage->Height * sizeof(Rgba));
	if (HalfData == NULL)
		return IL_FALSE;

	if (iCurImage->Format != IL_RGBA || iCurImage->Type != IL_FLOAT) {
		TempImage = iConvertImage(iCurImage, IL_RGBA, IL_FLOAT);
		if (TempImage == NULL) {
			ifree(HalfData);
			return IL_FALSE;
		}
	}

	ILuint Offset = 0;
	ILfloat *FloatPtr = (ILfloat*)TempImage->Data;
	for (unsigned int y = 0; y < TempImage->Height; y++) {
		for (unsigned int x = 0; x < TempImage->Width; x++) {
			HalfData[y * TempImage->Width + x].r = FloatPtr[Offset];
			HalfData[y * TempImage->Width + x].g = FloatPtr[Offset + 1];
			HalfData[y * TempImage->Width + x].b = FloatPtr[Offset + 2];
			HalfData[y * TempImage->Width + x].a = FloatPtr[Offset + 3];
			Offset += 4;  // 4 floats
		}
	}

	Out.setFrameBuffer(HalfData, 1, TempImage->Width);
	Out.writePixels(TempImage->Height);  //@TODO: Do each scanline separately to keep from using so much memory.

	// Free our half data.
	ifree(HalfData);
	// Destroy our temporary image if we used one.
	if (TempImage != iCurImage)
		iCloseImage(TempImage);

	return IL_TRUE;
}

ILconst_string iFormatExtsEXR[] = { 
	IL_TEXT("exr"),
	NULL 
};

ILformat iFormatEXR = { 
	/* .Validate = */ iIsValidExr, 
	/* .Load     = */ iLoadExrInternal, 
	/* .Save     = */ iSaveExrInternal, 
	/* .Exts     = */ iFormatExtsEXR
};

#endif //IL_NO_EXR
