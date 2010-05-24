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
	CIrcUser(CBot *pParentBot, const char *szName);
	~CIrcUser();

	void SetName(const char *szName);
	const char *GetName();
	bool HasChannel(CIrcChannel *pChannel);
	char GetModeOnChannel(CIrcChannel *pChannel);

	CBot *GetParentBot();

	CScriptObject::eScriptType GetType();

	template class DLLEXPORT CPool<CIrcChannel *>;
	CPool<CIrcChannel *> m_plIrcChannels;

private:
	CBot *m_pParentBot;
	char *m_szName;
	std::map<CIrcChannel *, char> m_mapChannelModes;
};

#endif
