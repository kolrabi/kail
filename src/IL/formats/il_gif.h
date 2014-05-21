//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Copyright (C)      2014 by Björn Paetzel
// Last modified: 2014-05-20
//
// Filename: src/IL/formats/il_gif.h
//
// Description: Reads from a Graphics Interchange Format (.gif) file.
//
//-----------------------------------------------------------------------------


#ifndef GIF_H
#define GIF_H

#include "il_internal.h"

#if __cplusplus
extern "C" {
#endif

#define GIF_VERSION87A "GIF87a"
#define GIF_VERSION89A "GIF89a"

enum {
	GifFlag_LSD_GlobalColorTableSizeMask 	= (7<<0),
	GifFlag_LSD_Sort                     	= (1<<3),
	GifFlag_LSD_ColorResolutionMask      	= (7<<4),
	GifFlag_LSD_HasGlobalColorTable      	= (1<<7),

	GifFlag_GCE_TransparentColor         	= (1<<0),
	GifFlag_GCE_UserInput                	= (1<<1),
	GifFlag_GCE_DisposalMethodMask       	= (7<<2),
	GifFlag_GCE_DisposalMethodShift      	=     2 ,
	GifFlag_GCE_ReservedMask             	= (3<<5),
	GifFlag_GCE_ReservedShift            	=     5 ,

	GifFlag_IMG_LocalColorTableSizeMask 	= (7<<0),
	GifFlag_IMG_ReservedMask              = (3<<3),
	GifFlag_IMG_Sort                     	= (1<<5),
	GifFlag_IMG_Interlaced               	= (1<<6),
	GifFlag_IMG_HasLocalColorTable       	= (1<<7),

	GifID_Terminator 											= 0x00,
	GifID_Extension                  			= 0x21,
	GifID_Image                      			= 0x2c,
	GifID_End                             = 0x3b,

  GifExt_PlainText                      = 0x01,
	GifExt_GraphicControl            			= 0xf9,
	GifExt_Comment                   			= 0xfe,
	GifExt_Application               			= 0xff,

	GifDisposal_DontCare                  =    0,
	GifDisposal_Overlay                   =    1,
	GifDisposal_Clear                     =    2,
	GifDisposal_Restore                   =    3,
};

#pragma pack(push)
#pragma pack(1)

typedef struct GifSignature {
	char Magic[6];
} GifSignature;

typedef struct GifLogicalScreenDescriptor {
	ILushort Width;
	ILushort Height;
	ILubyte  Flags;
	ILubyte  Background;
	ILubyte  PixelAspect;
} GifLogicalScreenDescriptor;

typedef struct GifGraphicControlExtension {
	ILubyte  Flags;
	ILushort Delay;
	ILubyte  TransparentColor;
} GifGraphicControlExtension;

typedef struct GifImageDescriptor {
	ILushort Left;
	ILushort Top;
	ILushort Width;
	ILushort Height;
	ILubyte  Flags;
} GifImageDescriptor;

#pragma pack(pop)


typedef struct GifLoadingContext {
	ILimage * Target;
	ILimage * Image;
	ILimage * PrevImage;

	GifLogicalScreenDescriptor Screen;
	ILuint 		Frame;

	ILuint   	Delay;

	ILboolean UseTransparentColor;
	ILubyte   TransparentColor;

	ILboolean UseLocalPal;
	ILpal			GlobalPal;
	ILpal			LocalPal;
	ILushort	Colors;

	ILubyte   DisposalMethod, NextDisposalMethod;
	ILboolean IsInterlaced;

	ILubyte   LZWCodeSize;
} GifLoadingContext;

typedef struct LZWInputStream {
	GifLoadingContext *Ctx;

	ILubyte 	CodeSize;
	ILubyte 	OriginalCodeSize;
	
	ILubyte 	InputBuffer[256];
	ILubyte 	InputAvail;
	ILubyte   InputPos;

	ILuint    InputBits;
	ILuint  	InputBitCount;

	ILuint    ClearCode, EndCode;
	ILuint    PrevCode, NextCode;

	ILuint *  Phrases[4096];
	ILubyte   OutputBuffer[4096];
	ILuint    OutputBufferLen;
} LZWInputStream;

#if __cplusplus
}
#endif

#endif//GIF_H
