/*
kwlbot IRC bot


File:		IrcUser.cpp
Purpose:	Class which represents a remote IRC user

*/

#include "StdInc.h"
#include "IrcUser.h"

CIrcUser::CIrcUser(const char *szName)
{
	TRACEFUNC("CIrcUser::CIrcUser");

	strcpy(m_szName, szName);
}

CIrcUser::~CIrcUser()
{
}

void CIrcUser::SetName(const char *szName)
{
	TRACEFUNC("CIrcUser::SetName");

	strcpy(m_szName, szName);
}

const char *CIrcUser::GetName()
{
	TRACEFUNC("CIrcUser::GetName");

	return m_szName;
}

bool CIrcUser::HasChannel(CIrcChannel *pChannel)
{
	TRACEFUNC("CIrcUser::HasChannel");

	for (CPool<CIrcChannel *>::iterator i = m_plIrcChannels.begin(); i != m_plIrcChannels.end(); ++i)
	{
		if (*i == pChannel)
		{
			return true;
		}
	}
	return false;
}

CScriptObject::eScriptType CIrcUser::GetType()
{
	return IrcUser;
}
