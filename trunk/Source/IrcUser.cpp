/*
kwlbot IRC bot


File:		IrcUser.cpp
Purpose:	Class which represents a remote IRC user

*/

#include "StdInc.h"
#include "IrcUser.h"
#include <cstdlib>

CIrcUser::CIrcUser(CBot *pParentBot, const char *szName)
	: m_szName(NULL)
{
	TRACEFUNC("CIrcUser::CIrcUser");

	m_pParentBot = pParentBot;

	SetName(szName);
}

CIrcUser::~CIrcUser()
{
	if (m_szName != NULL)
	{
		free(m_szName);
	}
}

void CIrcUser::SetName(const char *szName)
{
	TRACEFUNC("CIrcUser::SetName");

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

CBot *CIrcUser::GetParentBot()
{
	return m_pParentBot;
}

CScriptObject::eScriptType CIrcUser::GetType()
{
	return IrcUser;
}
