//-----------------------------------------------------------------------------
//
// ImageLib Utility Toolkit Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 02/07/2002 <--Y2K Compliant! =]
//
// Filename: src-ILUT/include/ilut_internal.h
//
// Description: Internal stuff for ILUT
//
//-----------------------------------------------------------------------------


#ifndef INTERNAL_H
#define INTERNAL_H

#define _IL_BUILD_LIBRARY
#define _ILU_BUILD_LIBRARY
#define _ILUT_BUILD_LIBRARY

///////////////////////////////////////////////////////////////////////////
//
// Include required headers
//

#include <IL/ilut.h>
#include <IL/devil_internal_exports.h>
#include <stdlib.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////
//
// ILUT Functions
//

// ImageLib Utility Toolkit's OpenGL Functions
#ifdef ILUT_USE_OPENGL
	ILboolean ilutGLInit();
#endif

// ImageLib Utility Toolkit's Win32 Functions
#ifdef ILUT_USE_WIN32
	ILboolean ilutWin32Init();
#endif

// ImageLib Utility Toolkit's Win32 Functions
#ifdef ILUT_USE_DIRECTX8
	ILboolean ilutD3D8Init();
#endif

#ifdef ILUT_USE_DIRECTX9
	ILboolean ilutD3D9Init();
#endif

#ifdef ILUT_USE_DIRECTX10
	ILboolean ilutD3D10Init();
#endif

#define CUBEMAP_SIDES 6
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) {if((p)!=NULL){(p)->lpVtbl->Release(p);(p)=NULL;}}
#endif

void	ilutDefaultStates(void);

///////////////////////////////////////////////////////////////////////////
//
// globals
//

extern ILimage *ilutCurImage;

#endif//INTERNAL_H
