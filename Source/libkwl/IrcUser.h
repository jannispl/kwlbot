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
public:
	CIrcUser(CBot *pParentBot, const char *szName);
	~CIrcUser();

	void SetName(const char *szName);
	const char *GetName();
	bool HasChannel(CIrcChannel *pChannel);

	CBot *GetParentBot();

	CScriptObject::eScriptType GetType();

	CPool<CIrcChannel *> m_plIrcChannels;
	std::map<CIrcChannel *, char> m_mapChannelModes;

private:
	CBot *m_pParentBot;
	char *m_szName;
};

#endif
