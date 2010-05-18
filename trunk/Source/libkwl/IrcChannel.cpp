/*
kwlbot IRC bot


File:		IrcChannel.cpp
Purpose:	Class which represents a remote IRC channel

*/

#include "StdInc.h"
#include "IrcChannel.h"
#include <cstdlib>

CIrcChannel::CIrcChannel(CBot *pParentBot, const char *szName)
	: m_szName(NULL)
{
	TRACEFUNC("CIrcChannel::CIrcChannel");

	m_pParentBot = pParentBot;

	SetName(szName);
}

CIrcChannel::~CIrcChannel()
{
	TRACEFUNC("CIrcChannel::~CIrcChannel");

	if (m_szName != NULL)
	{
		free(m_szName);
	}
}

void CIrcChannel::SetName(const char *szName)
{
	TRACEFUNC("CIrcChannel::SetName");

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

void CIrcChannel::SetTopic(const char *szTopic)
{
	TRACEFUNC("CIrcChannel::SetTopic");

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
