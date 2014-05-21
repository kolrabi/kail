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
  const ILformat *format;
  struct ILformatEntry *next;
} ILformatEntry;

static ILformatEntry *FormatHead = NULL;

#define ADD_FORMAT(name) \
  extern ILformat iFormat ## name; \
  iAddFormat(IL_ ## name, &iFormat ## name)

static void 
iAddFormat(ILenum id, const ILformat *format) {
  ILformatEntry *entry = (ILformatEntry*)ialloc(sizeof(ILformatEntry));
  if (!entry) return;

  entry->id = id;
  entry->format = format;
  entry->next = FormatHead;
  FormatHead = entry;
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
#ifndef IL_NO_GIF
  ADD_FORMAT(GIF);
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

  ILformatEntry *entry = FormatHead; 
  while (entry) {
    if (entry->format->Validate && entry->format->Validate(io))
      return entry->id;

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