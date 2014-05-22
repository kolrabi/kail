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

#define ADD_FORMAT(name) \
  extern ILformat iFormat ## name; \
  iAddFormat(IL_ ## name, #name, &iFormat ## name);

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
  // TODO: ADD_FORMAT(EXR);
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
#ifndef IL_NO_TIF
  ADD_FORMAT(TIF);
#endif

  //moved tga to end of list because it has no magic number
  //in header to assure that this is really a tga... (20040218)
#ifndef IL_NO_TGA
  ADD_FORMAT(TGA);
#endif
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
  if (!io) return IL_TYPE_UNKNOWN;

  ILuint pos = SIOtell(io);

  ILformatEntry *entry = FormatHead; 
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
  if (!Ext) return IL_TYPE_UNKNOWN;

  ILformatEntry *entry = FormatHead; 
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