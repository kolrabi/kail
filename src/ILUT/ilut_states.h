//-----------------------------------------------------------------------------
//
// ImageLib Utility Toolkit Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/28/2001 <--Y2K Compliant! =]
//
// Filename: src-ILUT/include/ilut_states.h
//
// Description: State machine
//
//-----------------------------------------------------------------------------

#ifndef STATES_H
#define STATES_H

#include "ilut_internal.h"


ILboolean ilutAble(ILenum Mode, ILboolean Flag);
void iInitThreads_ilut(void);

ILenum ilutGetCurrentFlags(void);

#define ILUT_ATTRIB_STACK_MAX 32

//
// Various states
//

enum {
	ILUT_STATE_FLAG_USE_PALETTES 			= (1<<0),
	ILUT_STATE_FLAG_OGL_CONV     			= (1<<1),
	ILUT_STATE_FLAG_FORCE_INT_FORMAT 	= (1<<2),
	ILUT_STATE_FLAG_USE_S3TC         	= (1<<3),
	ILUT_STATE_FLAG_GEN_S3TC         	= (1<<4),
	ILUT_STATE_FLAG_AUTODETECT_TARGET	= (1<<5),
};

typedef struct ILUT_STATES
{

	// ILUT states
	ILuint    Flags;
	ILenum		DXTCFormat;

	// GL states
	ILint			MaxTexW;
	ILint			MaxTexH;
	ILint			MaxTexD;

	// D3D states
	ILuint		D3DMipLevels;
	ILenum		D3DPool;
	ILint			D3DAlphaKeyColor; // 0x00rrggbb format , -1 for none

} ILUT_STATES;


#endif//STATES_H
