#define ILU_INTERNAL_C

#include "ilu_internal.h"

#if IL_THREAD_SAFE_PTHREAD
#include <pthread.h>
  static pthread_key_t iTlsKey;
#elif IL_THREAD_SAFE_WIN32
  static DWORD         iTlsKey = ~0;
#endif

static void iInitTlsData(ILU_TLS_DATA *Data) {
  imemclear(Data, sizeof(*Data)); 
  Data->iluFilter = ILU_NEAREST;
  Data->iluPlacement = ILU_CENTER;
}

#if IL_THREAD_SAFE_PTHREAD
// only called when thread safety is enabled and a thread stops
static void iFreeTLSData(void *ptr) {
  ILU_TLS_DATA *Data = (ILU_TLS_DATA*)ptr;
  ifree(Data);
}
#endif

ILU_TLS_DATA * iGetTLSData() {
#if IL_THREAD_SAFE_PTHREAD
  ILU_TLS_DATA *iDataPtr = (ILU_TLS_DATA*)pthread_getspecific(iTlsKey);
  if (iDataPtr == NULL) {
    iDataPtr = ioalloc(ILU_TLS_DATA);
    pthread_setspecific(iTlsKey, iDataPtr);
    iInitTlsData(iDataPtr);
  }

  return iDataPtr;

#elif IL_THREAD_SAFE_WIN32

  ILU_TLS_DATA *iDataPtr = (ILU_TLS_DATA*)TlsGetValue(iTlsKey);
  if (iDataPtr == NULL) {
    iDataPtr = ioalloc(ILU_TLS_DATA);
    TlsSetValue(iTlsKey, iDataPtr);
    iInitTlsData(iDataPtr);
  }

  return iDataPtr;

#else
  static ILU_TLS_DATA iData;
  static ILU_TLS_DATA *iDataPtr = NULL;

  if (iDataPtr == NULL) {
    iDataPtr = &iData;
    iInitTlsData(iDataPtr);
  }
  return iDataPtr;
#endif
}

void iInitThreads(void) {
  #if IL_THREAD_SAFE_PTHREAD
    pthread_key_create(&iTlsKey, &iFreeTLSData);
  #elif IL_THREAD_SAFE_WIN32
    iTlsKey = TlsAlloc();
  #endif
}
