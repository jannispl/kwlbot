#ifndef _DEFINITIONS_H
#define _DEFINITIONS_H

#define VERSION_STRING "v0.5 (" __DATE__ " " __TIME__ ")"

#define SLEEP_MS (50)

#define IRC_EOL "\r\n"
#define IRC_MAX_LEN (512)

#define ENABLE_RAW_EVENT
#define ENABLE_AUTO_RECONNECT
#define ENABLE_SHORTCUT_FUNCTIONS

#define AUTO_RECONNECT_TIMEOUT (10)
#define BOT_RESTART_TIMEOUT (2)

//#define SERVICE

#ifdef SERVICE
  #define INSPIRCD_SID "AAAAAA"

  #define UNREAL 1           // UnrealIRCd
  #define INSPIRCD 2         // InspIRCd
  #define HYBRID 3           // IRCD-Hybrid
  #define BAHAMUT 4          // Bahamut IRCd
  #define ULTIMATE 5         // UltimateIRCd
  #define VIAGRA 6           // ViagraIRCd
  #define IRC 7              // ircd

  #define IRCD UNREAL
#endif

#endif
