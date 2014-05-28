#include <string.h>

#ifdef _UNICODE
  #ifndef _WIN32  // At least in Linux, fopen works fine, and wcsicmp is not defined.
    #define iStrICmp wcscasecmp
    #define _wfopen fopen
  #else
    #define iStrIcmp wcsicmp
  #endif
  #define iStrCmp wcscmp
  #define iStrCpy wcscpy
  #define iStrCat wcscat
  #define iStrLen wcslen
#else
  #define iStrIcmp iCharStrICmp

  #define iStrCmp strcmp
  #define iStrCpy strcpy
  #define iStrCat strcat
  #define iStrLen strlen
#endif

#define iCharStrLen strlen
#define iCharStrCpy strcpy
#ifdef _WIN32
  #define iCharStrICmp stricmp
#else
  #define iCharStrICmp strcasecmp
#endif

ILstring  iStrDup(ILconst_string Str);
char *    iCharStrDup(const char *Str);
ILuint    ilStrNCpy(ILconst_string Str);
ILuint    ilCharStrLen(const char *Str);
ILstring  iGetExtension(ILconst_string FileName);
ILboolean iCheckExtension(ILconst_string Arg, ILconst_string Ext);
