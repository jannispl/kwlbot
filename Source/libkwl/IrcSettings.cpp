/*
kwlbot IRC bot


File:		IrcSettings.cpp
Purpose:	Class which manages IRC-related settings

*/

#include "StdInc.h"
#include "IrcSettings.h"

bool CIrcSettings::LoadFromConfig(CConfig *pConfig)
{
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

#ifdef SERVICE
	if (!pConfig->GetSingleValue("servicehost", &strTemp))
	{
		return false;
	}
	SetServiceHost(strTemp.c_str());
#endif
	
	return true;
}

bool CIrcSettings::SetNickname(const char *szNickname)
{
	m_strNickname = szNickname;
	return true;
}

bool CIrcSettings::SetIdent(const char *szIdent)
{
	m_strIdent = szIdent;
	return true;
}

bool CIrcSettings::SetRealname(const char *szRealname)
{
	m_strRealname = szRealname;
	return true;
}

bool CIrcSettings::SetAlternativeNickname(const char *szNickname)
{
	m_strAlternativeNickname = szNickname;
	return true;
}

bool CIrcSettings::SetQuitMessage(const char *szMessage)
{
	m_strQuitMessage = szMessage;
	return true;
}

#ifdef SERVICE
bool CIrcSettings::SetServiceHost(const char *szHostname)
{
	m_strServiceHost = szHostname;
	return true;
}
#endif

const char *CIrcSettings::GetNickname()
{
	return m_strNickname.c_str();
}

const char *CIrcSettings::GetIdent()
{
	return m_strIdent.c_str();
}

const char *CIrcSettings::GetRealname()
{
	return m_strRealname.c_str();
}

const char *CIrcSettings::GetAlternativeNickname()
{
	return m_strAlternativeNickname.c_str();
}

const char *CIrcSettings::GetQuitMessage()
{
	return m_strQuitMessage.c_str();
}

#ifdef SERVICE
const char *CIrcSettings::GetServiceHost()
{
	return m_strServiceHost.c_str();
}
#endif
