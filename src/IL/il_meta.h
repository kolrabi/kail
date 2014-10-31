#include "il_internal.h"

ILuint iGetMetaDataSize(ILenum Type, ILuint Count);
ILint iGetMetaLen(ILenum MetaID);
ILint iGetMetaLenf(ILenum MetaID);
ILboolean iEnumMetadata(ILimage *Image, ILuint Index, ILenum *IFD, ILenum *ID);
ILboolean iGetMetadata(ILimage *Image, ILenum IFD, ILenum ID, ILenum *Type, ILuint *Count, ILuint *Size, void **Data);
ILboolean iSetMetadata(ILimage *Image, ILenum IFD, ILenum ID, ILenum Type, ILuint Count, ILuint Size, const void *Data);

ILuint iGetMetaiv(ILimage *Image, ILenum MetaID, ILint *Param, ILint MaxCount);
ILboolean iSetMetaiv(ILimage *Image, ILenum MetaID, const ILint *Param);

ILuint iGetMetafv(ILimage *Image, ILenum MetaID, ILfloat *Param, ILint MaxCount);
ILboolean iSetMetafv(ILimage *Image, ILenum MetaID, const ILfloat *Param);

const void * iGetMetax(ILimage *Image, ILenum MetaID, ILuint *Size);
ILconst_string iGetMetaString(ILimage *Image, ILenum MetaID);
ILboolean iSetMetaString(ILimage *Image, ILenum MetaID, ILconst_string String);
