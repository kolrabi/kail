//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Last modified: 08/17/2008
//
// Filename: src-IL/src/il_alloc.c
//
// Description: Memory allocation functions
//
//-----------------------------------------------------------------------------

// TODO: store current ifree ptr with allocated memory to enable
// use of ilSetMemory when there are already allocated objects.

#define __ALLOC_C

// Memory leak detection
#ifdef _WIN32
	#ifdef _DEBUG 
		#define _CRTDBG_MAP_ALLOC
		#include <stdlib.h>
		#ifndef _WIN32_WCE  // Does not have this header.
			#include <crtdbg.h>
		#endif
	#endif
#endif//_WIN32

#include "il_internal.h"

#include <stdlib.h>
#include <math.h>

#ifdef MM_MALLOC
#include <mm_malloc.h>
#endif

static void  ILAPIENTRY DefaultFreeFunc(const void * CONST_RESTRICT Ptr);
static void* ILAPIENTRY DefaultAllocFunc(const ILsizei Size);

// Global variables: functions to allocate and deallocate memory
// Access would have to be protected by mutexes in a multithreaded environment
static mAlloc ialloc_ptr = DefaultAllocFunc;
static mFree  ifree_ptr  = DefaultFreeFunc;

/*** Vector Allocation/Deallocation Function ***/
#ifdef VECTORMEM
void *vec_malloc(const ILsizei size) {
	const ILsizei _size =  size % 16 > 0 ? size + 16 - (size % 16) : size; // align size value
	
#if   defined(MM_MALLOC)
	return _mm_malloc(_size,16);
#elif defined(VALLOC)
	return valloc( _size );
#elif defined(POSIX_MEMALIGN)
	char *buffer;
	return posix_memalign((void**)&buffer, 16, _size) == 0 ? buffer : NULL;
#elif defined(MEMALIGN)
	return memalign( 16, _size );
#else
	// Memalign hack from ffmpeg for MinGW
	void *ptr;
	int   diff;
	ptr = malloc(_size+16+1);
	diff= ((-(int)ptr - 1)&15) + 1;
	ptr = (void*)(((char*)ptr)+diff);
	((char*)ptr)[-1]= diff;
	return ptr;
#endif
}
#endif

/*** Allocation/Deallocation Function ***/
void* ILAPIENTRY ialloc(const ILsizei Size) {
	void *Ptr = ialloc_ptr(Size);
	if (Ptr == NULL)
		ilSetError(IL_OUT_OF_MEMORY);
	return Ptr;
}

void ILAPIENTRY ifree(const void * CONST_RESTRICT Ptr) {
	if (Ptr == NULL)
		return;
	ifree_ptr(Ptr);
	return;
}

void* ILAPIENTRY icalloc(const ILsizei Size, const ILsizei Num) {
    void *Ptr = ialloc(Size * Num);
    if (Ptr == NULL)
    	return Ptr;
    imemclear(Ptr, Size * Num);
    return Ptr;
}

/*** Default Allocation/Deallocation Function ***/
static void* ILAPIENTRY DefaultAllocFunc(const ILsizei Size) {
#ifdef VECTORMEM
	return vec_malloc(Size);
#else
	return malloc(Size);
#endif //VECTORMEM
}

static void ILAPIENTRY DefaultFreeFunc(const void * CONST_RESTRICT ptr)
{
	if (ptr)
	{
#ifdef MM_MALLOC
	    _mm_free((void*)ptr);
#else
#if defined(VECTORMEM) & !defined(POSIX_MEMALIGN) & !defined(VALLOC) & !defined(MEMALIGN) & !defined(MM_MALLOC)
	    free(((char*)Ptr) - ((char*)Ptr)[-1]);
#else	    
	    free((void*)ptr);
#endif //OTHERS...
#endif //MM_MALLOC
	}
}

/*** Manipulate Allocation/Deallocation Function ***/
void ILAPIENTRY ilResetMemory()  // Deprecated
{
	ialloc_ptr = DefaultAllocFunc;
	ifree_ptr  = DefaultFreeFunc;
}

void ILAPIENTRY ilSetMemory(mAlloc AllocFunc, mFree FreeFunc)
{
	ialloc_ptr 	= AllocFunc == NULL ? DefaultAllocFunc : AllocFunc;
	ifree_ptr 	= FreeFunc  == NULL ? DefaultFreeFunc  : FreeFunc;
}
