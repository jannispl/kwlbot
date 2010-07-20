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

  #pragma warning(disable:4251)
#else
  #include <unistd.h>
  #include <string.h>
#endif

#include <stdarg.h>

#include "Definitions.h"
#include "Pool.h"

#ifdef _DEBUG
  #define dbgprintf printf
#else
  #define dbgprintf
#endif

#ifdef WIN32
  #define DLLEXPORT __declspec(dllexport)
#else
  #define DLLEXPORT __attribute__ ((visibility("default")))
#endif
