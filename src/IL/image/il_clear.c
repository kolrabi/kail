#include "il_internal.h"

ILboolean ILAPIENTRY iClearImage(ILimage *Image)
{
  ILuint    i, c, NumBytes;
  ILubyte   Colours[32];  // Maximum is sizeof(double) * 4 = 32
  void      *ImageData, *ColourData;

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  ImageData = Image->Data;
  ColourData = Colours;
  
  NumBytes = Image->Bpp * Image->Bpc;
  iGetClear(Colours, Image->Format, Image->Type);
  
  if (Image->Format != IL_COLOUR_INDEX) {
    switch (Image->Type)
    {
      case IL_BYTE:
      case IL_UNSIGNED_BYTE:
        for (c = 0; c < NumBytes; c += Image->Bpc) {
          for (i = c; i < Image->SizeOfData; i += NumBytes) {
            ((ILubyte*)ImageData)[i] = ((ILubyte*)ColourData)[c];
          }
        }
        break;
        
      case IL_SHORT:
      case IL_UNSIGNED_SHORT:
        for (c = 0; c < NumBytes; c += Image->Bpc) {
          for (i = c; i < Image->SizeOfData; i += NumBytes) {
            ((ILushort*)ImageData)[i] = ((ILushort*)ColourData)[c/Image->Bpc];
          }
        }
        break;
        
      case IL_INT:
      case IL_UNSIGNED_INT:
        for (c = 0; c < NumBytes; c += Image->Bpc) {
          for (i = c; i < Image->SizeOfData; i += NumBytes) {
            ((ILuint*)ImageData)[i] = ((ILuint*)ColourData)[c/Image->Bpc];
          }
        }
        break;
        
      case IL_FLOAT:
        for (c = 0; c < NumBytes; c += Image->Bpc) {
          for (i = c; i < Image->SizeOfData; i += NumBytes) {
            ((ILfloat*)ImageData)[i] = ((ILfloat*)ColourData)[c/Image->Bpc];
          }
        }
        break;
      
      case IL_DOUBLE:
        for (c = 0; c < NumBytes; c += Image->Bpc) {
          for (i = c; i < Image->SizeOfData; i += NumBytes) {
            ((ILdouble*)ImageData)[i] = ((ILdouble*)ColourData)[c/Image->Bpc];
          }
        }
        break;
    }
  } else {
    // IL_COLOUR_INDEX
    for (i=0; i<Image->SizeOfData; i++) {
      ((ILubyte*)ImageData)[i] = ((ILubyte*)ColourData)[i % NumBytes];
    }
  }
  
  return IL_TRUE;
}
