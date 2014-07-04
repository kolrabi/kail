#include "il_internal.h"

ILuint iGetMetaDataSize(ILenum Type, ILuint Count);
ILuint iGetMetaLen(ILenum MetaID);
ILboolean iEnumMetadata(ILimage *Image, ILuint Index, ILenum *IFD, ILenum *ID);
ILboolean iGetMetadata(ILimage *Image, ILenum IFD, ILenum ID, ILenum *Type, ILuint *Count, ILuint *Size, void **Data);
ILboolean iSetMetadata(ILimage *Image, ILenum IFD, ILenum ID, ILenum Type, ILuint Count, ILuint Size, const void *Data);
ILint iGetMetai(ILimage *Image, ILenum MetaID);
ILuint iGetMetaiv(ILimage *Image, ILenum MetaID, ILint *Param);
ILboolean iSetMetaiv(ILimage *Image, ILenum MetaID, ILint *Param);
const void * iGetMetax(ILimage *Image, ILenum MetaID, ILuint *Size);
ILconst_string iGetMetaString(ILimage *Image, ILenum MetaID);
ILboolean iSetMetaString(ILimage *Image, ILenum MetaID, const char *String);
