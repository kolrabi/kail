#ifndef IL_TYPE_H
#define IL_TYPE_H

#include "il_internal.h"

#define iTypeConvf(TFrom, From, MaxFrom, TTo, To, MaxTo) \
  ((Tto*)To)[0] = IL_CLAMP_FLOAT( ((Tfrom*)From)[0] / (float)MaxFrom) * (MaxTo);


#endif
