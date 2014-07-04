//-----------------------------------------------------------------------------
//
// ImageLib Utility Toolkit Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-ILUT/src/ilut_states.c
//
// Description: The state machine
//
//-----------------------------------------------------------------------------


/**
 * @file
 * @brief State machine
 * @defgroup ILUT
 * @ingroup ILUT
 * @{
 * 
 * @defgroup ilut_state ILUT State
 */

#include "ilut_internal.h"
#include "ilut_states.h"
#include "ilut_opengl.h"

static ILconst_string _ilutVendor  = IL_TEXT("kolrabi");
static ILconst_string _ilutVersion = IL_TEXT("kolrabi's another Image Library Utility Toolkit (ILUT) 1.10.0");

#if IL_THREAD_SAFE_PTHREAD
#include <pthread.h>
  static pthread_key_t iTlsKey;
#elif IL_THREAD_SAFE_WIN32
  static DWORD         iTlsKey = ~0;
#endif

static void iInitTlsData(ILUT_TLS_DATA *Data) {
  imemclear(Data, sizeof(*Data)); 
}

#if IL_THREAD_SAFE_PTHREAD
// only called when thread safety is enabled and a thread stops
static void iFreeTLSData(void *ptr) {
  ILUT_TLS_DATA *Data = (ILUT_TLS_DATA*)ptr;
  ifree(Data);
}
#endif

ILUT_TLS_DATA * iGetTLSData() {
#if IL_THREAD_SAFE_PTHREAD
  ILUT_TLS_DATA *iDataPtr = (ILUT_TLS_DATA*)pthread_getspecific(iTlsKey);
  if (iDataPtr == NULL) {
    iDataPtr = ioalloc(ILUT_TLS_DATA);
    pthread_setspecific(iTlsKey, iDataPtr);
    iInitTlsData(iDataPtr);
  }

  return iDataPtr;

#elif IL_THREAD_SAFE_WIN32

  ILUT_TLS_DATA *iDataPtr = (ILUT_TLS_DATA*)TlsGetValue(iTlsKey);
  if (iDataPtr == NULL) {
    iDataPtr = ioalloc(ILUT_TLS_DATA);
    TlsSetValue(iTlsKey, iDataPtr);
    iInitTlsData(iDataPtr);
  }

  return iDataPtr;

#else
  static ILUT_TLS_DATA iData;
  static ILUT_TLS_DATA *iDataPtr = NULL;

  if (iDataPtr == NULL) {
    iDataPtr = &iData;
    iInitTlsData(iDataPtr);
  }
  return iDataPtr;
#endif
}

void iInitThreads(void) {
  static ILboolean isInit = IL_FALSE;
  if (!isInit) {
  #if IL_THREAD_SAFE_PTHREAD
    pthread_key_create(&iTlsKey, &iFreeTLSData);
  #elif IL_THREAD_SAFE_WIN32
    iTlsKey = TlsAlloc();
  #endif
    isInit = IL_TRUE;

    ilutDefaultStates();
  }
}

// Set all states to their defaults
void ilutDefaultStates() {
  ILUT_STATES *ilutStates = iGetTLSData()->ilutStates;
  ILuint       ilutCurrentPos = iGetTLSData()->ilutCurrentPos;

  ilutStates[ilutCurrentPos].ilutUsePalettes = IL_FALSE;
  ilutStates[ilutCurrentPos].ilutForceIntegerFormat = IL_FALSE;
  ilutStates[ilutCurrentPos].ilutOglConv = IL_FALSE;  // IL_TRUE ?
  ilutStates[ilutCurrentPos].ilutDXTCFormat = 0;
  ilutStates[ilutCurrentPos].ilutUseS3TC = IL_FALSE;
  ilutStates[ilutCurrentPos].ilutGenS3TC = IL_FALSE;
  ilutStates[ilutCurrentPos].ilutAutodetectTextureTarget = IL_FALSE;
  ilutStates[ilutCurrentPos].MaxTexW = 256;
  ilutStates[ilutCurrentPos].MaxTexH = 256;
  ilutStates[ilutCurrentPos].MaxTexD = 1;
  ilutStates[ilutCurrentPos].D3DMipLevels = 0;
  ilutStates[ilutCurrentPos].D3DPool = 0;
  ilutStates[ilutCurrentPos].D3DAlphaKeyColor = -1;
}

/**
 * Sets the number of mip map levels for DirectX 8 textures (ILUT_D3D_MIPLEVELS).
 * @ingroup ilut_dx8
 */
void ILAPIENTRY ilutD3D8MipFunc(ILuint NumLevels) {
  ILUT_STATES *ilutStates = iGetTLSData()->ilutStates;
  ILuint       ilutCurrentPos = iGetTLSData()->ilutCurrentPos;
  ilutStates[ilutCurrentPos].D3DMipLevels = NumLevels;
}


ILconst_string ILAPIENTRY ilutGetString(ILenum StringName) {
  switch (StringName)
  {
    case ILUT_VENDOR:
      return _ilutVendor;
    //changed 2003-09-04
    case ILUT_VERSION_NUM:
      return _ilutVersion;
    default:
      iSetError(ILUT_INVALID_PARAM);
      break;
  }
  return NULL;
}


ILboolean ILAPIENTRY ilutEnable(ILenum Mode) {
  ILboolean Result;
  Result = ilutAble(Mode, IL_TRUE);
  return Result;
}


ILboolean ILAPIENTRY ilutDisable(ILenum Mode) {
  ILboolean Result;
  Result = ilutAble(Mode, IL_FALSE);
  return Result;
}


ILboolean ilutAble(ILenum Mode, ILboolean Flag) {
  ILUT_STATES *ilutStates = iGetTLSData()->ilutStates;
  ILuint       ilutCurrentPos = iGetTLSData()->ilutCurrentPos;
  switch (Mode) {
    case ILUT_PALETTE_MODE:
      ilutStates[ilutCurrentPos].ilutUsePalettes = Flag;
      break;

    case ILUT_FORCE_INTEGER_FORMAT:
      ilutStates[ilutCurrentPos].ilutForceIntegerFormat = Flag;
      break;

    case ILUT_OPENGL_CONV:
      ilutStates[ilutCurrentPos].ilutOglConv = Flag;
      break;

    case ILUT_GL_USE_S3TC:
      ilutStates[ilutCurrentPos].ilutUseS3TC = Flag;
      break;

    case ILUT_GL_GEN_S3TC:
      ilutStates[ilutCurrentPos].ilutGenS3TC = Flag;
      break;

    case ILUT_GL_AUTODETECT_TEXTURE_TARGET:
      ilutStates[ilutCurrentPos].ilutAutodetectTextureTarget = Flag;
      break;


    default:
      iSetError(ILUT_INVALID_ENUM);
      return IL_FALSE;
  }

  return IL_TRUE;
}


ILboolean ILAPIENTRY ilutIsEnabled(ILenum Mode) {
  ILUT_STATES *ilutStates = iGetTLSData()->ilutStates;
  ILuint       ilutCurrentPos = iGetTLSData()->ilutCurrentPos;

  switch (Mode) {
    case ILUT_PALETTE_MODE:
      return ilutStates[ilutCurrentPos].ilutUsePalettes;

    case ILUT_FORCE_INTEGER_FORMAT:
      return ilutStates[ilutCurrentPos].ilutForceIntegerFormat;

    case ILUT_OPENGL_CONV:
      return ilutStates[ilutCurrentPos].ilutOglConv;

    case ILUT_GL_USE_S3TC:
      return ilutStates[ilutCurrentPos].ilutUseS3TC;

    case ILUT_GL_GEN_S3TC:
      return ilutStates[ilutCurrentPos].ilutGenS3TC;

    case ILUT_GL_AUTODETECT_TEXTURE_TARGET:
      return ilutStates[ilutCurrentPos].ilutAutodetectTextureTarget;


    default:
      iSetError(ILUT_INVALID_ENUM);
  }

  return IL_FALSE;
}


ILboolean ILAPIENTRY ilutIsDisabled(ILenum Mode) {
  return !ilutIsEnabled(Mode);
}


void ILAPIENTRY ilutGetBooleanv(ILenum Mode, ILboolean *Param) {
  ILUT_STATES *ilutStates = iGetTLSData()->ilutStates;
  ILuint       ilutCurrentPos = iGetTLSData()->ilutCurrentPos;

  switch (Mode)   {
    case ILUT_PALETTE_MODE:
      *Param = ilutStates[ilutCurrentPos].ilutUsePalettes;
      break;

    case ILUT_FORCE_INTEGER_FORMAT:
      *Param = ilutStates[ilutCurrentPos].ilutForceIntegerFormat;
      break;

    case ILUT_OPENGL_CONV:
      *Param = ilutStates[ilutCurrentPos].ilutOglConv;
      break;

    case ILUT_GL_USE_S3TC:
      *Param = ilutStates[ilutCurrentPos].ilutUseS3TC;
      break;

    case ILUT_GL_GEN_S3TC:
      *Param = ilutStates[ilutCurrentPos].ilutGenS3TC;
      break;

    case ILUT_GL_AUTODETECT_TEXTURE_TARGET:
      *Param = ilutStates[ilutCurrentPos].ilutAutodetectTextureTarget;
      break;

    default:
      iSetError(ILUT_INVALID_ENUM);
  }
  return;
}


ILboolean ILAPIENTRY ilutGetBoolean(ILenum Mode) {
  ILboolean Temp = IL_FALSE;
  ilutGetBooleanv(Mode, &Temp);
  return Temp;
}


void ILAPIENTRY ilutGetIntegerv(ILenum Mode, ILint *Param) {
  ILUT_STATES *ilutStates = iGetTLSData()->ilutStates;
  ILuint       ilutCurrentPos = iGetTLSData()->ilutCurrentPos;

  switch (Mode) {
    case ILUT_MAXTEX_WIDTH:
      *Param = ilutStates[ilutCurrentPos].MaxTexW;
      break;
    case ILUT_MAXTEX_HEIGHT:
      *Param = ilutStates[ilutCurrentPos].MaxTexH;
      break;
    case ILUT_MAXTEX_DEPTH:
      *Param = ilutStates[ilutCurrentPos].MaxTexD;
      break;
    case ILUT_VERSION_NUM:
      *Param = ILUT_VERSION;
      break;
    case ILUT_PALETTE_MODE: // unused
      *Param = ilutStates[ilutCurrentPos].ilutUsePalettes;
      break;
    case ILUT_FORCE_INTEGER_FORMAT: 
      *Param = ilutStates[ilutCurrentPos].ilutForceIntegerFormat;
      break;
    case ILUT_OPENGL_CONV:
      *Param = ilutStates[ilutCurrentPos].ilutOglConv;
      break;
    case ILUT_GL_USE_S3TC:
      *Param = ilutStates[ilutCurrentPos].ilutUseS3TC;
      break;
    case ILUT_GL_GEN_S3TC:
      *Param = ilutStates[ilutCurrentPos].ilutUseS3TC;
      break;
    case ILUT_S3TC_FORMAT:
      *Param = ilutStates[ilutCurrentPos].ilutDXTCFormat;
      break;
    case ILUT_GL_AUTODETECT_TEXTURE_TARGET:
      *Param = ilutStates[ilutCurrentPos].ilutAutodetectTextureTarget;
      break;
    case ILUT_D3D_MIPLEVELS:
      *Param = ilutStates[ilutCurrentPos].D3DMipLevels;
      break;
    case ILUT_D3D_ALPHA_KEY_COLOR:
      *Param = ilutStates[ilutCurrentPos].D3DAlphaKeyColor;
      break;
    case ILUT_D3D_POOL:
      *Param = ilutStates[ilutCurrentPos].D3DPool;
      break;

    default:
      iSetError(ILUT_INVALID_ENUM);
  }
  return;
}

/**
 * Returns the current value of the @a Mode.
 *
 * Valid @a Modes are:
 *
 * Mode                   | R/W | Default               | Description
 * ---------------------- | --- | --------------------- | ---------------------------------------------
 * ILUT_VERSION_NUM       | R   | ILUT_VERSION          | The version of the image library utility toolkit.
 * ILUT_MAXTEX_WIDTH      | R   | Depends               | Maximum supported texture width. (OpenGL)
 * ILUT_MAXTEX_HEIGHT     | R   | Depends               | Maximum supported texture height. (OpenGL)
 * ILUT_MAXTEX_DEPTH      | R   | Depends               | Maximum supported texture depth. (OpenGL)
 * ILUT_OPENGL_CONV       | RW  | IL_FALSE              | Use GL_RGB8 and GL_RGBA8 instead of GL_RGB and GL_RGBA, respectively.
 * ILUT_GL_USE_S3TC       | RW  | IL_FALSE              | Use S3 texture compression.
 * ILUT_GL_GEN_S3TC       | RW  | IL_FALSE              | Generate S3 texture compression data.
 * ILUT_S3TC_FORMAT       | RW  | 0                     | Texture compression format to use.
 * ILUT_DXTC_FORMAT       | RW  | 0                     | Alias of ILUT_S3TC_FORMAT.
 * ILUT_GL_AUTODETECT_TEXTURE_TARGET | RW | IL_FALSE    | Automatically use cube map targets when converting image with cube faces.
 * ILUT_D3D_MIPLEVELS     | RW  | 0                     | Number of mip map levels to generate.
 * ILUT_D3D_ALPHA_KEY_COLOR | RW | -1                   | Alpha key pixel value (ABGR?).
 * ILUT_FORCE_INTEGER_FORMAT | RW | IL_FALSE            | Convert float pixel format images to integer pixel format.
 * 
 * @ingroup ilut_state
 */
ILint ILAPIENTRY ilutGetInteger(ILenum Mode)
{
  ILint Temp = 0;
  ilutGetIntegerv(Mode, &Temp);
  return Temp;
}


void ILAPIENTRY ilutSetInteger(ILenum Mode, ILint Param) {
  ILUT_STATES *ilutStates = iGetTLSData()->ilutStates;
  ILuint       ilutCurrentPos = iGetTLSData()->ilutCurrentPos;

  switch (Mode) {
    case ILUT_S3TC_FORMAT:
      if (Param >= IL_DXT1 && Param <= IL_DXT5) {
        ilutStates[ilutCurrentPos].ilutDXTCFormat = Param;
      }
      break;

    case ILUT_MAXTEX_WIDTH:
      if (Param >= 1) {
        ilutStates[ilutCurrentPos].MaxTexW = Param;
      }
      break;

    case ILUT_MAXTEX_HEIGHT:
      if (Param >= 1) {
        ilutStates[ilutCurrentPos].MaxTexH = Param;
      }
      break;

    case ILUT_MAXTEX_DEPTH:
      if (Param >= 1) {
        ilutStates[ilutCurrentPos].MaxTexD = Param;
      }
      break;

    case ILUT_GL_USE_S3TC:
      if (Param == IL_TRUE || Param == IL_FALSE) {
        ilutStates[ilutCurrentPos].ilutUseS3TC = (ILboolean)Param;
      }
      break;

    case ILUT_GL_GEN_S3TC:
      if (Param == IL_TRUE || Param == IL_FALSE) {
        ilutStates[ilutCurrentPos].ilutGenS3TC = (ILboolean)Param;
      }
      break;

    case ILUT_GL_AUTODETECT_TEXTURE_TARGET:
      if (Param == IL_TRUE || Param == IL_FALSE) {
        ilutStates[ilutCurrentPos].ilutAutodetectTextureTarget = (ILboolean)Param;
      }
      break;

    case ILUT_D3D_MIPLEVELS:
      if (Param >= 0) {
        ilutStates[ilutCurrentPos].D3DMipLevels = Param;
      }
      break;

    case ILUT_D3D_ALPHA_KEY_COLOR:
      ilutStates[ilutCurrentPos].D3DAlphaKeyColor = Param;
      break;

    case ILUT_D3D_POOL:
      if (Param >= 0 && Param <= 2) {
        ilutStates[ilutCurrentPos].D3DPool = Param;
      }
      break;

    default:
      iSetError(ILUT_INVALID_ENUM);
  }

  iSetError(IL_INVALID_PARAM);  // Parameter not in valid bounds.
}

/** 
 * Pushes a new set of modes and attributes onto the state stack.
 * @param  Bits [description]
 * @return      [description]
 * @ingroup ilut_state
 */
void ILAPIENTRY ilutPushAttrib(ILuint Bits) {
  // Should we check here to see if ilCurrentPos is negative?
  ILUT_TLS_DATA *tls = iGetTLSData();

  if (tls->ilutCurrentPos >= ILUT_ATTRIB_STACK_MAX - 1) {
    tls->ilutCurrentPos = ILUT_ATTRIB_STACK_MAX - 1;
    iSetError(ILUT_STACK_OVERFLOW);
    return;
  }

  tls->ilutCurrentPos++;

  //memcpy(&ilutStates[ilutCurrentPos], &ilutStates[ilutCurrentPos - 1], sizeof(ILUT_STATES));

  if (Bits & ILUT_OPENGL_BIT) {
    tls->ilutStates[tls->ilutCurrentPos].ilutUsePalettes = tls->ilutStates[tls->ilutCurrentPos-1].ilutUsePalettes;
    tls->ilutStates[tls->ilutCurrentPos].ilutOglConv     = tls->ilutStates[tls->ilutCurrentPos-1].ilutOglConv;
  }
  if (Bits & ILUT_D3D_BIT) {
    tls->ilutStates[tls->ilutCurrentPos].D3DMipLevels     = tls->ilutStates[tls->ilutCurrentPos-1].D3DMipLevels;
    tls->ilutStates[tls->ilutCurrentPos].D3DAlphaKeyColor = tls->ilutStates[tls->ilutCurrentPos-1].D3DAlphaKeyColor;
  }
}


/**
 * Pops the last pushed stack entry off the stack and copies the 
 * bits specified when pushed by ilutPushAttrib to the previous set of states.
 * @ingroup ilut_state
 */
void ILAPIENTRY ilutPopAttrib() {
  ILUT_TLS_DATA *tls = iGetTLSData();

  if (tls->ilutCurrentPos <= 0) {
    tls->ilutCurrentPos = 0;
    iSetError(ILUT_STACK_UNDERFLOW);
    return;
  }

  // Should we check here to see if ilutCurrentPos is too large?
  tls->ilutCurrentPos--;
}


/**
 * Initializes one specific ILUT renderer.
 * ilInit() must have been called before.
 * @ingroup ilut_setup
 * @see ilutInit
 */
ILboolean ILAPIENTRY ilutRenderer(ILenum Renderer) {
  if (Renderer > ILUT_SDL) {
    iSetError(ILUT_INVALID_VALUE);
    return IL_FALSE;
  }

  iInitThreads();

  switch (Renderer) {
    #ifdef ILUT_USE_OPENGL
    case ILUT_OPENGL:
      return ilutGLInit();
    #endif

    #ifdef ILUT_USE_ALLEGRO
    case ILUT_ALLEGRO:
      return IL_TRUE; // ilutAllegroInit();
    #endif

    #ifdef ILUT_USE_X11
    case ILUT_X11:
      return ilutXInit();
    #endif

    #ifdef ILUT_USE_WIN32
    case ILUT_WIN32:
      return ilutWin32Init();
    #endif

    #ifdef ILUT_USE_SDL
    case ILUT_SDL:
      return InitSDL();
    #endif

    #ifdef ILUT_USE_DIRECTX8
    case ILUT_DIRECT3D8:
      return ilutD3D8Init();
    #endif

    #ifdef ILUT_USE_DIRECTX9
    case ILUT_DIRECT3D9:
      return ilutD3D9Init();
    #endif
    
    #ifdef ILUT_USE_DIRECTX10
    case ILUT_DIRECT3D10:
      return ilutD3D10Init();
    #endif

    default:
      iSetError(ILUT_NOT_SUPPORTED);
  }

  return IL_FALSE;
}

/** @} */

