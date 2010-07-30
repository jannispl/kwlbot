/*
kwlbot IRC bot


File:		IrcUser.h
Purpose:	Class which represents a remote IRC user

*/

class CIrcUser;

#ifndef _IRCUSER_H
#define _IRCUSER_H

#include "ScriptObject.h"
#include "Bot.h"
#include "IrcChannel.h"
#include "Script.h"
#include <map>

class DLLEXPORT CIrcUser : public CScriptObject
{
	friend class CBot;

public:
	CIrcUser(CBot *pParentBot, const char *szNickname, bool bTemporary = false);
	~CIrcUser();

	void SetNickname(const char *szNickname);
	const char *GetNickname();
	void UpdateIfNecessary(const char *szHost, const char *szIdent);
	const char *GetHost();
	const char *GetIdent();

	bool IsTemporary();

	bool HasChannel(CIrcChannel *pChannel);
	char GetModeOnChannel(CIrcChannel *pChannel);

	CBot *GetParentBot();

	CScriptObject::eScriptType GetType();

#ifdef WIN32
	template class DLLEXPORT CPool<CIrcChannel *>;
#endif
	CPool<CIrcChannel *> m_plIrcChannels;

private:
	CBot *m_pParentBot;
	std::string m_strNickname;
	std::string m_strHost;
	std::string m_strIdent;
	std::map<CIrcChannel *, char> m_mapChannelModes;
	bool m_bTemporary;
};

#endif
