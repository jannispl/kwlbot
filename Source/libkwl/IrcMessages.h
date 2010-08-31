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
		static char szBuffer[IRC_MAX_LEN + 1];

		va_list vaArgs;
		va_start(vaArgs, szFormat);
		int iLength = vsnprintf(szBuffer, IRC_MAX_LEN + 1, szFormat, vaArgs);
		va_end(vaArgs);

		m_strRaw.assign(szBuffer, iLength);
	}
};

class CPrivateMessage : public CIrcMessage
{
public:
	CPrivateMessage(const std::string &strTarget, const std::string &strMessage)
	{
		m_strRaw = "PRIVMSG " + strTarget + " :" + strMessage;
	}
};

class CNoticeMessage : public CIrcMessage
{
public:
	CNoticeMessage(const std::string &strTarget, const std::string &strMessage)
	{
		m_strRaw = "NOTICE " + strTarget + " :" + strMessage;
	}
};

class CTopicMessage : public CIrcMessage
{
public:
	CTopicMessage(const std::string &strChannel, const std::string &strTopic)
	{
		m_strRaw = "TOPIC " + strChannel + " :" + strTopic;
	}
};

#endif
