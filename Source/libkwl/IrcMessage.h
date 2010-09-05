/*
kwlbot IRC bot


File:		IrcMessage.h
Purpose:	Abstract class which defines an IRC message

*/

class CIrcMessage;

#ifndef _IRCMESSAGE_H
#define _IRCMESSAGE_H

#include "Bot.h"

/**
 * @brief Abstract class which defines an IRC message.
 */
class CIrcMessage
{
public:
	CIrcMessage()
#ifdef SERVICE
		: m_bServiceMessage(false)
#endif
	{
	}

	~CIrcMessage()
	{
	}

	void Send(CBot *pBot) const;

protected:
	std::string m_strRaw;

#ifdef SERVICE
private:
	bool m_bServiceMessage;
#endif
};

#endif
