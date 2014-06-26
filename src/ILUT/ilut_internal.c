//-----------------------------------------------------------------------------
//
// ImageLib Utility Toolkit Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/15/2002 <--Y2K Compliant! =]
//
// Filename: src-ILUT/src/ilut_internal.c
//
// Description: Internal stuff for ILUT
//
//-----------------------------------------------------------------------------


#include "ilut_internal.h"

void iInitThreads(void);

/**
 * Initializes the ILUT using all available renderers.
 * ilInit() must have been called before.
 * @ingroup ilut_setup
 * @see ilutRenderer
 */
void ILAPIENTRY ilutInit()
{
  iInitThreads();
  ilutDefaultStates();  // Set states to their defaults
  // Can cause crashes if DevIL is not initialized yet

#ifdef ILUT_USE_OPENGL
  ilutGLInit();  // default renderer is OpenGL
#endif

#ifdef ILUT_USE_DIRECTX8
  ilutD3D8Init();
#endif

#ifdef ILUT_USE_DIRECTX9
  ilutD3D9Init();
#endif

#ifdef ILUT_USE_DIRECTX10
  ilutD3D10Init();
#endif

#ifdef ILUT_USE_WIN32
  ilutWin32Init();
#endif

#ifdef ILUT_USE_X11
  ilutXInit();
#endif


#ifdef ILUT_USE_SDL
  InitSDL();
#endif
}
