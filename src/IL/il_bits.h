//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_bits.h
//
// Description: Implements a file class that reads/writes bits directly.
//
//-----------------------------------------------------------------------------


#ifndef BITS_H
#define BITS_H

#include "il_internal.h"


// Struct for dealing with reading bits from a file
typedef struct BITFILE
{
  SIO *     io;
  ILuint    BitPos;
  ILint     ByteBitOff;
  ILubyte   Buff;
} BITFILE;

// Functions for reading bits from a file
BITFILE* bitfile(SIO *io);
ILint    bclose(BITFILE *BitFile);
ILint    btell(BITFILE *BitFile);
ILint    bseek(BITFILE *BitFile, ILint Offset, ILuint Mode);
ILint    bread(void *Buffer, ILuint Size, ILuint Number, BITFILE *BitFile);

// Useful macros for manipulating bits
#define SetBits(var, bits)    (var |= bits)
#define ClearBits(var, bits)  (var &= ~(bits))

#endif//BITS_H
