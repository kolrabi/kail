//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 01/04/2009
//
// Filename: src-IL/src/il_xpm.c
//
// Description: Reads from an .xpm file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_XPM
#include <ctype.h>

// Global variables
typedef ILubyte XpmPixel[4];

#define XPM_MAX_CHAR_PER_PIXEL 3

//If this is defined, only xpm files with 1 char/pixel
//can be loaded. They load somewhat faster then, though
//(not much).
//#define XPM_DONT_USE_HASHTABLE


static ILuint XpmGetsInternal(SIO* io, char *Buffer, ILuint MaxLen) {
  ILuint  i = 0;
  ILint Current;

  if (SIOeof(io))
    return 0;

  while ((Current = SIOgetc(io)) != IL_EOF && i < MaxLen - 1) {
    if (Current == IL_EOF)
      return 0;
    if (Current == '\n') //unix line ending
      break;

    if (Current == '\r') { //dos/mac line ending
      Current = SIOgetc(io);
      if (Current == '\n') //dos line ending
        break;

      if (Current == IL_EOF)
        break;

      Buffer[i++] = (char)Current;
      continue;
    }
    Buffer[i++] = (char)Current;
  }

  Buffer[i++] = 0;

  return i;
}


// Internal function to get the header and check it.
static ILboolean iIsValidXpm(SIO* io) {
  char    Buffer[10];
  ILuint  Pos = SIOtell(io);

  XpmGetsInternal(io, Buffer, 10);
  SIOseek(io, Pos, IL_SEEK_SET);  // Restore position

  if (strncmp("/* XPM */", Buffer, iCharStrLen("/* XPM */")))
    return IL_FALSE;
  return IL_TRUE;
}


#ifndef XPM_DONT_USE_HASHTABLE

//The following hash table code was inspired by the xpm
//loading code of xv, one of the best image viewers of X11

//For xpm files with more than one character/pixel, it is
//impractical to use a simple lookup table for the
//character-to-color mapping (because the table requires
//2^(chars/pixel) entries, this is quite big).
//Because of that, a hash table is used for the mapping.
//The hash table has 257 entries, and collisions are
//resolved by chaining.

//257 is the smallest prime > 256
#define XPM_HASH_LEN 257

#include "pack_push.h"
typedef struct XPMHASHENTRY
{
  ILubyte ColourName[XPM_MAX_CHAR_PER_PIXEL];
  XpmPixel ColourValue;
  struct XPMHASHENTRY *Next;
} XPMHASHENTRY;
#include "pack_pop.h"


static ILuint XpmHash(const char* name, ILuint len)
{
  ILuint i, sum;
  for (sum = i = 0; i < len; ++i)
    sum += (ILubyte)name[i];
  return sum % XPM_HASH_LEN;
}


static XPMHASHENTRY** XpmCreateHashTable()
{
  XPMHASHENTRY** Table =
    (XPMHASHENTRY**)ialloc(XPM_HASH_LEN*sizeof(XPMHASHENTRY*));
  if (Table != NULL)
    memset(Table, 0, XPM_HASH_LEN*sizeof(XPMHASHENTRY*));
  return Table;
}


static void XpmDestroyHashTable(XPMHASHENTRY **Table)
{
  ILint i;
  XPMHASHENTRY* Entry;

  for (i = 0; i < XPM_HASH_LEN; ++i) {
    while (Table[i] != NULL) {
      Entry = Table[i]->Next;
      ifree(Table[i]);
      Table[i] = Entry;
    }
  }

  ifree(Table);
}


static void XpmInsertEntry(XPMHASHENTRY **Table, const char* Name, ILuint Len, XpmPixel Colour)
{
  XPMHASHENTRY* NewEntry;
  ILuint Index;
  Index = XpmHash(Name, Len);

  NewEntry = (XPMHASHENTRY*)ialloc(sizeof(XPMHASHENTRY));
  if (NewEntry != NULL) {
    NewEntry->Next = Table[Index];
    memcpy(NewEntry->ColourName, Name, Len);
    memcpy(NewEntry->ColourValue, Colour, sizeof(XpmPixel));
    Table[Index] = NewEntry;
  }
}


static void XpmGetEntry(XPMHASHENTRY **Table, const char* Name, ILuint Len, XpmPixel Colour)
{
  XPMHASHENTRY* Entry;
  ILuint Index;

  Index = XpmHash(Name, Len);
  Entry = Table[Index];
  while (Entry != NULL && strncmp((char*)(Entry->ColourName), (char*)Name, Len) != 0)
    Entry = Entry->Next;

  if (Entry != NULL)
    memcpy(Colour, Entry->ColourValue, sizeof(XpmPixel));
}

#endif //XPM_DONT_USE_HASHTABLE


static ILuint XpmGets(SIO* io, char *Buffer, ILuint MaxLen)
{
  ILuint    Size, i, j;
  ILboolean NotComment = IL_FALSE, InsideComment = IL_FALSE;

  do {
    Size = XpmGetsInternal(io, Buffer, MaxLen);
    if (Size == 0)
      return 0;

    //skip leading whitespace (sometimes there's whitespace
    //before a comment or before the pixel data)

    for(i = 0; i < Size && isspace(Buffer[i]); ++i) ;
    Size = Size - i;
    for(j = 0; j < Size; ++j)
      Buffer[j] = Buffer[j + i];

    if (Size == 0)
      continue;

    if (Buffer[0] == '/' && Buffer[1] == '*') {
      for (i = 2; i < Size; i++) {
        if (Buffer[i] == '*' && Buffer[i+1] == '/') {
          break;
        }
      }
      if (i >= Size)
        InsideComment = IL_TRUE;
    }
    else if (InsideComment) {
      for (i = 0; i < Size; i++) {
        if (Buffer[i] == '*' && Buffer[i+1] == '/') {
          break;
        }
      }
      if (i < Size)
        InsideComment = IL_FALSE;
    }
    else {
      NotComment = IL_TRUE;
    }
  } while (!NotComment);

  return Size;
}


static ILuint XpmGetInt(const char *Buffer, ILuint Size, ILuint *Position)
{
  char    Buff[1024];
  ILuint    i, j;
  ILboolean IsInNum = IL_FALSE;

  for (i = *Position, j = 0; i < Size; i++) {
    if (isdigit(Buffer[i])) {
      IsInNum = IL_TRUE;
      Buff[j++] = Buffer[i];
    }
    else {
      if (IsInNum) {
        Buff[j] = 0;
        *Position = i;
        return (ILuint)atoi(Buff);
      }
    }
  }

  return 0;
}


static ILboolean XpmPredefCol(const char *Buff, XpmPixel *Colour)
{
  ILuint len;
  ILint val = 128;

  if (!iCharStrICmp(Buff, "none")) {
    (*Colour)[0] = 0;
    (*Colour)[1] = 0;
    (*Colour)[2] = 0;
    (*Colour)[3] = 0;
    return IL_TRUE;
  }

  (*Colour)[3] = 255;

  if (!iCharStrICmp(Buff, "black")) {
    (*Colour)[0] = 0;
    (*Colour)[1] = 0;
    (*Colour)[2] = 0;
    return IL_TRUE;
  }
  if (!iCharStrICmp(Buff, "white")) {
    (*Colour)[0] = 255;
    (*Colour)[1] = 255;
    (*Colour)[2] = 255;
    return IL_TRUE;
  }
  if (!iCharStrICmp(Buff, "red")) {
    (*Colour)[0] = 255;
    (*Colour)[1] = 0;
    (*Colour)[2] = 0;
    return IL_TRUE;
  }
  if (!iCharStrICmp(Buff, "green")) {
    (*Colour)[0] = 0;
    (*Colour)[1] = 255;
    (*Colour)[2] = 0;
    return IL_TRUE;
  }
  if (!iCharStrICmp(Buff, "blue")) {
    (*Colour)[0] = 0;
    (*Colour)[1] = 0;
    (*Colour)[2] = 255;
    return IL_TRUE;
  }
  if (!iCharStrICmp(Buff, "yellow")) {
    (*Colour)[0] = 255;
    (*Colour)[1] = 255;
    (*Colour)[2] = 0;
    return IL_TRUE;
  }
  if (!iCharStrICmp(Buff, "cyan")) {
    (*Colour)[0] = 0;
    (*Colour)[1] = 255;
    (*Colour)[2] = 255;
    return IL_TRUE;
  }
  if (!iCharStrICmp(Buff, "gray")) {
    (*Colour)[0] = 128;
    (*Colour)[1] = 128;
    (*Colour)[2] = 128;
    return IL_TRUE;
  }

  //check for grayXXX codes (added 20040218)
  len = iCharStrLen(Buff);
  if (len >= 4) {
    if (Buff[0] == 'g' || Buff[0] == 'G'
      || Buff[1] == 'r' || Buff[1] == 'R'
      || Buff[2] == 'a' || Buff[2] == 'A'
      || Buff[3] == 'y' || Buff[3] == 'Y') {
      if (isdigit(Buff[4])) { // isdigit returns false on '\0'
        val = Buff[4] - '0';
        if (isdigit(Buff[5])) {
          val = val*10 + Buff[5] - '0';
          if (isdigit(Buff[6]))
            val = val*10 + Buff[6] - '0';
        }
        val = (255*val)/100;
      }
      (*Colour)[0] = (ILubyte)val;
      (*Colour)[1] = (ILubyte)val;
      (*Colour)[2] = (ILubyte)val;
      return IL_TRUE;
    }
  }


  // Unknown colour string, so use black
  // (changed 20040218)
  (*Colour)[0] = 0;
  (*Colour)[1] = 0;
  (*Colour)[2] = 0;

  return IL_FALSE;
}


#ifndef XPM_DONT_USE_HASHTABLE
static ILboolean XpmGetColour(const char *Buffer, ILuint Size, ILuint Len, XPMHASHENTRY **Table)
#else
static ILboolean XpmGetColour(const char *Buffer, ILuint Size, ILuint Len, XpmPixel* Colours)
#endif
{
  ILuint    i = 0, j, strLen = 0;

  XpmPixel  Colour;
  char      Name[XPM_MAX_CHAR_PER_PIXEL];

  for ( ; i < Size; i++) {
    if (Buffer[i] == '\"')
      break;
  }
  i++;  // Skip the quotes.

  if (i >= Size)
    return IL_FALSE;

  // Get the characters.
  for (j = 0; j < Len; ++j) {
    Name[j] = Buffer[i++];
  }

  // Skip to the colour definition.
  for ( ; i < Size; i++) {
    if (Buffer[i] == 'c')
      break;
  }
  i++;  // Skip the 'c'.

  if (i >= Size || Buffer[i] != ' ') { // no 'c' found...assume black
#ifndef XPM_DONT_USE_HASHTABLE
    memset(Colour, 0, sizeof(Colour));
    Colour[3] = 255;
    XpmInsertEntry(Table, Name, Len, Colour);
#else
    memset(Colours[Name[0]], 0, sizeof(Colour));
    Colours[Name[0]][3] = 255;
#endif
    return IL_TRUE;
  }

  for ( ; i < Size; i++) {
    if (Buffer[i] != ' ')
      break;
  }

  if (i >= Size)
    return IL_FALSE;

  if (Buffer[i] == '#') {
    char      ColBuff[3];

    // colour string may 4 digits/color or 1 digit/color
    // (added 20040218) TODO: is isxdigit() ANSI???
    ++i;
    while (i + strLen < Size && isxdigit(Buffer[i + strLen]))
      ++strLen;

    for (j = 0; j < 3; j++) {
      if (strLen >= 10) { // 4 digits
        ColBuff[0] = Buffer[i + j*4];
        ColBuff[1] = Buffer[i + j*4 + 1];
      }
      else if (strLen >= 8) { // 3 digits
        ColBuff[0] = Buffer[i + j*3];
        ColBuff[1] = Buffer[i + j*3 + 1];
      }
      else if (strLen >= 6) { // 2 digits
        ColBuff[0] = Buffer[i + j*2];
        ColBuff[1] = Buffer[i + j*2 + 1];
      }
      else if(j < strLen) { // 1 digit, strLen >= 1
        ColBuff[0] = Buffer[i + j];
        ColBuff[1] = 0;
      }

      ColBuff[2] = 0; // add terminating '\0' char
      Colour[j] = (ILubyte)strtol(ColBuff, NULL, 16);
    }
    Colour[3] = 255;  // Full alpha.
  }
  else {
    char      Buff[1024];
    for (j = 0; i < Size; i++) {
      if (!isalnum(Buffer[i]))
        break;
      Buff[j++] = Buffer[i];
    }
    Buff[j] = 0;

    if (i >= Size)
      return IL_FALSE;

    if (!XpmPredefCol(Buff, &Colour))
      return IL_FALSE;
  }


#ifndef XPM_DONT_USE_HASHTABLE
  XpmInsertEntry(Table, Name, Len, Colour);
#else
  memcpy(Colours[Name[0]], Colour, sizeof(Colour));
#endif
  return IL_TRUE;
}


static ILboolean iLoadXpmInternal(ILimage *Image) {
#define BUFFER_SIZE 2000
  char      Buffer[BUFFER_SIZE];
  ILubyte   *Data;
  ILuint      Size, Pos, Width, Height, NumColours, i, x, y;
  ILuint      CharsPerPixel;

#ifndef XPM_DONT_USE_HASHTABLE
  XPMHASHENTRY  **HashTable;
#else
  XpmPixel  *Colours;
  ILuint    Offset;
#endif
  SIO* io = &Image->io;

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  io = &Image->io;

  Size = XpmGetsInternal(io, Buffer, BUFFER_SIZE);

  if (strncmp("/* XPM */", (char*)Buffer, iCharStrLen("/* XPM */"))) {
    iSetError(IL_INVALID_FILE_HEADER);
    return IL_FALSE;
  }

  /* Size = */ XpmGets(io, Buffer, BUFFER_SIZE);
  // @TODO:  Actually check the variable name here.

  Size = XpmGets(io, Buffer, BUFFER_SIZE);
  Pos = 0;
  Width = XpmGetInt(Buffer, Size, &Pos);
  Height = XpmGetInt(Buffer, Size, &Pos);
  NumColours = XpmGetInt(Buffer, Size, &Pos);

  CharsPerPixel = XpmGetInt(Buffer, Size, &Pos);

#ifdef XPM_DONT_USE_HASHTABLE
  if (CharsPerPixel != 1) {
    iSetError(IL_FORMAT_NOT_SUPPORTED);
    return IL_FALSE;
  }
#endif

  if (CharsPerPixel > XPM_MAX_CHAR_PER_PIXEL
    || Width*CharsPerPixel > BUFFER_SIZE) 
  {
    iSetError(IL_FORMAT_NOT_SUPPORTED);
    return IL_FALSE;
  }

#ifndef XPM_DONT_USE_HASHTABLE
  HashTable = XpmCreateHashTable();
  if (HashTable == NULL)
    return IL_FALSE;
#else
  Colours = ialloc(256 * sizeof(XpmPixel));
  if (Colours == NULL)
    return IL_FALSE;
#endif

  for (i = 0; i < NumColours; i++) {
    Size = XpmGets(io, Buffer, BUFFER_SIZE);
#ifndef XPM_DONT_USE_HASHTABLE
    if (!XpmGetColour(Buffer, Size, CharsPerPixel, HashTable)) {
      XpmDestroyHashTable(HashTable);
#else
    if (!XpmGetColour(Buffer, Size, CharsPerPixel, Colours)) {
      ifree(Colours);
#endif
      return IL_FALSE;
    }
  }
  
  if (!iTexImage(Image, Width, Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL)) {
#ifndef XPM_DONT_USE_HASHTABLE
    XpmDestroyHashTable(HashTable);
#else
    ifree(Colours);
#endif
    return IL_FALSE;
  }

  Data = Image->Data;

  for (y = 0; y < Height; y++) {
    /* Size = */ XpmGets(io, Buffer, BUFFER_SIZE);
    for (x = 0; x < Width; x++) {
#ifndef XPM_DONT_USE_HASHTABLE
      XpmGetEntry(HashTable, &Buffer[1 + x*CharsPerPixel], CharsPerPixel, &Data[(x << 2)]);
#else
      Offset = (x << 2);
      Data[Offset + 0] = Colours[Buffer[x + 1]][0];
      Data[Offset + 1] = Colours[Buffer[x + 1]][1];
      Data[Offset + 2] = Colours[Buffer[x + 1]][2];
      Data[Offset + 3] = Colours[Buffer[x + 1]][3];
#endif
    }

    Data += Image->Bps;
  }

  //added 20040218
  Image->Origin = IL_ORIGIN_UPPER_LEFT;

#ifndef XPM_DONT_USE_HASHTABLE
  XpmDestroyHashTable(HashTable);
#else
  ifree(Colours);
#endif
  return IL_TRUE;

#undef BUFFER_SIZE
}

static ILboolean iSaveXpmInternal(ILimage *Image) {
  ILimage *   ConvertedImage;
  SIO *       io;
  const char *InternalName = "IL_IMAGE";
  const char *Name;
  char        tmp[256];
  ILuint      i, x, y;
  ILpal *     Pal;

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  io = &Image->io;

  if (Image->Format == IL_COLOUR_INDEX && Image->Type == IL_UNSIGNED_BYTE)
    ConvertedImage = Image;
  else
    ConvertedImage = iConvertImage(Image, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE);

  if (ConvertedImage == NULL) {
    return IL_FALSE;
  }

  Pal = iConvertPal(&ConvertedImage->Pal, IL_PAL_RGB24);
  if (!Pal) {
    if (ConvertedImage != Image)
      iCloseImage(ConvertedImage);

    return IL_FALSE;
  }

  Name = iGetString(Image, IL_META_DOCUMENT_NAME);
  if (Name == NULL)
    Name = InternalName;

  // header
  SIOputs(io, "/* XPM */\n");

  SIOputs(io, "static char * ");
  SIOputs(io, Name);
  SIOputs(io, "[] = {\n");

  snprintf(tmp, sizeof(tmp), "  \"%u %u 256 2\", \n", ConvertedImage->Width, ConvertedImage->Height);
  SIOputs(io, tmp);

  for (i=0; i<256; i++) {
    snprintf(tmp, sizeof(tmp), "  \"%02x c #%02x%02x%02x\", \n", 
      i,
      Pal->Palette[i*3+0],
      Pal->Palette[i*3+1],
      Pal->Palette[i*3+2]
    );
    SIOputs(io, tmp);
  }
  iClosePal(Pal);

  for (y=0; y<ConvertedImage->Height; y++) {
    ILuint yOffset = y;
    if ( ConvertedImage->Origin != IL_ORIGIN_UPPER_LEFT)
      yOffset = (ConvertedImage->Height - y - 1);

    SIOputs(io, "  \"");
    for (x=0; x<ConvertedImage->Width; x++) {
      snprintf(tmp, sizeof(tmp), "%02x", ConvertedImage->Data[x+yOffset*ConvertedImage->Bps]);
      SIOputs(io, tmp);
    }
    if (y+1 < ConvertedImage->Height)
      SIOputs(io, "\",\n");
    else
      SIOputs(io, "\"\n");
  }
  SIOputs(io, "}\n");

  if (ConvertedImage != Image)
    iCloseImage(ConvertedImage);

  return IL_TRUE;

#undef BUFFER_SIZE
}

static ILconst_string iFormatExtsXPM[] = { 
  IL_TEXT("xpm"), 
  NULL 
};

ILformat iFormatXPM = { 
  /* .Validate = */ iIsValidXpm, 
  /* .Load     = */ iLoadXpmInternal, 
  /* .Save     = */ iSaveXpmInternal, 
  /* .Exts     = */ iFormatExtsXPM
};

#endif//IL_NO_XPM

