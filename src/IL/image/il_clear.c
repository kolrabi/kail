#include "il_internal.h"

ILboolean ILAPIENTRY iClearImage(ILimage *Image)
{
  ILuint    i, c, NumBytes;
  ILubyte   Colours[32];  // Maximum is sizeof(double) * 4 = 32
  ILubyte   *BytePtr;
  ILushort  *ShortPtr;
  ILuint    *IntPtr;
  ILfloat   *FloatPtr;
  ILdouble  *DblPtr;

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }
  
  NumBytes = Image->Bpp * Image->Bpc;
  iGetClear(Colours, Image->Format, Image->Type);
  
  if (Image->Format != IL_COLOUR_INDEX) {
    switch (Image->Type)
    {
      case IL_BYTE:
      case IL_UNSIGNED_BYTE:
        BytePtr = (ILubyte*)Colours;
        for (c = 0; c < NumBytes; c += Image->Bpc) {
          for (i = c; i < Image->SizeOfData; i += NumBytes) {
            Image->Data[i] = BytePtr[c];
          }
        }
          break;
        
      case IL_SHORT:
      case IL_UNSIGNED_SHORT:
        ShortPtr = (ILushort*)Colours;
        for (c = 0; c < NumBytes; c += Image->Bpc) {
          for (i = c; i < Image->SizeOfData; i += NumBytes) {
            *((ILushort*)(Image->Data + i)) = ShortPtr[c / Image->Bpc];
          }
        }
          break;
        
      case IL_INT:
      case IL_UNSIGNED_INT:
        IntPtr = (ILuint*)Colours;
        for (c = 0; c < NumBytes; c += Image->Bpc) {
          for (i = c; i < Image->SizeOfData; i += NumBytes) {
            *((ILuint*)(Image->Data + i)) = IntPtr[c / Image->Bpc];
          }
        }
          break;
        
      case IL_FLOAT:
        FloatPtr = (ILfloat*)Colours;
        for (c = 0; c < NumBytes; c += Image->Bpc) {
          for (i = c; i < Image->SizeOfData; i += NumBytes) {
            *((ILfloat*)(Image->Data + i)) = FloatPtr[c / Image->Bpc];
          }
        }
          break;
        
      case IL_DOUBLE:
        DblPtr = (ILdouble*)Colours;
        for (c = 0; c < NumBytes; c += Image->Bpc) {
          for (i = c; i < Image->SizeOfData; i += NumBytes) {
            *((ILdouble*)(Image->Data + i)) = DblPtr[c / Image->Bpc];
          }
        }
          break;
    }
  } else {
    // IL_COLOUR_INDEX
    BytePtr = (ILubyte*)Colours;
    for (i=0; i<Image->SizeOfData; i++) {
      Image->Data[i] = BytePtr[i % NumBytes];
    }
  }
  
  return IL_TRUE;
}
