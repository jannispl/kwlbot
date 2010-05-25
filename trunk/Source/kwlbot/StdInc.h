/*
kwlbot IRC bot


File:		StdInc.h
Purpose:	Precompiled header file

*/

#include <stdio.h>

#ifdef WIN32
  #define WIN32_LEAN_AND_MEAN
  #define WIN32_EXTRA_LEAN
  #define VC_EXTRALEAN
  #define NOPROXYSTUB
  #define NOSERVICE
  #define NOSOUND
  #define NOKANJI
  #define NOIMAGE
  #define NOTAPE
  #define NOCOMM
  #define NOMCX
  #define NOIME
  #define NORPC

  #include <windows.h>

  //#pragma warning(disable:4251)
#else
  #include <unistd.h>
#endif

#include <stdarg.h>

#define DLLEXPORT __attribute__ ((visibility("default")))

#include "../libkwl/Definitions.h"

#ifdef _DEBUG
  #include "../libkwl/Debug.h"
  extern CDebug *g_pDebug;
  #define TRACEFUNC(func) g_pDebug->SetLastFunction(func)
  #define dbgprintf printf
#else
  #define TRACEFUNC(func)
  #define dbgprintf
#endif
