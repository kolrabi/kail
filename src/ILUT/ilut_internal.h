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

#include "ilut_states.h"

typedef struct {
  ILUT_STATES ilutStates[ILUT_ATTRIB_STACK_MAX];
  ILuint ilutCurrentPos;  // Which position on the stack
} ILUT_TLS_DATA;

///////////////////////////////////////////////////////////////////////////
//
// ILUT Functions
//

// ImageLib Utility Toolkit's OpenGL Functions
#ifdef ILUT_USE_OPENGL
	ILboolean ilutGLInit(void);
#endif

// ImageLib Utility Toolkit's Win32 Functions
#ifdef ILUT_USE_WIN32
	ILboolean ilutWin32Init(void);
#endif

// ImageLib Utility Toolkit's DirectX9 Functions
#ifdef ILUT_USE_DIRECTX9
	ILboolean ilutD3D9Init(void);
#endif

// ImageLib Utility Toolkit's DirectX10 Functions
#ifdef ILUT_USE_DIRECTX10
	ILboolean ilutD3D10Init(void);
#endif

#ifdef ILUT_USE_SDL
  ILboolean InitSDL(void);
#endif

#ifdef ILUT_USE_X11
  ILboolean ilutXInit(void);
#endif

#define CUBEMAP_SIDES 6
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) {if((p)!=NULL){(p)->lpVtbl->Release(p);(p)=NULL;}}
#endif

void	ilutDefaultStates(void);

#define imemclear(x,y) memset(x,0,y);

#endif//INTERNAL_H
