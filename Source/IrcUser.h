/*
kwlbot IRC bot


File:		IrcUser.h
Purpose:	Class which represents a remote IRC user

*/

class CIrcUser;

#ifndef _IRCUSER_H
#define _IRCUSER_H

#include "IrcChannel.h"
#include "Script.h"
#include <map>

class CIrcUser
{
public:
	CIrcUser(const char *szName);
	~CIrcUser();

	void SetName(const char *szName);
	const char *GetName();

	v8::Handle<v8::Value> GetScriptThis();

	CPool<CIrcChannel *> m_plIrcChannels;
	std::map<CIrcChannel *, char> m_mapChannelModes;

private:
	char m_szName[MAX_NICKNAME_LEN + 1];
};

#endif
