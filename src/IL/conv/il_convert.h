#ifndef IL_CONV_H
#define IL_CONV_H

#include "il_internal.h"

#define iBytes(ptr)    ((ILbyte*)  (void*)(ptr))
#define iShorts(ptr)   ((ILshort*) (void*)(ptr))
#define iInts(ptr)     ((ILint*)   (void*)(ptr))
#define iUBytes(ptr)   ((ILubyte*) (void*)(ptr))
#define iUShorts(ptr)  ((ILushort*)(void*)(ptr))
#define iUInts(ptr)    ((ILuint*)  (void*)(ptr))
#define iFloats(ptr)   ((ILfloat*) (void*)(ptr))
#define iDoubles(ptr)  ((ILdouble*)(void*)(ptr))

#ifdef CLAMP_DOUBLES
  #define CLAMP_DOUBLE_VALUE(f) (IL_CLAMP(f))
#else
  #define CLAMP_DOUBLE_VALUE(f) (f)
#endif

#ifdef CLAMP_FLOATS
  #define CLAMP_FLOAT_VALUE(f) (IL_CLAMP(f))
#else
  #define CLAMP_FLOAT_VALUE(f) (f)
#endif

#ifdef CLAMP_HALF
  #define CLAMP_HALF_VALUE(f) (IL_CLAMP(f))
#else
  #define CLAMP_HALF_VALUE(f) (f)
#endif

INLINE void iCopyPixelElement( const void *Src, ILuint SrcIndex, void *Dest, ILuint DestIndex, ILuint Bpc ) {
  memcpy( iUBytes(Dest) + DestIndex * Bpc, iUBytes(Src) + SrcIndex * Bpc, Bpc );
}

INLINE ILfloat iGetPixelElement( const void *Buffer, ILuint Index, ILenum Type) {
  switch(Type) {
    case IL_UNSIGNED_BYTE:  return (iUBytes(Buffer) [Index]               ) / (ILfloat)IL_MAX_UNSIGNED_BYTE; 
    case IL_BYTE:           return ((iBytes(Buffer)  [Index] - IL_MIN_BYTE ) & IL_MAX_UNSIGNED_BYTE ) / (ILfloat)IL_MAX_UNSIGNED_BYTE;          
    case IL_UNSIGNED_SHORT: return (iUShorts(Buffer)[Index]               ) / (ILfloat)IL_MAX_UNSIGNED_SHORT;
    case IL_SHORT:          return ((iShorts(Buffer) [Index] - IL_MIN_SHORT) & IL_MAX_UNSIGNED_SHORT )/ (ILfloat)IL_MAX_UNSIGNED_SHORT;         
    case IL_UNSIGNED_INT:   return (iUInts(Buffer)  [Index]               ) / (ILfloat)IL_MAX_UNSIGNED_INT;
    case IL_INT:            return ((iInts(Buffer)   [Index] - IL_MIN_INT  ) & IL_MAX_UNSIGNED_INT ) / (ILfloat)IL_MAX_UNSIGNED_INT;           
    case IL_FLOAT:          return iFloats(Buffer)  [Index];
    case IL_DOUBLE:         return iDoubles(Buffer) [Index];
    case IL_HALF:           return iGetHalf( iShorts(Buffer)+Index );
  }
  return 0.0;
}

INLINE void iSetPixelElement( void *Buffer, ILuint Index, ILenum Type, ILdouble val ) {
  switch(Type) {
    case IL_UNSIGNED_BYTE:  iUBytes(Buffer) [Index] = val * IL_MAX_UNSIGNED_BYTE; break;
    case IL_BYTE:           iBytes(Buffer)  [Index] = val * IL_MAX_UNSIGNED_BYTE + IL_MIN_BYTE; break;
    case IL_UNSIGNED_SHORT: iUShorts(Buffer)[Index] = val * IL_MAX_UNSIGNED_SHORT; break;
    case IL_SHORT:          iShorts(Buffer) [Index] = val * IL_MAX_UNSIGNED_SHORT + IL_MIN_SHORT; break;
    case IL_UNSIGNED_INT:   iUInts(Buffer)  [Index] = val * IL_MAX_UNSIGNED_INT; break;
    case IL_INT:            iInts(Buffer)   [Index] = val * IL_MAX_UNSIGNED_INT + IL_MIN_INT; break;
    case IL_FLOAT:          iFloats(Buffer) [Index] = val; break;
    case IL_DOUBLE:         iDoubles(Buffer)[Index] = val; break;
    case IL_HALF:           iSetHalf( val, iShorts(Buffer)+Index ); break;
  }
}

INLINE void iSetPixelElementMax( void *Buffer, ILuint Index, ILenum Type ) {
  switch(Type) {
    case IL_UNSIGNED_BYTE:  iUBytes(Buffer) [Index] = IL_MAX_UNSIGNED_BYTE; break;
    case IL_BYTE:           iBytes(Buffer)  [Index] = IL_MAX_BYTE; break;
    case IL_UNSIGNED_SHORT: iUShorts(Buffer)[Index] = IL_MAX_UNSIGNED_SHORT; break;
    case IL_SHORT:          iShorts(Buffer) [Index] = IL_MAX_SHORT; break;
    case IL_UNSIGNED_INT:   iUInts(Buffer)  [Index] = IL_MAX_UNSIGNED_INT; break;
    case IL_INT:            iInts(Buffer)   [Index] = IL_MAX_INT; break;
    case IL_FLOAT:          iFloats(Buffer) [Index] = 1.0f; break;
    case IL_DOUBLE:         iDoubles(Buffer)[Index] = 1.0; break;
    case IL_HALF:           iSetHalf( 1.0f, iShorts(Buffer)+Index ); break;
  }
}

#endif
