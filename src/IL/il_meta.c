#include "il_meta.h"

typedef struct {
  ILenum MetaID;
  ILenum ExifIFD;
  ILenum ExifID;
  ILenum Type;
  ILint Length;
} ILmetaDesc;

static ILmetaDesc MetaDescriptions[] = {
  { IL_META_MAKE,                     IL_TIFF_IFD0,     0x010F, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_MODEL,                    IL_TIFF_IFD0,     0x0110, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_DOCUMENT_NAME,            IL_TIFF_IFD0,     0x010D, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_IMAGE_DESCRIPTION,        IL_TIFF_IFD0,     0x010E, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_SOFTWARE,                 IL_TIFF_IFD0,     0x0131, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_DATETIME,                 IL_TIFF_IFD0,     0x0132, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_ARTIST,                   IL_TIFF_IFD0,     0x013B, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_HOST_COMPUTER,            IL_TIFF_IFD0,     0x013C, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_COPYRIGHT,                IL_TIFF_IFD0,     0x8298, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_SPECTRAL_SENSITIVITY,     IL_TIFF_IFD_EXIF, 0x8824, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_DATETIME_ORIGINAL,        IL_TIFF_IFD_EXIF, 0x9003, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_DATETIME_DIGITIZED,       IL_TIFF_IFD_EXIF, 0x9004, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_SUB_SECOND_TIME,          IL_TIFF_IFD_EXIF, 0x9291, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_SUB_SECOND_TIME_ORIGINAL, IL_TIFF_IFD_EXIF, 0x9292, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_SUB_SECOND_TIME_DIGITIZED,IL_TIFF_IFD_EXIF, 0x9293, IL_EXIF_TYPE_ASCII,    -1 },

  { IL_META_GPS_LATITUDE_REF,         IL_TIFF_IFD_GPS,  0x0001, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_GPS_LONGITUDE_REF,        IL_TIFF_IFD_GPS,  0x0003, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_GPS_SATELLITES,           IL_TIFF_IFD_GPS,  0x0008, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_GPS_STATUS,               IL_TIFF_IFD_GPS,  0x0009, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_GPS_MEASURE_MODE,         IL_TIFF_IFD_GPS,  0x000A, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_GPS_SPEED_REF,            IL_TIFF_IFD_GPS,  0x000C, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_GPS_TRACK_REF,            IL_TIFF_IFD_GPS,  0x000E, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_GPS_IMAGE_DIRECTION_REF,  IL_TIFF_IFD_GPS,  0x0010, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_GPS_MAP_DATUM,            IL_TIFF_IFD_GPS,  0x0012, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_GPS_DEST_LATITUDE_REF,    IL_TIFF_IFD_GPS,  0x0013, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_GPS_DEST_LONGITUDE_REF,   IL_TIFF_IFD_GPS,  0x0015, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_GPS_DEST_BEARING_REF,     IL_TIFF_IFD_GPS,  0x0017, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_GPS_DEST_DISTANCE_REF,    IL_TIFF_IFD_GPS,  0x0019, IL_EXIF_TYPE_ASCII,    -1 },
  { IL_META_GPS_DATESTAMP,            IL_TIFF_IFD_GPS,  0x001D, IL_EXIF_TYPE_ASCII,    -1 },

  { IL_META_PAGE_NUMBER,              IL_TIFF_IFD0,     0x0129, IL_EXIF_TYPE_WORD,      2 },

  { IL_META_EXPOSURE_TIME,            IL_TIFF_IFD_EXIF, 0x829A, IL_EXIF_TYPE_RATIONAL,  1 },
  { IL_META_FSTOP,                    IL_TIFF_IFD_EXIF, 0x829D, IL_EXIF_TYPE_RATIONAL,  1 },
  { IL_META_EXPOSURE_PROGRAM,         IL_TIFF_IFD_EXIF, 0x8822, IL_EXIF_TYPE_WORD,      1 },
  { IL_META_ISO_RATINGS,              IL_TIFF_IFD_EXIF, 0x8827, IL_EXIF_TYPE_WORD,     -1 },

  { IL_META_EXIF_VERSION,             IL_TIFF_IFD_EXIF, 0x9000, IL_EXIF_TYPE_BLOB,      4 },
  { IL_META_SHUTTER_SPEED,            IL_TIFF_IFD_EXIF, 0x9201, IL_EXIF_TYPE_RATIONAL,  1 },
  { IL_META_APERTURE,                 IL_TIFF_IFD_EXIF, 0x9202, IL_EXIF_TYPE_RATIONAL,  1 },
  { IL_META_BRIGHTNESS,               IL_TIFF_IFD_EXIF, 0x9203, IL_EXIF_TYPE_SRATIONAL, 1 },
  { IL_META_EXPOSURE_BIAS,            IL_TIFF_IFD_EXIF, 0x9204, IL_EXIF_TYPE_SRATIONAL, 1 },
  { IL_META_MAX_APERTURE,             IL_TIFF_IFD_EXIF, 0x9205, IL_EXIF_TYPE_RATIONAL,  1 },
  { IL_META_SUBJECT_DISTANCE,         IL_TIFF_IFD_EXIF, 0x9206, IL_EXIF_TYPE_RATIONAL,  1 },
  { IL_META_METERING_MODE,            IL_TIFF_IFD_EXIF, 0x9207, IL_EXIF_TYPE_WORD,      1 },
  { IL_META_LIGHT_SOURCE,             IL_TIFF_IFD_EXIF, 0x9208, IL_EXIF_TYPE_WORD,      1 },
  { IL_META_FLASH,                    IL_TIFF_IFD_EXIF, 0x9209, IL_EXIF_TYPE_WORD,      1 },
  { IL_META_FOCAL_LENGTH,             IL_TIFF_IFD_EXIF, 0x920A, IL_EXIF_TYPE_RATIONAL,  1 },
  { IL_META_SUBJECT_AREA,             IL_TIFF_IFD_EXIF, 0x9214, IL_EXIF_TYPE_WORD,     -1 },

  { IL_META_FLASH_ENERGY,             IL_TIFF_IFD_EXIF, 0xA20B, IL_EXIF_TYPE_RATIONAL,  1 },

  { IL_META_X_RESOLUTION,             IL_TIFF_IFD0,     0x011A, IL_EXIF_TYPE_RATIONAL,  1 },
  { IL_META_Y_RESOLUTION,             IL_TIFF_IFD0,     0x011B, IL_EXIF_TYPE_RATIONAL,  1 },

  { IL_META_COLOR_SPACE,              IL_TIFF_IFD_EXIF, 0xA001, IL_EXIF_TYPE_WORD,      1 },
  { IL_META_EXPOSURE_MODE,            IL_TIFF_IFD_EXIF, 0xA402, IL_EXIF_TYPE_WORD,      1 },
  { IL_META_WHITE_BALANCE,            IL_TIFF_IFD_EXIF, 0xA403, IL_EXIF_TYPE_WORD,      1 },
  { IL_META_COMPRESSION,              IL_TIFF_IFD0,     0x0103, IL_EXIF_TYPE_WORD,      1 },
  { IL_META_RESOLUTION_UNIT,          IL_TIFF_IFD0,     0x0128, IL_EXIF_TYPE_WORD,      1 },
  { IL_META_SENSING_METHOD,           IL_TIFF_IFD0,     0xA217, IL_EXIF_TYPE_WORD,      1 },

  { IL_META_GPS_VERSION,              IL_TIFF_IFD_GPS,  0x0000, IL_EXIF_TYPE_BYTE,      4 },
  { IL_META_GPS_LATITUDE,             IL_TIFF_IFD_GPS,  0x0002, IL_EXIF_TYPE_RATIONAL,  3 },
  { IL_META_GPS_LONGITUDE,            IL_TIFF_IFD_GPS,  0x0004, IL_EXIF_TYPE_RATIONAL,  3 },
  { IL_META_GPS_ALTITUDE_REF,         IL_TIFF_IFD_GPS,  0x0005, IL_EXIF_TYPE_BYTE,      1 },
  { IL_META_GPS_ALTITUDE,             IL_TIFF_IFD_GPS,  0x0006, IL_EXIF_TYPE_RATIONAL,  3 },
  { IL_META_GPS_TIMESTAMP,            IL_TIFF_IFD_GPS,  0x0007, IL_EXIF_TYPE_RATIONAL,  3 },
  { IL_META_GPS_DOP,                  IL_TIFF_IFD_GPS,  0x000B, IL_EXIF_TYPE_RATIONAL,  1 },
  { IL_META_GPS_SPEED,                IL_TIFF_IFD_GPS,  0x000D, IL_EXIF_TYPE_RATIONAL,  1 },
  { IL_META_GPS_TRACK,                IL_TIFF_IFD_GPS,  0x000F, IL_EXIF_TYPE_RATIONAL,  1 },
  { IL_META_GPS_IMAGE_DIRECTION,      IL_TIFF_IFD_GPS,  0x0011, IL_EXIF_TYPE_RATIONAL,  1 },
  { IL_META_GPS_DEST_LATITUDE,        IL_TIFF_IFD_GPS,  0x0014, IL_EXIF_TYPE_RATIONAL,  3 },
  { IL_META_GPS_DEST_LONGITUDE,       IL_TIFF_IFD_GPS,  0x0016, IL_EXIF_TYPE_RATIONAL,  3 },
  { IL_META_GPS_DEST_BEARING,         IL_TIFF_IFD_GPS,  0x0018, IL_EXIF_TYPE_RATIONAL,  3 },
  { IL_META_GPS_DEST_DISTANCE,        IL_TIFF_IFD_GPS,  0x001A, IL_EXIF_TYPE_RATIONAL,  1 },
  { IL_META_GPS_DIFFERENTIAL,         IL_TIFF_IFD_GPS,  0x001E, IL_EXIF_TYPE_WORD,      1 },

  { IL_META_MAKER_NOTE,               IL_TIFF_IFD_EXIF, 0x927C, IL_EXIF_TYPE_BLOB,     -1 },
  { IL_META_FLASHPIX_VERSION,         IL_TIFF_IFD_EXIF, 0xA000, IL_EXIF_TYPE_BLOB,      4 },
  { IL_META_FILESOURCE,               IL_TIFF_IFD_EXIF, 0xA300, IL_EXIF_TYPE_BLOB,     -1 },
  { IL_META_USER_COMMENT,             IL_TIFF_IFD_EXIF, 0x9286, IL_EXIF_TYPE_BLOB,     -1 },
};

ILuint iGetMetaDataSize(ILenum Type, ILuint Count) {
  switch (Type) {
    case IL_EXIF_TYPE_BYTE:       return Count;
    case IL_EXIF_TYPE_ASCII:      return Count;
    case IL_EXIF_TYPE_BLOB:       return Count;
    case IL_EXIF_TYPE_SBYTE:      return Count;
    case IL_EXIF_TYPE_WORD:       return Count * 2;
    case IL_EXIF_TYPE_SWORD:      return Count * 2;
    case IL_EXIF_TYPE_DWORD:      return Count * 4;
    case IL_EXIF_TYPE_SDWORD:     return Count * 4;
    case IL_EXIF_TYPE_RATIONAL:   return Count * 8;
    case IL_EXIF_TYPE_SRATIONAL:  return Count * 8;
  }
  return 0;
}

ILmetaDesc *iGetMetaDesc(ILenum MetaID) {
  ILuint i;
  for (i=0; i<sizeof(MetaDescriptions) / sizeof(MetaDescriptions[0]); i++) {
    if (MetaDescriptions[i].MetaID == MetaID)
      return &MetaDescriptions[i];
  }
  return NULL;
}

ILmetaDesc *iGetMetaDescFromExifID(ILenum IFD, ILenum ID) {
  ILuint i;
  for (i=0; i<sizeof(MetaDescriptions) / sizeof(MetaDescriptions[0]); i++) {
    if (MetaDescriptions[i].ExifIFD == IFD && MetaDescriptions[i].ExifID == ID)
      return &MetaDescriptions[i];
  }
  return NULL;
}

ILuint iGetMetaLen(ILenum MetaID) {
  ILuint i;
  for (i=0; i<sizeof(MetaDescriptions) / sizeof(MetaDescriptions[0]); i++) {
    if (MetaDescriptions[i].MetaID == MetaID)
      return MetaDescriptions[i].Length;
  }
  return 0;
}

ILuint iGetMetaLenf(ILenum MetaID) {
  ILuint i;
  for (i=0; i<sizeof(MetaDescriptions) / sizeof(MetaDescriptions[0]); i++) {
    if (MetaDescriptions[i].MetaID == MetaID) {
      if (MetaDescriptions[i].Type == IL_EXIF_TYPE_RATIONAL ||
          MetaDescriptions[i].Type == IL_EXIF_TYPE_SRATIONAL)
        return MetaDescriptions[i].Length / 2;

      if (MetaDescriptions[i].Type == IL_EXIF_TYPE_BLOB ||
          MetaDescriptions[i].Type == IL_EXIF_TYPE_ASCII)
        return 0;

      return MetaDescriptions[i].Length;
    }
  }
  return 0;
}

ILboolean iEnumMetadata(ILimage *Image, ILuint Index, ILenum *IFD, ILenum *ID) {
  ILmeta *Exif = Image->MetaTags;
  while(Index) {
    if (Exif == NULL) return IL_FALSE;
    Index--;
    Exif = Exif->Next;
  }

  if (IFD) *IFD = Exif->IFD;
  if (ID)  *ID  = Exif->ID;
  return IL_TRUE;
}

ILboolean iGetMetadata(ILimage *Image, ILenum IFD, ILenum ID, ILenum *Type, ILuint *Count, ILuint *Size, void **Data) {
  ILmeta *Exif = Image->MetaTags;
  while (Exif) {
    if (Exif->IFD == IFD && Exif->ID == ID) {
      if (Type)     *Type     = Exif->Type;
      if (Count)    *Count    = Exif->Length;
      if (Size)     *Size     = Exif->Size;
      if (Data)     *Data     = Exif->Data;

      return IL_TRUE;
    }
    Exif = Exif->Next;
  } while(Exif);
  return IL_FALSE;
}

ILboolean iSetMetadata(ILimage *Image, ILenum IFD, ILenum ID, ILenum Type, ILuint Count, ILuint Size, const void *Data) {
  ILmeta *Meta = Image->MetaTags;
  ILmeta *MetaNew;

  if (Type > IL_EXIF_TYPE_DOUBLE) {
    iSetError(IL_INVALID_ENUM);
    return IL_FALSE;
  }

  // TODO: check for valid type?

  while (Meta) {
    if (Meta->IFD == IFD && Meta->ID == ID) {
      ifree(Meta->Data);
      ifree(Meta->String);

      Meta->Type = Type;
      Meta->Length = Count;
      Meta->Size = Size;
      Meta->Data = ialloc(Size);
      memcpy(Meta->Data, Data, Size);

      if (Meta->Type == IL_EXIF_TYPE_ASCII || ID == 0x9000) {
#ifdef _UNICODE
        Meta->String = iWideFromMultiByte((const char*)Meta->Data);
#else
        Meta->String = iCharStrDup((const char*)Meta->Data);
#endif
      }

      return IL_TRUE;
    }
    if (Meta->Next) {
      Meta = Meta->Next;
    } else {
      break;
    }
  }

  if (IFD != IL_TIFF_IFD0    && IFD != IL_TIFF_IFD1 && IFD != IL_TIFF_IFD_EXIF && 
      IFD != IL_TIFF_IFD_GPS && IFD != IL_TIFF_IFD_INTEROP) {
    iSetError(IL_INVALID_ENUM);
    return IL_FALSE;
  }

  MetaNew           = ioalloc(ILmeta);
  MetaNew->IFD      = IFD;
  MetaNew->ID       = ID;
  MetaNew->Type     = Type;
  MetaNew->Length   = Count;
  MetaNew->Size     = Size;
  MetaNew->Data     = ialloc(Size);
  memcpy(MetaNew->Data, Data, Size);

#ifdef _UNICODE
  MetaNew->String = iWideFromMultiByte((const char*)Data);
#else
  MetaNew->String = iCharStrDup((const char*)Data);
#endif

  if (!Image->MetaTags)
    Image->MetaTags = MetaNew;
  else 
    Meta->Next = MetaNew;

  return IL_TRUE;
}

ILuint iGetMetaiv(ILimage *Image, ILenum MetaID, ILint *Param, ILuint MaxCount) {
  ILuint count, size;
  ILenum Type;
  void *data;
  ILuint i;
  ILmetaDesc *Desc = iGetMetaDesc(MetaID);

  if (!Desc) {
    iSetError(IL_INVALID_ENUM);
    return 0;
  }

  if (!iGetMetadata(Image->BaseImage, Desc->ExifIFD, Desc->ExifID, &Type, &count, &size, &data))
    return 0;

  if (MetaID == IL_META_EXIF_VERSION) {
    if (Param) {
      ILint value = 0;
      value =              ((const char*)data)[0] - 0x30;
      value = value * 10 + ((const char*)data)[1] - 0x30;
      value = value * 10 + ((const char*)data)[2] - 0x30;
      value = value * 10 + ((const char*)data)[3] - 0x30;
      *Param = value;
    }
    return 1;
  }

  if (Type == IL_EXIF_TYPE_RATIONAL) {
    count *= 2;
    Type = IL_EXIF_TYPE_DWORD;
  }

  if (Type == IL_EXIF_TYPE_SRATIONAL) {
    count *= 2;
    Type = IL_EXIF_TYPE_SDWORD;
  }

  if (MaxCount > 0 && count > MaxCount) {
    iSetError(IL_INTERNAL_ERROR);
    return 0;
  }

  if (Param) {
    for (i = 0; i<count; i++) {
      switch (Type) {
        case IL_EXIF_TYPE_BYTE:     Param[i] = ((ILubyte*)  data)[i]; break;
        case IL_EXIF_TYPE_WORD:     Param[i] = ((ILushort*) data)[i]; break;
        case IL_EXIF_TYPE_DWORD:    Param[i] = ((ILuint*)   data)[i]; break;
        case IL_EXIF_TYPE_SBYTE:    Param[i] = ((ILbyte*)   data)[i]; break;
        case IL_EXIF_TYPE_SWORD:    Param[i] = ((ILshort*)  data)[i]; break;
        case IL_EXIF_TYPE_SDWORD:   Param[i] = ((ILint*)    data)[i]; break;
      }
    }
  }

  return count;
}

ILboolean iSetMetaiv(ILimage *Image, ILenum MetaID, const ILint *Param) {
  void *data;
  ILint i;
  ILint Count;
  ILuint Size;

  ILmetaDesc *Desc = iGetMetaDesc(MetaID);
  if (!Desc) {
    iSetError(IL_INVALID_ENUM);
    return IL_FALSE;
  }

  if (Desc->Type == IL_EXIF_TYPE_ASCII || Desc->Type == IL_EXIF_TYPE_BLOB) {
    // not a number
    iSetError(IL_INVALID_ENUM);
    return IL_FALSE;
  }

  Count = Desc->Length;
  if (Count == -1) {
    Count = *Param++;
  }

  Size = iGetMetaDataSize(Desc->Type, Count);
  data = ialloc(Size);
  if (!data) return IL_FALSE;

  for (i=0; i<Count; i++) {
    switch (Desc->Type) {
      case IL_EXIF_TYPE_BYTE:     ((ILubyte*)  data)[i] = Param[i]; break;
      case IL_EXIF_TYPE_WORD:     ((ILushort*) data)[i] = Param[i]; break;
      case IL_EXIF_TYPE_DWORD:    ((ILuint*)   data)[i] = Param[i]; break;
      case IL_EXIF_TYPE_SBYTE:    ((ILbyte*)   data)[i] = Param[i]; break;
      case IL_EXIF_TYPE_SWORD:    ((ILshort*)  data)[i] = Param[i]; break;
      case IL_EXIF_TYPE_SDWORD:   ((ILint*)    data)[i] = Param[i]; break;
      case IL_EXIF_TYPE_RATIONAL: ((ILuint*)   data)[i*2]   = Param[i*2]; 
                                  ((ILuint*)   data)[i*2+1] = Param[i*2+1]; break;
      case IL_EXIF_TYPE_SRATIONAL:((ILint*)    data)[i*2]   = Param[i*2]; 
                                  ((ILint*)    data)[i*2+1] = Param[i*2+1]; break;
    }
  }

  iSetMetadata(Image, Desc->ExifIFD, Desc->ExifID, Desc->Type, Count, Size, data);

  ifree(data);
  return IL_TRUE;
}

ILuint iGetMetafv(ILimage *Image, ILenum MetaID, ILfloat *Param, ILuint MaxCount) {
  ILuint count, size;
  ILenum Type;
  void *data;
  ILuint i;
  ILmetaDesc *Desc = iGetMetaDesc(MetaID);

  if (!Desc) {
    iSetError(IL_INVALID_ENUM);
    return 0;
  }

  if (!iGetMetadata(Image, Desc->ExifIFD, Desc->ExifID, &Type, &count, &size, &data))
    return 0;

  if (MaxCount > 0 && count > MaxCount) {
    iSetError(IL_INTERNAL_ERROR);
    return 0;
  }

  if (Param) {
    for (i = 0; i<count; i++) {
      switch (Type) {
        case IL_EXIF_TYPE_BYTE:     Param[i] = ((ILubyte*)  data)[i]; break;
        case IL_EXIF_TYPE_WORD:     Param[i] = ((ILushort*) data)[i]; break;
        case IL_EXIF_TYPE_DWORD:    Param[i] = ((ILuint*)   data)[i]; break;
        case IL_EXIF_TYPE_SBYTE:    Param[i] = ((ILbyte*)   data)[i]; break;
        case IL_EXIF_TYPE_SWORD:    Param[i] = ((ILshort*)  data)[i]; break;
        case IL_EXIF_TYPE_SDWORD:   Param[i] = ((ILint*)    data)[i]; break;
        case IL_EXIF_TYPE_RATIONAL: Param[i] = (float)(((ILuint*)data)[i*2]) / (float)(((ILuint*)data)[i*2+1]);
        case IL_EXIF_TYPE_SRATIONAL:Param[i] = (float)(((ILint*)data)[i*2])  / (float)(((ILuint*)data)[i*2+1]);
      }
    }
  }

  return count;
}

ILboolean iSetMetafv(ILimage *Image, ILenum MetaID, const ILfloat *Param) {
  void *data;
  ILint i;
  ILint Count;
  ILuint Size;

  ILmetaDesc *Desc = iGetMetaDesc(MetaID);
  if (!Desc) {
    iSetError(IL_INVALID_ENUM);
    return IL_FALSE;
  }

  if (Desc->Type == IL_EXIF_TYPE_ASCII || Desc->Type == IL_EXIF_TYPE_BLOB) {
    // not a number
    iSetError(IL_INVALID_ENUM);
    return IL_FALSE;
  }

  Count = Desc->Length;
  if (Count == -1) {
    Count = *Param++;
  }

  Size = iGetMetaDataSize(Desc->Type, Count);
  data = ialloc(Size);
  if (!data) return IL_FALSE;

  for (i=0; i<Count; i++) {
    switch (Desc->Type) {
      case IL_EXIF_TYPE_BYTE:     ((ILubyte*)  data)[i] = Param[i]; break;
      case IL_EXIF_TYPE_WORD:     ((ILushort*) data)[i] = Param[i]; break;
      case IL_EXIF_TYPE_DWORD:    ((ILuint*)   data)[i] = Param[i]; break;
      case IL_EXIF_TYPE_SBYTE:    ((ILbyte*)   data)[i] = Param[i]; break;
      case IL_EXIF_TYPE_SWORD:    ((ILshort*)  data)[i] = Param[i]; break;
      case IL_EXIF_TYPE_SDWORD:   ((ILint*)    data)[i] = Param[i]; break;
      case IL_EXIF_TYPE_RATIONAL: ((ILuint*)   data)[i*2]   = Param[i] * 10000000; 
                                  ((ILuint*)   data)[i*2+1] = 10000000; break;
      case IL_EXIF_TYPE_SRATIONAL:((ILint*)    data)[i*2]   = Param[i] * 10000000; 
                                  ((ILuint*)   data)[i*2+1] = 10000000; break;
    }
  }

  iSetMetadata(Image, Desc->ExifIFD, Desc->ExifID, Desc->Type, Count, Size, data);

  ifree(data);
  return IL_TRUE;
}

const void * iGetMetax(ILimage *Image, ILenum MetaID, ILuint *Size) {
  ILmeta *Meta = Image->MetaTags;
  ILmetaDesc *Desc = iGetMetaDesc(MetaID);
  if (!Desc) {
    iSetError(IL_INVALID_ENUM);
    return NULL;
  }

  if (Desc->Type == IL_EXIF_TYPE_BLOB) {
    while (Meta) {
      if (Meta->IFD == Desc->ExifIFD && Meta->ID == Desc->ExifID) {
        if (Size) *Size = Meta->Size;
        return Meta->Data;
      }
      Meta = Meta->Next;
    }
  } else {
    iSetError(IL_INVALID_ENUM);
    return NULL;
  }

  return NULL;
}

ILconst_string iGetMetaString(ILimage *BaseImage, ILenum MetaID) {
  ILmeta *Meta = BaseImage->MetaTags;
  ILmetaDesc *Desc = iGetMetaDesc(MetaID);
  if (!Desc) {
    iSetError(IL_INVALID_ENUM);
    return NULL;
  }

  if (Desc->Type == IL_EXIF_TYPE_ASCII || Desc->Type == IL_EXIF_TYPE_BLOB) {
    while (Meta) {
      if (Meta->IFD == Desc->ExifIFD && Meta->ID == Desc->ExifID) {
        return Meta->String;
      }
      Meta = Meta->Next;
    }
  } else {
    iSetError(IL_INVALID_ENUM);
    return NULL;
  }

  return NULL;
}

ILboolean iSetMetaString(ILimage *Image, ILenum MetaID, ILconst_string String) {
  ILmeta *Meta = Image->MetaTags;
  ILmetaDesc *Desc = iGetMetaDesc(MetaID);
  ILuint Length;

  if (!Desc) {
    iSetError(IL_INVALID_ENUM);
    return IL_FALSE;
  }

  if (Desc->Type != IL_EXIF_TYPE_ASCII && Desc->Type != IL_EXIF_TYPE_BLOB) {
    iSetError(IL_INVALID_ENUM);
    return IL_FALSE;
  }

  while (Meta) {
    if (Meta->IFD == Desc->ExifIFD && Meta->ID == Desc->ExifID) {
      ifree(Meta->String);
      ifree(Meta->Data);

      if (!String) {
        Meta->Type = IL_EXIF_TYPE_NONE;
      }

      Length = iStrLen(String) * sizeof(*String);

      Meta->Type    = Desc->Type;
      Meta->Length  = Length + 1;
      Meta->Size    = Length + 1;
      Meta->Data    = iStrDup(String);
      Meta->String  = iStrDup(String);
     
      return IL_TRUE;
    }
    Meta = Meta->Next;
  }
  return IL_FALSE;
}


void iClearMetadata(ILimage *Image) {
  ILimage *BaseImage = Image ? Image->BaseImage : NULL;

  if (!BaseImage) {
    iSetError(IL_ILLEGAL_OPERATION);
    return;
  }

  while(BaseImage->MetaTags) {
    ILmeta *Next = BaseImage->MetaTags->Next;
    ifree(BaseImage->MetaTags->Data);
    ifree(BaseImage->MetaTags->String);
    ifree(BaseImage->MetaTags);
    BaseImage->MetaTags = Next;
  }
}
