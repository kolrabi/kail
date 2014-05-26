#ifndef ALLOC_H
#define ALLOC_H

#if defined (__ALLOC_C)
	#define __ALLOC_EXTERN
#else
	#define __ALLOC_EXTERN extern
#endif
#include <IL/il.h>

__ALLOC_EXTERN mAlloc ialloc_ptr;
__ALLOC_EXTERN mFree  ifree_ptr;

#endif//ALLOC_H
