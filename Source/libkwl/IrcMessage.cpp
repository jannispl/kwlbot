/*
kwlbot IRC bot


File:		IrcMessage.cpp
Purpose:	Abstract class which defines an IRC message

*/

#include "StdInc.h"
#include "IrcMessage.h"

void CIrcMessage::Send(CBot *pBot) const
{
	pBot->SendRaw(m_strRaw);
}
