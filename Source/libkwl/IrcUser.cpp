/*
kwlbot IRC bot


File:		IrcUser.cpp
Purpose:	Class which represents a remote IRC user

*/

#include "StdInc.h"
#include "IrcUser.h"
#include <cstdlib>

CIrcUser::CIrcUser(CBot *pParentBot, const std::string &strNickname, bool bTemporary)
{
	m_pParentBot = pParentBot;
	m_bTemporary = bTemporary;

	m_strNickname = strNickname;
}

CIrcUser::~CIrcUser()
{
}

const std::string &CIrcUser::GetNickname()
{
	return m_strNickname;
}

const std::string &CIrcUser::GetIdent()
{
	return m_strIdent;
}

const std::string &CIrcUser::GetRealname()
{
	return m_strRealname;
}

bool CIrcUser::IsTemporary()
{
	return m_bTemporary;
}

const std::string &CIrcUser::GetHostname()
{
	return m_strHostname;
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

#ifdef SERVICE
const std::string &CIrcUser::GetUserModes()
{
	return m_strUserModes;
}

const std::string &CIrcUser::GetCloakedHost()
{
	return m_strCloakedHost;
}

const std::string &CIrcUser::GetVirtualHost()
{
	return m_strVirtualHost;
}
#endif

CBot *CIrcUser::GetParentBot()
{
	return m_pParentBot;
}

CScriptObject::eScriptType CIrcUser::GetType()
{
	return IrcUser;
}

void CIrcUser::UpdateIfNecessary(const std::string &strIdent, const std::string &strHostname)
{
	if (strIdent[0] != '\0')
	{
		m_strIdent = strIdent;
	}

	if (strHostname[0] != '\0')
	{
		m_strHostname = strHostname;
	}
}
