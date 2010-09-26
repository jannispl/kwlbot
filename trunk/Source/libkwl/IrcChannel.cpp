/*
kwlbot IRC bot


File:		IrcChannel.cpp
Purpose:	Class which represents a remote IRC channel

*/

#include "StdInc.h"
#include "IrcChannel.h"
#include <cstdlib>

CIrcChannel::CIrcChannel(CBot *pParentBot, const std::string &strName)
	: m_bHasDetailedUsers(false)
{
	m_pParentBot = pParentBot;

	m_strName = strName;
}

CIrcChannel::~CIrcChannel()
{
}

const std::string &CIrcChannel::GetName()
{
	return m_strName;
}

CIrcUser *CIrcChannel::FindUser(const char *szNickname, bool bCaseSensitive)
{
	typedef int (* Compare_t)(const char *, const char *);
	Compare_t pfnCompare = bCaseSensitive ? strcmp : stricmp;

	for (CPool<CIrcUser *>::iterator i = m_plIrcUsers.begin(); i != m_plIrcUsers.end(); ++i)
	{
		if (pfnCompare(szNickname, (*i)->GetNickname().c_str()) == 0)
		{
			return *i;
		}
	}

	return NULL;
}

bool CIrcChannel::HasUser(CIrcUser *pUser)
{
	for (CPool<CIrcUser *>::iterator i = m_plIrcUsers.begin(); i != m_plIrcUsers.end(); ++i)
	{
		if (*i == pUser)
		{
			return true;
		}
	}
	return false;
}

CPool<CIrcUser *> *CIrcChannel::GetUsers()
{
	return &m_plIrcUsers;
}

CBot *CIrcChannel::GetParentBot()
{
	return m_pParentBot;
}

CScriptObject::eScriptType CIrcChannel::GetType()
{
	return IrcChannel;
}
