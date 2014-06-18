/* -----------------------------------------------------------------------------
//
// ImageLib Sources
// Last modified: 17/04/2005
// by Meloni Dario
// 
// Description: Common altivec function.
//
//--------------------------------------------------------------------------- */

#include <IL/config.h>

#ifdef ALTIVEC_GCC
#include "altivec_common.h"

vector float fill_vector_f( float value ) {
	vector_t vec;
	vec.sf[0] = value;
	vector float temp = vec_ld(0,vec.sf);
	return vec_splat(temp,0);
}

#endif
