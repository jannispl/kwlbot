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
	CIrcMessage() {}
	~CIrcMessage() {}

	void Send(CBot *pBot) const;

protected:
	std::string m_strRaw;
};

#endif
