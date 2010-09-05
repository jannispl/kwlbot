#ifndef _DEFINITIONS_H
#define _DEFINITIONS_H

#define VERSION_STRING "v0.5 (" __DATE__ " " __TIME__ ")"

#define SLEEP_MS (50)

#define IRC_EOL "\r\n"
#define IRC_MAX_LEN (512)

#define ENABLE_RAW_EVENT
#define ENABLE_AUTO_RECONNECT
#define AUTO_RECONNECT_TIMEOUT (10)
#define BOT_RESTART_TIMEOUT (2)

#define SERVICE

#ifdef SERVICE
  #define UNREAL 1
  #define INSPIRCD 2
  #define HYBRID 3

  #define IRCD UNREAL
#endif

#endif
