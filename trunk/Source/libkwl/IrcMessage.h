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
	virtual void Send(CBot *pBot);

protected:
	char m_szRaw[IRC_MAX_LEN + 1];
};

#endif
