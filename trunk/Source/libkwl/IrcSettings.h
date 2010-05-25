/*
kwlbot IRC bot


File:		IrcSettings.h
Purpose:	Class which manages IRC-related settings

*/

class CIrcSettings;

#ifndef _CIRCSETTINGS_H
#define _CIRCSETTINGS_H

#include <string>
#include "Config.h"

class DLLEXPORT CIrcSettings
{
public:
	bool LoadFromConfig(CConfig *pConfig);

	bool SetNickname(const char *szNickname);
	bool SetIdent(const char *szIdent);
	bool SetRealname(const char *szRealname);
	bool SetAlternativeNickname(const char *szNickname);
	bool SetQuitMessage(const char *szMessage);

	const char *GetNickname();
	const char *GetIdent();
	const char *GetRealname();
	const char *GetAlternativeNickname();
	const char *GetQuitMessage();

private:
	std::string m_strNickname;
	std::string m_strIdent;
	std::string m_strRealname;
	std::string m_strAlternativeNickname;
	std::string m_strQuitMessage;
};

#endif
