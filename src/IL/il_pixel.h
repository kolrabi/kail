#ifndef IL_PIXEL_H
#define IL_PIXEL_H

#include "il_internal.h"

#ifdef CLAMP_FLOAT 
#define IL_CLAMP_FLOAT(x) IL_CLAMP(x)
#else
#define IL_CLAMP_FLOAT(x) (x)
#endif

#define iPixelConv1f(Tfrom, From, MaxFrom, Tto, To, MaxTo) \
    ((Tto*)To)[0] = IL_CLAMP_FLOAT( ((Tfrom*)From)[0] / (float)MaxFrom) * (MaxTo);

#define iPixelConv2f(Tfrom, From, MaxFrom, Tto, To, MaxTo) \
    ((Tto*)To)[0] = IL_CLAMP_FLOAT( ((Tfrom*)From)[0] / (float)MaxFrom) * (MaxTo); \
    ((Tto*)To)[1] = IL_CLAMP_FLOAT( ((Tfrom*)From)[1] / (float)MaxFrom) * (MaxTo);

#define iPixelConv3f(Tfrom, From, MaxFrom, Tto, To, MaxTo) \
    ((Tto*)To)[0] = IL_CLAMP_FLOAT( ((Tfrom*)From)[0] / (float)MaxFrom) * (MaxTo); \
    ((Tto*)To)[1] = IL_CLAMP_FLOAT( ((Tfrom*)From)[1] / (float)MaxFrom) * (MaxTo); \
    ((Tto*)To)[2] = IL_CLAMP_FLOAT( ((Tfrom*)From)[2] / (float)MaxFrom) * (MaxTo);

#define iPixelConv3Swapf(Tfrom, From, MaxFrom, Tto, To, MaxTo) \
    ((Tto*)To)[2] = IL_CLAMP_FLOAT( ((Tfrom*)From)[0] / (float)MaxFrom) * (MaxTo); \
    ((Tto*)To)[1] = IL_CLAMP_FLOAT( ((Tfrom*)From)[1] / (float)MaxFrom) * (MaxTo); \
    ((Tto*)To)[0] = IL_CLAMP_FLOAT( ((Tfrom*)From)[2] / (float)MaxFrom) * (MaxTo);

#define iPixelConv4f(Tfrom, From, MaxFrom, Tto, To, MaxTo) \
    ((Tto*)To)[0] = IL_CLAMP_FLOAT( ((Tfrom*)From)[0] / (float)MaxFrom) * (MaxTo); \
    ((Tto*)To)[1] = IL_CLAMP_FLOAT( ((Tfrom*)From)[1] / (float)MaxFrom) * (MaxTo); \
    ((Tto*)To)[2] = IL_CLAMP_FLOAT( ((Tfrom*)From)[2] / (float)MaxFrom) * (MaxTo); \
    ((Tto*)To)[3] = IL_CLAMP_FLOAT( ((Tfrom*)From)[3] / (float)MaxFrom) * (MaxTo); 

#define iPixelConv4Swapf(Tfrom, From, MaxFrom, Tto, To, MaxTo) \
    ((Tto*)To)[2] = IL_CLAMP_FLOAT( ((Tfrom*)From)[0] / (float)MaxFrom) * (MaxTo); \
    ((Tto*)To)[1] = IL_CLAMP_FLOAT( ((Tfrom*)From)[1] / (float)MaxFrom) * (MaxTo); \
    ((Tto*)To)[0] = IL_CLAMP_FLOAT( ((Tfrom*)From)[2] / (float)MaxFrom) * (MaxTo); \
    ((Tto*)To)[3] = IL_CLAMP_FLOAT( ((Tfrom*)From)[3] / (float)MaxFrom) * (MaxTo); 

#define iPixelConv3Lf(Tfrom, From, MaxFrom, Tto, To, MaxTo) \
    ((Tto*)To)[0] = IL_CLAMP_FLOAT( ( ((Tfrom*)From)[0] / (float)MaxFrom) * 0.212671f + \
                                    ( ((Tfrom*)From)[1] / (float)MaxFrom) * 0.715160f + \
                                    ( ((Tfrom*)From)[2] / (float)MaxFrom) * 0.072169f   ) * (MaxTo);

#define iPixelConv3LSwapf(Tfrom, From, MaxFrom, Tto, To, MaxTo) \
    ((Tto*)To)[0] = IL_CLAMP_FLOAT( ( ((Tfrom*)From)[2] / (float)MaxFrom) * 0.212671f + \
                                    ( ((Tfrom*)From)[1] / (float)MaxFrom) * 0.715160f + \
                                    ( ((Tfrom*)From)[0] / (float)MaxFrom) * 0.072169f   ) * (MaxTo);

#define iPixelConv4LAf(Tfrom, From, MaxFrom, Tto, To, MaxTo) \
    ((Tto*)To)[0] = IL_CLAMP_FLOAT( ( ((Tfrom*)From)[0] / (float)MaxFrom) * 0.212671f + \
                                    ( ((Tfrom*)From)[1] / (float)MaxFrom) * 0.715160f + \
                                    ( ((Tfrom*)From)[2] / (float)MaxFrom) * 0.072169f   ) * (MaxTo); \
    ((Tto*)To)[1] = IL_CLAMP_FLOAT( ((Tfrom*)From)[3] / (float)MaxFrom) * (MaxTo); 

#define iPixelConv4LASwapf(Tfrom, From, MaxFrom, Tto, To, MaxTo) \
    ((Tto*)To)[0] = IL_CLAMP_FLOAT( ( ((Tfrom*)From)[2] / (float)MaxFrom) * 0.212671f + \
                                    ( ((Tfrom*)From)[1] / (float)MaxFrom) * 0.715160f + \
                                    ( ((Tfrom*)From)[0] / (float)MaxFrom) * 0.072169f   ) * (MaxTo); \
    ((Tto*)To)[1] = IL_CLAMP_FLOAT( ((Tfrom*)From)[3] / (float)MaxFrom) * (MaxTo); 

#endif