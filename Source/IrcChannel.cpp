/*
kwlbot IRC bot


File:		IrcChannel.cpp
Purpose:	Class which represents a remote IRC channel

*/

#include "StdInc.h"
#include "IrcChannel.h"

CIrcChannel::CIrcChannel(const char *szName)
{
	TRACEFUNC("CIrcChannel::CIrcChannel");

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

CScriptObject::eScriptType CIrcChannel::GetType()
{
	return IrcChannel;
}