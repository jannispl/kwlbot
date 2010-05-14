/*
kwlbot IRC bot


File:		IrcSettings.cpp
Purpose:	Class which manages IRC-related settings

*/

#include "StdInc.h"
#include "IrcSettings.h"
#include <string.h>

bool CIrcSettings::LoadFromConfig(CConfig *pConfig)
{
	TRACEFUNC("CIrcSettings::LoadFromConfig");

	if (pConfig == NULL)
	{
		return false;
	}

	std::string strTemp;
	if (!pConfig->GetSingleValue("nickname", &strTemp))
	{
		return false;
	}
	SetNickname(strTemp.c_str());

	if (!pConfig->GetSingleValue("ident", &strTemp))
	{
		return false;
	}
	SetIdent(strTemp.c_str());

	if (!pConfig->GetSingleValue("realname", &strTemp))
	{
		return false;
	}
	SetRealname(strTemp.c_str());
	
	return true;
}

bool CIrcSettings::SetNickname(const char *szNickname)
{
	TRACEFUNC("CIrcSettings::SetNickname");

	if (strlen(szNickname) > MAX_NICKNAME_LEN)
	{
		return false;
	}

	strcpy(m_szNickname, szNickname);
	return true;
}

bool CIrcSettings::SetIdent(const char *szIdent)
{
	TRACEFUNC("CIrcSettings::SetIdent");

	if (strlen(szIdent) > MAX_IDENT_LEN)
	{
		return false;
	}

	strcpy(m_szIdent, szIdent);
	return true;
}

bool CIrcSettings::SetRealname(const char *szRealname)
{
	TRACEFUNC("CIrcSettings::SetRealname");

	if (strlen(szRealname) > MAX_REALNAME_LEN)
	{
		return false;
	}

	strcpy(m_szRealname, szRealname);
	return true;
}

const char *CIrcSettings::GetNickname()
{
	TRACEFUNC("CIrcSettings::GetNickname");

	return m_szNickname;
}

const char *CIrcSettings::GetIdent()
{
	TRACEFUNC("CIrcSettings::GetIdent");

	return m_szIdent;
}

const char *CIrcSettings::GetRealname()
{
	TRACEFUNC("CIrcSettings::GetRealname");

	return m_szRealname;
}
