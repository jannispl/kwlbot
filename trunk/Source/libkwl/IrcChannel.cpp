/*
kwlbot IRC bot


File:		IrcChannel.cpp
Purpose:	Class which represents a remote IRC channel

*/

#include "StdInc.h"
#include "IrcChannel.h"
#include <cstdlib>

CIrcChannel::CIrcChannel(CBot *pParentBot, const char *szName)
	: m_szName(NULL), m_bHasDetailedUsers(false)
{
	m_pParentBot = pParentBot;

	UpdateName(szName);
}

CIrcChannel::~CIrcChannel()
{
	if (m_szName != NULL)
	{
		free(m_szName);
	}
}

void CIrcChannel::UpdateName(const char *szName)
{
	size_t iLen = strlen(szName);
	if (m_szName == NULL)
	{
		m_szName = (char *)malloc(iLen + 1);
	}
	else
	{
		m_szName = (char *)realloc(m_szName, iLen + 1);
	}

	strcpy(m_szName, szName);
}

const char *CIrcChannel::GetName()
{
	return m_szName;
}

CIrcUser *CIrcChannel::FindUser(const char *szNickname, bool bCaseSensitive)
{
	typedef int (* Compare_t)(const char *, const char *);
	Compare_t pfnCompare = bCaseSensitive ? strcmp : stricmp;

	for (CPool<CIrcUser *>::iterator i = m_plIrcUsers.begin(); i != m_plIrcUsers.end(); ++i)
	{
		if (pfnCompare(szNickname, (*i)->GetNickname()) == 0)
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

void CIrcChannel::SetTopic(const char *szTopic)
{
	m_pParentBot->SendRawFormat("TOPIC %s :%s", GetName(), szTopic);
}

CBot *CIrcChannel::GetParentBot()
{
	return m_pParentBot;
}

CScriptObject::eScriptType CIrcChannel::GetType()
{
	return IrcChannel;
}
