/*
kwlbot IRC bot


File:		IrcSettings.cpp
Purpose:	Class which manages IRC-related settings

*/

#include "StdInc.h"
#include "IrcSettings.h"

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

	if (!pConfig->GetSingleValue("altnick", &strTemp))
	{
		strTemp = GetNickname() + std::string("`");
	}
	SetAlternativeNickname(strTemp.c_str());

	if (!pConfig->GetSingleValue("quitmsg", &strTemp))
	{
		strTemp = GetNickname();
	}
	SetQuitMessage(strTemp.c_str());
	
	return true;
}

bool CIrcSettings::SetNickname(const char *szNickname)
{
	TRACEFUNC("CIrcSettings::SetNickname");

	m_strNickname = szNickname;
	return true;
}

bool CIrcSettings::SetIdent(const char *szIdent)
{
	TRACEFUNC("CIrcSettings::SetIdent");

	m_strIdent = szIdent;
	return true;
}

bool CIrcSettings::SetRealname(const char *szRealname)
{
	TRACEFUNC("CIrcSettings::SetRealname");

	m_strRealname = szRealname;
	return true;
}

bool CIrcSettings::SetAlternativeNickname(const char *szNickname)
{
	TRACEFUNC("CIrcSettings::SetAlternativeNickname");

	m_strAlternativeNickname = szNickname;
	return true;
}

bool CIrcSettings::SetQuitMessage(const char *szMessage)
{
	TRACEFUNC("CIrcSettings::SetQuitMessage");

	m_strQuitMessage = szMessage;
	return true;
}

const char *CIrcSettings::GetNickname()
{
	TRACEFUNC("CIrcSettings::GetNickname");

	return m_strNickname.c_str();
}

const char *CIrcSettings::GetIdent()
{
	TRACEFUNC("CIrcSettings::GetIdent");

	return m_strIdent.c_str();
}

const char *CIrcSettings::GetRealname()
{
	TRACEFUNC("CIrcSettings::GetRealname");

	return m_strRealname.c_str();
}

const char *CIrcSettings::GetAlternativeNickname()
{
	TRACEFUNC("CIrcSettings::GetAlternativeNickname");

	return m_strAlternativeNickname.c_str();
}

const char *CIrcSettings::GetQuitMessage()
{
	TRACEFUNC("CIrcSettings::GetQuitMessage");

	return m_strQuitMessage.c_str();
}
