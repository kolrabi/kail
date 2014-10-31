//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C)      2014 by Björn Paetzel
// Last modified: 2014-05-21
//
//-----------------------------------------------------------------------------
#ifndef IL_FORMATS_H
#define IL_FORMATS_H

#include "il_internal.h"

typedef ILboolean (*ILformatLoadFunc)(ILimage *);
typedef ILboolean (*ILformatSaveFunc)(ILimage *);
typedef ILboolean (*ILformatValidateFunc)(SIO *);

typedef struct {
  ILformatValidateFunc  Validate;
  ILformatLoadFunc      Load;
  ILformatSaveFunc      Save;
  ILconst_string *      Exts;
} ILformat;

const ILformat *iGetFormat(ILenum id);
ILenum iIdentifyFormat(SIO *io);
ILenum iIdentifyFormatExt(ILconst_string ext);

void iInitFormats(void);
void iDeinitFormats(void);

extern ILchar* iLoadExtensions;
extern ILchar* iSaveExtensions;

extern ILformat iFormatBLP;
extern ILformat iFormatBMP;
extern ILformat iFormatCUT;
extern ILformat iFormatDCX;
extern ILformat iFormatDDS;
extern ILformat iFormatDICOM;
extern ILformat iFormatDOOM;
extern ILformat iFormatDOOM_FLAT;
extern ILformat iFormatDPX;
extern ILformat iFormatEXIF;
extern ILformat iFormatEXR;
extern ILformat iFormatFITS;
extern ILformat iFormatFTX;
extern ILformat iFormatGIF;
extern ILformat iFormatHDR;
extern ILformat iFormatCHEAD;
extern ILformat iFormatICNS;
extern ILformat iFormatICO;
extern ILformat iFormatIFF;
extern ILformat iFormatILBM;
extern ILformat iFormatIWI;
extern ILformat iFormatJP2;
extern ILformat iFormatJPG;
extern ILformat iFormatLIF;
extern ILformat iFormatMDL;
extern ILformat iFormatMNG;
extern ILformat iFormatMP3;
extern ILformat iFormatPCD;
extern ILformat iFormatPCX;
extern ILformat iFormatPIC;
extern ILformat iFormatPIX;
extern ILformat iFormatPNG;
extern ILformat iFormatPNM_PGM;
extern ILformat iFormatPNM_PBM;
extern ILformat iFormatPNM_PPM;
extern ILformat iFormatPSD;
extern ILformat iFormatPSP;
extern ILformat iFormatPXR;
extern ILformat iFormatRAW;
extern ILformat iFormatROT;
extern ILformat iFormatSGI;
extern ILformat iFormatSUN;
extern ILformat iFormatTGA;
extern ILformat iFormatTEXTURE;
extern ILformat iFormatTIF;
extern ILformat iFormatTPL;
extern ILformat iFormatUTX;
extern ILformat iFormatVTF;
extern ILformat iFormatWAL;
extern ILformat iFormatWBMP;
extern ILformat iFormatXPM;

extern ILformat iFormatACT_PAL;
extern ILformat iFormatCOL_PAL;
extern ILformat iFormatHALO_PAL;
extern ILformat iFormatJASC_PAL;
extern ILformat iFormatPLT_PAL;

#endif
