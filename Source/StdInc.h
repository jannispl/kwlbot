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
#else
  #include <unistd.h>
#endif

#include "Definitions.h"

#include "Core.h"
#include "Bot.h"
#include "IrcSettings.h"
#include "IrcSocket.h"

#include "IrcChannel.h"
#include "IrcUser.h"

#include "Config.h"
#include "TcpSocket.h"
#include "Pool.h"

#ifdef _DEBUG
  #include "Debug.h"
  extern CDebug *g_pDebug;
  #define TRACEFUNC(func) g_pDebug->SetLastFunction(func)
#else
  #define TRACEFUNC(func)
#endif