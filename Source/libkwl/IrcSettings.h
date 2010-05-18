/*
kwlbot IRC bot


File:		IrcSettings.h
Purpose:	Class which manages IRC-related settings

*/

class CIrcSettings;

#ifndef _CIRCSETTINGS_H
#define _CIRCSETTINGS_H

#include "Config.h"

class DLLEXPORT CIrcSettings
{
public:
	bool LoadFromConfig(CConfig *pConfig);

	bool SetNickname(const char *szNickname);
	bool SetIdent(const char *szIdent);
	bool SetRealname(const char *szRealname);

	const char *GetNickname();
	const char *GetIdent();
	const char *GetRealname();

private:
	char m_szNickname[MAX_NICKNAME_LEN + 1];
	char m_szIdent[MAX_IDENT_LEN + 1];
	char m_szRealname[MAX_REALNAME_LEN + 1];
};

#endif
