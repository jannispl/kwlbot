/*
kwlbot IRC bot


File:		IrcUser.cpp
Purpose:	Class which represents a remote IRC user

*/

#include "StdInc.h"
#include "IrcUser.h"
#include <cstdlib>

CIrcUser::CIrcUser(CBot *pParentBot, const char *szNickname, bool bTemporary)
{
	TRACEFUNC("CIrcUser::CIrcUser");

	m_pParentBot = pParentBot;
	m_bTemporary = bTemporary;

	SetNickname(szNickname);
}

CIrcUser::~CIrcUser()
{
}

void CIrcUser::SetNickname(const char *szNickname)
{
	TRACEFUNC("CIrcUser::SetNickname");

	m_strNickname = szNickname;
}

const char *CIrcUser::GetNickname()
{
	TRACEFUNC("CIrcUser::GetNickname");

	return m_strNickname.c_str();
}

void CIrcUser::SetIdent(const char *szIdent)
{
	TRACEFUNC("CIrcUser::SetIdent");

	m_strIdent = szIdent;
}

const char *CIrcUser::GetIdent()
{
	TRACEFUNC("CIrcUser::GetIdent");

	return m_strIdent.c_str();
}

bool CIrcUser::IsTemporary()
{
	return m_bTemporary;
}

void CIrcUser::SetHost(const char *szHost)
{
	TRACEFUNC("CIrcUser::SetHost");

	m_strHost = szHost;
}

const char *CIrcUser::GetHost()
{
	TRACEFUNC("CIrcUser::GetHost");

	return m_strHost.c_str();
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

char CIrcUser::GetModeOnChannel(CIrcChannel *pChannel)
{
	TRACEFUNC("CIrcUser::GetModeOnChannel");

	return m_mapChannelModes[pChannel];
}

CBot *CIrcUser::GetParentBot()
{
	return m_pParentBot;
}

CScriptObject::eScriptType CIrcUser::GetType()
{
	return IrcUser;
}
