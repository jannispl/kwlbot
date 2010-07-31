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
	m_pParentBot = pParentBot;
	m_bTemporary = bTemporary;

	UpdateNickname(szNickname);
}

CIrcUser::~CIrcUser()
{
}

void CIrcUser::UpdateNickname(const char *szNickname)
{
	m_strNickname = szNickname;
}

const char *CIrcUser::GetNickname()
{
	return m_strNickname.c_str();
}

void CIrcUser::UpdateIfNecessary(const char *szIdent, const char *szHostname)
{
	if (szIdent[0] != '\0')
	{
		m_strIdent = szIdent;
	}

	if (szHostname[0] != '\0')
	{
		m_strHostname = szHostname;
	}
}

const char *CIrcUser::GetIdent()
{
	return m_strIdent.c_str();
}

bool CIrcUser::IsTemporary()
{
	return m_bTemporary;
}

const char *CIrcUser::GetHostname()
{
	return m_strHostname.c_str();
}

bool CIrcUser::HasChannel(CIrcChannel *pChannel)
{
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
