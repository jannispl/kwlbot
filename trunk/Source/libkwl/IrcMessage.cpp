/*
kwlbot IRC bot


File:		IrcMessage.cpp
Purpose:	Abstract class which defines an IRC message

*/

#include "StdInc.h"
#include "IrcMessage.h"

void CIrcMessage::Send(CBot *pBot)
{
	pBot->SendRaw(m_szRaw);
}
