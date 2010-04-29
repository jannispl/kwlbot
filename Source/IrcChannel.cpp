/*
kwlbot IRC bot


File:		IrcChannel.cpp
Purpose:	Class which represents a remote IRC channel

*/

#include "StdInc.h"
#include "IrcChannel.h"

CIrcChannel::CIrcChannel(CBot *pParentBot, const char *szName)
{
	TRACEFUNC("CIrcChannel::CIrcChannel");

	m_pParentBot = pParentBot;

	strcpy(m_szName, szName);
}

CIrcChannel::~CIrcChannel()
{
}

void CIrcChannel::SetName(const char *szName)
{
	TRACEFUNC("CIrcChannel::SetName");

	strcpy(m_szName, szName);
}

const char *CIrcChannel::GetName()
{
	TRACEFUNC("CIrcChannel::GetName");

	return m_szName;
}

bool CIrcChannel::HasUser(CIrcUser *pUser)
{
	TRACEFUNC("CIrcChannel::HasUser");

	for (CPool<CIrcUser *>::iterator i = m_plIrcUsers.begin(); i != m_plIrcUsers.end(); ++i)
	{
		if (*i == pUser)
		{
			return true;
		}
	}
	return false;
}

CBot *CIrcChannel::GetParentBot()
{
	return m_pParentBot;
}

CScriptObject::eScriptType CIrcChannel::GetType()
{
	return IrcChannel;
}
