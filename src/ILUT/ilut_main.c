//-----------------------------------------------------------------------------
//
// ImageLib Utility Toolkit Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/28/2001 <--Y2K Compliant! =]
//
// Filename: src-ILUT/src/ilut_main.c
//
// Description: Startup functions
//
//-----------------------------------------------------------------------------

/**
 * @file
 * @brief Main functions.
 *
 * @defgroup ILUT Image Library Utility Toolkit
 *                Functions to make images usable in other APIs.

 * @ingroup ILUT
 * @{
 * @defgroup ilu_setup  Initialization / Deinitalization
 * @}
 */

#ifdef _WIN32

#include <windows.h>

#ifndef IL_STATIC_LIB
#if (defined(IL_USE_PRAGMA_LIBS))
	#if defined(_MSC_VER) || defined(__BORLANDC__)
		#pragma comment(lib, "ILU.lib")
	#endif
#endif

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	(void)hModule;  (void)ul_reason_for_call;  (void)lpReserved;
	return TRUE;
}
#endif
#endif
