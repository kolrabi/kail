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

#define ILUT_ATTRIB_STACK_MAX 32

//
// Various states
//

typedef struct ILUT_STATES
{

	// ILUT states
	ILboolean	ilutUsePalettes;
	ILboolean	ilutOglConv;
	ILboolean	ilutForceIntegerFormat;
	ILenum		ilutDXTCFormat;

	// GL states
	ILboolean	ilutUseS3TC;
	ILboolean	ilutGenS3TC;
	ILboolean	ilutAutodetectTextureTarget;
	ILint		MaxTexW;
	ILint		MaxTexH;
	ILint		MaxTexD;

	// D3D states
	ILuint		D3DMipLevels;
	ILenum		D3DPool;
	ILint		D3DAlphaKeyColor; // 0x00rrggbb format , -1 for none

} ILUT_STATES;


#endif//STATES_H
