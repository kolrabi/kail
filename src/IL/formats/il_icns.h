//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 08/23/2008
//
// Filename: src-IL/include/il_icns.h
//
// Description: Reads from a Mac OS X icon (.icns) file.
//
//-----------------------------------------------------------------------------


#ifndef ICNS_H
#define ICNS_H

#include "il_internal.h"

#include "pack_push.h"

typedef struct ICNSHEAD
{
	char		Head[4];	// Must be 'ICNS'
	ILuint		Size;		// Total size of the file (including header)
} ICNSHEAD;

typedef struct ICNSDATA
{
	char		ID[4];		// Identifier ('it32', 'il32', etc.)
	ILuint		Size;		// Total size of the entry (including identifier)
} ICNSDATA;

#include "pack_pop.h"

ILboolean iIcnsReadData(ILimage* image, ILboolean *BaseCreated, ILboolean IsAlpha, 
	ILuint Width, ICNSDATA *Entry, ILimage **Image);

#endif//ICNS_H
