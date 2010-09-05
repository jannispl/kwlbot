/*
kwlbot IRC bot


File:		IrcMessage.cpp
Purpose:	Abstract class which defines an IRC message

*/

#include "StdInc.h"
#include "IrcMessage.h"

void CIrcMessage::Send(CBot *pBot) const
{
#ifndef SERVICE
	pBot->SendRaw(m_strRaw);
#else
	if (!m_bServiceMessage)
	{
		pBot->SendRawFormat(":%s %s", pBot->GetSocket()->GetCurrentNickname(), m_strRaw.c_str());
	}
	else
	{
		pBot->SendRaw(m_strRaw);
	}
#endif
}
