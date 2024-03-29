/*
kwlbot IRC bot


File:		IrcMessages.h
Purpose:	Various classes describing IRC messages

*/

class CRawMessage;
class CJoinMessage;
class CPartMessage;
class CPrivateMessage;
class CNoticeMessage;
class CTopicMessage;
class CWhoMessage;

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

class CJoinMessage : public CIrcMessage
{
public:
#if !defined(SERVICE) || IRCD != HYBRID
	CJoinMessage(const std::string &strChannel)
	{
		m_strRaw = "JOIN " + strChannel;
	}
#else
	CJoinMessage(time_t ullChannelStamp, const std::string &strNickname, const std::string &strChannel)
	{
		m_bServiceMessage = true;
		char szTemp[32];
		sprintf(szTemp, "%ld", (long)ullChannelStamp);
		m_strRaw = "SJOIN " + std::string(szTemp) + " " + strChannel + " " + strNickname;
	}
#endif
};

class CPartMessage : public CIrcMessage
{
public:
	CPartMessage(const std::string &strChannel)
	{
		m_strRaw = "PART " + strChannel;
	}

	CPartMessage(const std::string &strChannel, const std::string &strReason)
	{
		m_strRaw = "PART " + strChannel + " :" + strReason;
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

class CWhoMessage : public CIrcMessage
{
public:
	CWhoMessage(const std::string &strTarget)
	{
		m_strRaw = "WHO " + strTarget;
	}
};

#endif
