/*
kwlbot IRC bot


File:		IrcMessages.h
Purpose:	Various classes describing IRC messages

*/

class CRawMessage;
class CPrivateMessage;
class CNoticeMessage;
class CTopicMessage;

#ifndef _IRCMESSAGES_H
#define _IRCMESSAGES_H

#include "IrcMessage.h"
#include "Bot.h"

#ifdef WIN32
  #define snprintf _snprintf
#endif

class CRawMessage : public CIrcMessage
{
public:
	CRawMessage(const char *szFormat, ...)
	{
		va_list vaArgs;
		va_start(vaArgs, szFormat);
		vsnprintf(m_szRaw, IRC_MAX_LEN + 1, szFormat, vaArgs);
		va_end(vaArgs);
	}
};

class CPrivateMessage : public CIrcMessage
{
public:
	CPrivateMessage(const char *szTarget, const char *szMessage)
	{
		snprintf(m_szRaw, IRC_MAX_LEN, "PRIVMSG %s :%s", szTarget, szMessage);
	}
};

class CNoticeMessage : public CIrcMessage
{
public:
	CNoticeMessage(const char *szTarget, const char *szMessage)
	{
		snprintf(m_szRaw, IRC_MAX_LEN, "NOTICE %s :%s", szTarget, szMessage);
	}
};

class CTopicMessage : public CIrcMessage
{
public:
	CTopicMessage(const char *szChannel, const char *szTopic)
	{
		snprintf(m_szRaw, IRC_MAX_LEN, "TOPIC %s :%s", szChannel, szTopic);
	}
};

#endif
