/*
kwlbot IRC bot


File:		IrcChannel.h
Purpose:	Class which represents a remote IRC channel

*/

class CIrcChannel;

#ifndef _IRCCHANNEL_H
#define _IRCCHANNEL_H

#include "ScriptObject.h"
#include "IrcUser.h"
#include "Pool.h"
#include "Script.h"

class CIrcChannel : public CScriptObject
{
public:
	CIrcChannel(const char *szName);
	~CIrcChannel();

	void SetName(const char *szName);
	const char *GetName();
	bool HasUser(CIrcUser *pUser);

	CScriptObject::eScriptType GetType();

	CPool<CIrcUser *> m_plIrcUsers;

private:
	char m_szName[MAX_CHANNEL_LEN + 1];
};

#endif
