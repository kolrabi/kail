
#ifndef INTERNAL_H
#define INTERNAL_H

#include <string.h>

#ifdef _MSC_VER
  #if _MSC_VER > 1000
    #pragma once
    #pragma intrinsic(memcpy)
    #pragma intrinsic(memset)
    //pragma comment(linker, "/NODEFAULTLIB:libc")
    #define WIN32_LEAN_AND_MEAN   // Exclude rarely-used stuff from Windows headers

    #ifdef _DEBUG 
      #define _CRTDBG_MAP_ALLOC
      #include <stdlib.h>
      #ifndef _WIN32_WCE
        #include <crtdbg.h>
      #endif
    #endif
  #endif // _MSC_VER > 1000
#endif

#define _IL_BUILD_LIBRARY
#define _ILU_BUILD_LIBRARY

// Standard headers
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

// Local headers
#define _IL_BUILD_LIBRARY
#define _ILU_BUILD_LIBRARY

#include <IL/ilu.h>
#include <IL/devil_internal_exports.h>

extern ILimage *iluCurImage;


// Useful global variables
extern const ILdouble IL_PI;
extern const ILdouble IL_DEGCONV;


#ifdef ILU_INTERNAL_C
#undef NOINLINE
#undef INLINE
#define INLINE
#endif

// Internal functions

#ifndef NOINLINE
INLINE ILfloat ilCos(ILfloat Angle) {
  return (ILfloat)(cos(Angle * IL_DEGCONV));
}

INLINE ILfloat ilSin(ILfloat Angle) {
  return (ILfloat)(sin(Angle * IL_DEGCONV));
}


INLINE ILint ilRound(ILfloat Num) {
  return (ILint)(Num + 0.5); // this is truncating in away-from-0, not rounding
}
#else
ILfloat ilCos(ILfloat Angle);
ILfloat ilSin(ILfloat Angle);
ILint ilRound(ILfloat Num);
#endif



ILuint    iScaleAdvanced(ILimage *Image, ILuint Width, ILuint Height, ILenum Filter);
ILubyte * iScanFill(ILimage *Image);
ILuint    iColoursUsed(ILimage *Image);
ILboolean iCompareImage(ILimage * Image, ILimage * Original);
ILboolean iContrast(ILimage *Image, ILfloat Contrast);
ILboolean iCrop(ILimage *Image, ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth);
ILboolean iEnlargeCanvas(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILenum Placement);
ILboolean iEqualize(ILimage *Image);
ILconst_string iErrorString(ILenum Error);
ILboolean iGammaCorrect(ILimage *Image, ILfloat Gamma);
void      iInit(void);
ILboolean iInvertAlpha(ILimage *Image);
ILboolean iNegative(ILimage *Image);
ILboolean iNoisify(ILimage *Image, ILclampf Tolerance);
ILboolean iNormalize(ILimage *Image);
ILboolean iReplaceColour(ILimage *Image, ILubyte Red, ILubyte Green, ILubyte Blue, ILfloat Tolerance, const ILubyte *ClearCol);
ILboolean iRotate(ILimage *Image, ILfloat Angle);
ILboolean iScale(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILenum Filter);
ILboolean iSetLanguage(ILenum Language);
ILboolean iWave(ILimage *Image, ILfloat Angle);

#define imemclear(x,y) memset(x,0,y);


#endif//INTERNAL_H
