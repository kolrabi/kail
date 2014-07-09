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

#include "il_alloc.h"

#include <stdlib.h>
#include <math.h>

#ifdef MM_MALLOC
#include <mm_malloc.h>
#endif

static void  ILAPIENTRY DefaultFreeFunc(void *Ptr);
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
  void *    ptr = malloc(_size+16+1);
  intptr_t  diff;

  if (!ptr) 
    return NULL;
  
  diff  = ((-(intptr_t)ptr - 1)&15) + 1;

  ptr = (void*)(((char*)ptr)+diff);
  ((char*)ptr)[-1]= diff;
  return ptr;
#endif
}
#endif

/*** Allocation Function ***/
void* ILAPIENTRY ialloc(ILsizei Size) {
  void **Ptr = (void**)ialloc_ptr(Size + sizeof(void*));
  if (Ptr == NULL) {
    iSetError(IL_OUT_OF_MEMORY);
    return NULL;
  }

  // store appropriate ifree_ptr for this chunk of memory
  *Ptr = ifree_ptr;
  return (void*)(Ptr + 1);
}

/*** Deallocation Function ***/
void ILAPIENTRY ifreeReal(void *Ptr) {
  void **pptr = (void **)Ptr;

  if (Ptr == NULL)
    return;

  pptr--;
  ((mFree)*pptr)(pptr);
}

/*** Allocating @a num elements of size @a Size bytes each. */
void* ILAPIENTRY icalloc(const ILsizei Size, const ILsizei Num) {
  void *Ptr = ialloc(Size * Num);
  if (Ptr == NULL)
    return Ptr;
  imemclear(Ptr, Size * Num);
  return Ptr;
}

/*** Default Allocation Function ***/
static void* ILAPIENTRY DefaultAllocFunc(const ILsizei Size) {
#ifdef VECTORMEM
  return vec_malloc(Size);
#else
  return malloc(Size);
#endif //VECTORMEM
}

/*** Default Deallocation Function ***/
static void ILAPIENTRY DefaultFreeFunc(void *ptr) {
  if (ptr) {
#ifdef MM_MALLOC
    _mm_free(ptr);
#else
  #if defined(VECTORMEM) && !defined(POSIX_MEMALIGN) && !defined(VALLOC) && !defined(MEMALIGN) && !defined(MM_MALLOC)
    free(((char*)Ptr) - ((char*)Ptr)[-1]);
  #else     
    free(ptr);
  #endif //OTHERS...
#endif //MM_MALLOC
  }
}

void iSetMemory(mAlloc AllocFunc, mFree FreeFunc) {
  ialloc_ptr  = AllocFunc == NULL ? DefaultAllocFunc : AllocFunc;
  ifree_ptr   = FreeFunc  == NULL ? DefaultFreeFunc  : FreeFunc;
}
