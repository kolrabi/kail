//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C)      2014 by Björn Paetzel
// Last modified: 2014-05-21
//
//-----------------------------------------------------------------------------

#include "il_formats.h"

typedef struct ILformatEntry {
  ILenum id;
  const char *name;
  const ILformat *format;
  struct ILformatEntry *next;
} ILformatEntry;

static ILformatEntry *FormatHead = NULL;

ILchar* iLoadExtensions = NULL;
ILchar* iSaveExtensions = NULL;

#define ADD_FORMAT(name) \
  { extern ILformat iFormat ## name; \
  iAddFormat(IL_ ## name, #name, &iFormat ## name); }

static void 
iAddFormat(ILenum id, const char *name, const ILformat *format) {
  ILformatEntry *entry = (ILformatEntry*)ialloc(sizeof(ILformatEntry));
  if (!entry) return;

  entry->id = id;
  entry->name = name;
  entry->format = format;
  entry->next = FormatHead;
  FormatHead = entry;

  iTrace("Format %-10s: Auto: %3s Load: %3s Save: %3s", 
    name, 
    format->Validate ? "yes" : "no",
    format->Load     ? "yes" : "no",
    format->Save     ? "yes" : "no"
  );
}

void 
iInitFormats() {
  ILuint saveExtLen = 1;
  ILuint loadExtLen = 1;

  ILformatEntry *entry; 

#ifndef IL_NO_BLP
  ADD_FORMAT(BLP);
#endif
#ifndef IL_NO_BMP
  ADD_FORMAT(BMP);
#endif
#ifndef IL_NO_CUT
  ADD_FORMAT(CUT);
#endif
#ifndef IL_NO_CHEAD
  ADD_FORMAT(CHEAD);
#endif
#ifndef IL_NO_DCX
  ADD_FORMAT(DCX);
#endif
#ifndef IL_NO_DDS
  ADD_FORMAT(DDS);
#endif
#ifndef IL_NO_DICOM
  ADD_FORMAT(DICOM);
#endif
#ifndef IL_NO_DOOM
  ADD_FORMAT(DOOM);
  ADD_FORMAT(DOOM_FLAT);
#endif
#ifndef IL_NO_DPX
  ADD_FORMAT(DPX);
#endif
#ifndef IL_NO_EXR
  ADD_FORMAT(EXR);
#endif
#ifndef IL_NO_GIF
  ADD_FORMAT(GIF);
#endif
#ifndef IL_NO_FITS
  ADD_FORMAT(FITS);
#endif
#ifndef IL_NO_FTX
  ADD_FORMAT(FTX);
#endif
#ifndef IL_NO_HDR
  ADD_FORMAT(HDR);
#endif
#ifndef IL_NO_ICNS
  ADD_FORMAT(ICNS);
#endif
#ifndef IL_NO_IFF
  ADD_FORMAT(IFF);
#endif
#ifndef IL_NO_ILBM
  ADD_FORMAT(ILBM);
#endif
#ifndef IL_NO_IWI
  ADD_FORMAT(IWI);
#endif
#ifndef IL_NO_JP2
  ADD_FORMAT(JP2);
#endif
#ifndef IL_NO_JPG
  ADD_FORMAT(JPG);
#endif
#ifndef IL_NO_LIF
  ADD_FORMAT(LIF);
#endif
#ifndef IL_NO_MDL
  ADD_FORMAT(MDL);
#endif
#ifndef IL_NO_MNG
  ADD_FORMAT(MNG);
#endif
#ifndef IL_NO_MP3
  ADD_FORMAT(MP3);
#endif
#ifndef IL_NO_PCD
  ADD_FORMAT(PCD);
#endif
#ifndef IL_NO_PCX
  ADD_FORMAT(PCX);
#endif
#ifndef IL_NO_PIC
  ADD_FORMAT(PIC);
#endif
#ifndef IL_NO_PIX
  ADD_FORMAT(PIX);
#endif
#ifndef IL_NO_PNG
  ADD_FORMAT(PNG);
#endif
#ifndef IL_NO_PNM
  ADD_FORMAT(PNM_PPM); // used to be IL_PNM
  ADD_FORMAT(PNM_PBM);
  ADD_FORMAT(PNM_PGM);
#endif
#ifndef IL_NO_PSD
  ADD_FORMAT(PSD);
#endif
#ifndef IL_NO_PSP
  ADD_FORMAT(PSP);
#endif
#ifndef IL_NO_PXR
  ADD_FORMAT(PXR);
#endif
#ifndef IL_NO_RAW
  ADD_FORMAT(RAW);
#endif
#ifndef IL_NO_ROT
  ADD_FORMAT(ROT);
#endif
#ifndef IL_NO_SGI
  ADD_FORMAT(SGI);
#endif
#ifndef IL_NO_SUN
  ADD_FORMAT(SUN);
#endif
#ifndef IL_NO_TEXTURE
  ADD_FORMAT(TEXTURE);
#endif
#ifndef IL_NO_TPL
  ADD_FORMAT(TPL);
#endif
#ifndef IL_NO_TIF
  ADD_FORMAT(TIF);
#endif
#ifndef IL_NO_VTF
  ADD_FORMAT(VTF);
#endif
#ifndef IL_NO_WAL
  ADD_FORMAT(WAL);
#endif
#ifndef IL_NO_WBMP
  ADD_FORMAT(WBMP);
#endif
#ifndef IL_NO_XPM
  ADD_FORMAT(XPM);
#endif

  ADD_FORMAT(EXIF);

  // FIXME: JASC_PAL and HALO_PAL use the same extension!
  ADD_FORMAT(JASC_PAL);
  ADD_FORMAT(HALO_PAL);
  ADD_FORMAT(ACT_PAL);
  ADD_FORMAT(COL_PAL);
  ADD_FORMAT(PLT_PAL);

  // Some file types have a weak signature, so we test for these formats 
  // after checking for most other formats
#ifndef IL_NO_ICO
  ADD_FORMAT(ICO);
#endif

  //moved tga to end of list because it has no magic number
  //in header to assure that this is really a tga... (20040218)
#ifndef IL_NO_TGA
  ADD_FORMAT(TGA);
#endif

  entry = FormatHead; 
  while (entry) {
    ILconst_string *formatExt = entry->format->Exts;
    while (*formatExt) {
      if (entry->format->Load)
        loadExtLen += 1 + iStrLen(*formatExt);

      if (entry->format->Save)
        saveExtLen += 1 + iStrLen(*formatExt);

      formatExt++;
    }
    entry = entry->next;
  }

  iLoadExtensions = (ILchar*)ialloc(loadExtLen * sizeof(ILchar));
  imemclear(iLoadExtensions, loadExtLen);

  iSaveExtensions = (ILchar*)ialloc(saveExtLen * sizeof(ILchar));
  imemclear(iSaveExtensions, saveExtLen);

  entry = FormatHead;
  while (entry) {
    ILconst_string *formatExt = entry->format->Exts;
    while (*formatExt) {
      if (entry->format->Load) {
        if (iLoadExtensions[0]) iStrCat(iLoadExtensions, IL_TEXT(" "));
        iStrCat(iLoadExtensions, *formatExt);
      }

      if (entry->format->Save) {
        if (iSaveExtensions[0]) iStrCat(iSaveExtensions, IL_TEXT(" "));
        iStrCat(iSaveExtensions, *formatExt);
      }

      formatExt++;
    }
    entry = entry->next;
  }

  iTrace("---- Loadable extensions: " IL_SFMT, iLoadExtensions);
  iTrace("---- Savable extensions:  " IL_SFMT, iSaveExtensions);
}

void 
iDeinitFormats() {
  ILformatEntry *entry = FormatHead; 
  while (entry) {
    ILformatEntry *next = entry->next; 
    ifree(entry);
    entry = next;
  }
  FormatHead = NULL;

  ifree(iLoadExtensions);
  ifree(iSaveExtensions);
}

const ILformat *
iGetFormat(
  ILenum id
) {
  ILformatEntry *entry = FormatHead; 
  while (entry) {
    if (entry->id == id)
      return entry->format;

    entry = entry->next;
  }
  iTrace("**** unknown format id");
  return NULL;
}

ILenum
iIdentifyFormat(SIO *io) {
  ILformatEntry *entry = FormatHead; 
  ILuint pos;

  if (!io) return IL_TYPE_UNKNOWN;

  pos = SIOtell(io);

  while (entry) {
    if (entry->format->Validate && entry->format->Validate(io))
      return entry->id;

    SIOseek(io, pos, IL_SEEK_SET);
    entry = entry->next;
  }
  iTrace("**** unknown format");
  return IL_TYPE_UNKNOWN;
}

ILenum
iIdentifyFormatExt(ILconst_string Ext) {
  ILformatEntry *entry = FormatHead; 

  if (!Ext) return IL_TYPE_UNKNOWN;

  while (entry) {
    ILconst_string *formatExt = entry->format->Exts;
    while (*formatExt) {
      if (!iStrCmp(Ext, *formatExt))
        return entry->id;
      formatExt++;
    }
    entry = entry->next;
  }
  iTrace("**** unknown extension");
  return IL_TYPE_UNKNOWN;
} 
