/*
kwlbot IRC bot


File:		IrcChannel.h
Purpose:	Class which represents a remote IRC channel

*/

class CIrcChannel;

#ifndef _IRCCHANNEL_H
#define _IRCCHANNEL_H

#include "IrcUser.h"

class CIrcChannel
{
public:
	CIrcChannel(const char *szName);
	~CIrcChannel();

	void SetName(const char *szName);
	const char *GetName();

	CPool<CIrcUser *> m_plIrcUsers;

private:
	char m_szName[MAX_CHANNEL_LEN + 1];
};

#endif
