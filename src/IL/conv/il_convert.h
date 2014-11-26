#ifndef IL_CONV_H
#define IL_CONV_H

#include "il_internal.h"
#include "il_manip.h"

#ifdef CLAMP_DOUBLES
  #define CLAMP_DOUBLE_VALUE(f) (IL_CLAMP(f))
#else
  #define CLAMP_DOUBLE_VALUE(f) (f)
#endif

#ifdef CLAMP_FLOATS
  #define CLAMP_FLOAT_VALUE(f) (IL_CLAMP(f))
#else
  #define CLAMP_FLOAT_VALUE(f) (f)
#endif

#ifdef CLAMP_HALF
  #define CLAMP_HALF_VALUE(f) (IL_CLAMP(f))
#else
  #define CLAMP_HALF_VALUE(f) (f)
#endif

#endif
