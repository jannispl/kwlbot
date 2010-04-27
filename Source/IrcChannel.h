/*
kwlbot IRC bot


File:		IrcChannel.h
Purpose:	Class which represents a remote IRC channel

*/

class CIrcChannel;

#ifndef _IRCCHANNEL_H
#define _IRCCHANNEL_H

#include "IrcUser.h"
#include "Pool.h"
#include "Script.h"

class CIrcChannel
{
public:
	CIrcChannel(const char *szName);
	~CIrcChannel();

	void SetName(const char *szName);
	const char *GetName();

	v8::Handle<v8::Value> GetScriptThis();

	CPool<CIrcUser *> m_plIrcUsers;

private:
	char m_szName[MAX_CHANNEL_LEN + 1];
};

#endif
