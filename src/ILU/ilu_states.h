//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/28/2001 <--Y2K Compliant! =]
//
// Filename: src-ILU/include/ilu_states.h
//
// Description: The state machine
//
//-----------------------------------------------------------------------------


#ifndef STATES_H
#define STATES_H

extern ILenum iluFilter;
extern ILenum iluPlacement;

void iGetIntegerv(ILenum Mode, ILint *Param);
ILconst_string iGetString(ILenum StringName);
void iImageParameter(ILenum PName, ILenum Param) ;

#endif//STATES_H
