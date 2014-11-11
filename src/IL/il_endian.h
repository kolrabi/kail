//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 01/29/2009
//
// Filename: src-IL/include/il_endian.h
//
// Description: Handles Endian-ness
//
//-----------------------------------------------------------------------------

#ifndef IL_ENDIAN_H
#define IL_ENDIAN_H

#include "il_internal.h"

#ifndef WORDS_BIGENDIAN  // This is defined by cmake
	#define WORDS_LITTLEENDIAN
#endif

#ifdef WORDS_BIGENDIAN
	#define Short(s) 			iSwapShort(s)
	#define UShort(s) 		iSwapUShort(s)
	#define Int(i) 				iSwapInt(i)
	#define UInt(i) 			iSwapUInt(i)
	#define Float(f) 			iSwapFloat(f)
	#define Double(d) 		iSwapDouble(d)
 
	#define BigShort(s)  
	#define BigUShort(s)  
	#define BigInt(i)  
	#define BigUInt(i)  
	#define BigFloat(f)  
	#define BigDouble(d)  
#else
	#define Short(s)  
	#define UShort(s)  
	#define Int(i)  
	#define UInt(i)  
	#define Float(f)  
	#define Double(d)  

	#define BigShort(s) 	iSwapShort(s)
	#define BigUShort(s) 	iSwapUShort(s)
	#define BigInt(i) 		iSwapInt(i)
	#define BigUInt(i) 		iSwapUInt(i)
	#define BigFloat(f) 	iSwapFloat(f)
	#define BigDouble(d) 	iSwapDouble(d)
#endif

#ifdef IL_ENDIAN_C // defined when included from il_endian.c
#undef NOINLINE
#undef INLINE
#define INLINE static inline
#endif

#define SWAP_BYTES(x, y) { ILubyte t = (x); (x) = (y); (y) = t; }

#ifndef NOINLINE
INLINE void iSwapUShort(ILushort *s)  {
	ILubyte *tmp = (ILubyte*)s;
	SWAP_BYTES(tmp[0], tmp[1]);
}

INLINE void iSwapShort(ILshort *s) {
	iSwapUShort((ILushort*)s);
}

INLINE void iSwapUInt(ILuint *i) {
	ILubyte *tmp = (ILubyte*)i;
	SWAP_BYTES(tmp[0], tmp[3]);
	SWAP_BYTES(tmp[1], tmp[2]);
}

INLINE void iSwapInt(ILint *i) {
	iSwapUInt((ILuint*)i);
}

INLINE void iSwapFloat(ILfloat *f) {
	ILubyte *tmp = (ILubyte*)f;
	SWAP_BYTES(tmp[0], tmp[3]);
	SWAP_BYTES(tmp[1], tmp[2]);
}

INLINE void iSwapDouble(ILdouble *d) {
	ILubyte *tmp = (ILubyte*)d;
	SWAP_BYTES(tmp[0], tmp[7]);
	SWAP_BYTES(tmp[1], tmp[6]);
	SWAP_BYTES(tmp[2], tmp[5]);
	SWAP_BYTES(tmp[3], tmp[4]);
}


INLINE ILushort GetLittleUShort(SIO* io) {
	ILushort s;
	SIOread(io, &s, sizeof(ILushort), 1);
	UShort(&s);
	return s;
}

INLINE ILshort GetLittleShort(SIO* io) {
	ILshort s;
	SIOread(io, &s, sizeof(ILshort), 1);
	Short(&s);
	return s;
}

INLINE ILuint GetLittleUInt(SIO* io) {
	ILuint i;
	SIOread(io, &i, sizeof(ILuint), 1);
	UInt(&i);
	return i;
}

INLINE ILint GetLittleInt(SIO* io) {
	ILint i;
	SIOread(io, &i, sizeof(ILint), 1);
	Int(&i);
	return i;
}

INLINE ILfloat GetLittleFloat(SIO* io) {
	ILfloat f;
	SIOread(io, &f, sizeof(ILfloat), 1);
	Float(&f);
	return f;
}

INLINE ILdouble GetLittleDouble(SIO* io) {
	ILdouble d;
	SIOread(io, &d, sizeof(ILdouble), 1);
	Double(&d);
	return d;
}


INLINE ILushort GetBigUShort(SIO* io) {
	ILushort s;
	SIOread(io, &s, sizeof(ILushort), 1);
	BigUShort(&s);
	return s;
}


INLINE ILshort GetBigShort(SIO* io) {
	ILshort s;
	SIOread(io, &s, sizeof(ILshort), 1);
	BigShort(&s);
	return s;
}


INLINE ILuint GetBigUInt(SIO* io) {
	ILuint i;
	SIOread(io, &i, sizeof(ILuint), 1);
	BigUInt(&i);
	return i;
}


INLINE ILint GetBigInt(SIO* io) {
	ILint i;
	SIOread(io, &i, sizeof(ILint), 1);
	BigInt(&i);
	return i;
}


INLINE ILfloat GetBigFloat(SIO* io) {
	ILfloat f;
	SIOread(io, &f, sizeof(ILfloat), 1);
	BigFloat(&f);
	return f;
}


INLINE ILdouble GetBigDouble(SIO* io) {
	ILdouble d;
	SIOread(io, &d, sizeof(ILdouble), 1);
	BigDouble(&d);
	return d;
}

INLINE ILuint SaveLittleUShort(SIO* io, ILushort s) {
	UShort(&s);
	return io->write(&s, sizeof(ILushort), 1, io->handle);
}

INLINE ILuint SaveLittleShort(SIO* io, ILshort s) {
	Short(&s);
	return io->write(&s, sizeof(ILshort), 1, io->handle);
}


INLINE ILuint SaveLittleUInt(SIO* io, ILuint i) {
	UInt(&i);
	return io->write(&i, sizeof(ILuint), 1, io->handle);
}


INLINE ILuint SaveLittleInt(SIO* io, ILint i) {
	Int(&i);
	return io->write(&i, sizeof(ILint), 1, io->handle);
}

INLINE ILuint SaveLittleFloat(SIO* io, ILfloat f) {
	Float(&f);
	return io->write(&f, sizeof(ILfloat), 1, io->handle);
}


INLINE ILuint SaveLittleDouble(SIO* io, ILdouble d) {
	Double(&d);
	return io->write(&d, sizeof(ILdouble), 1, io->handle);
}


INLINE ILuint SaveBigUShort(SIO* io, ILushort s) {
	BigUShort(&s);
	return io->write(&s, sizeof(ILushort), 1, io->handle);
}


INLINE ILuint SaveBigShort(SIO* io, ILshort s) {
	BigShort(&s);
	return io->write(&s, sizeof(ILshort), 1, io->handle);
}


INLINE ILuint SaveBigUInt(SIO* io, ILuint i) {
	BigUInt(&i);
	return io->write(&i, sizeof(ILuint), 1, io->handle);
}


INLINE ILuint SaveBigInt(SIO* io, ILint i) {
	BigInt(&i);
	return io->write(&i, sizeof(ILint), 1, io->handle);
}


INLINE ILuint SaveBigFloat(SIO* io, ILfloat f) {
	BigFloat(&f);
	return io->write(&f, sizeof(ILfloat), 1, io->handle);
}


INLINE ILuint SaveBigDouble(SIO* io, ILdouble d) {
	BigDouble(&d);
	return io->write(&d, sizeof(ILdouble), 1, io->handle);
}
#endif//NOINLINE

void		iEndianSwapData(ILimage *_Image);

#endif//ENDIAN_H
