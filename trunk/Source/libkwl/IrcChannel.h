/*
kwlbot IRC bot


File:		IrcChannel.h
Purpose:	Class which represents a remote IRC channel

*/

class CIrcChannel;

#ifndef _IRCCHANNEL_H
#define _IRCCHANNEL_H

#include "ScriptObject.h"
#include "Bot.h"
#include "IrcUser.h"
#include "Pool.h"
#include "Script.h"

class DLLEXPORT CIrcChannel : public CScriptObject
{
public:
	CIrcChannel(CBot *pParentBot, const char *szName);
	~CIrcChannel();

	void SetName(const char *szName);
	const char *GetName();
	bool HasUser(CIrcUser *pUser);
	void SetTopic(const char *szTopic);

	CBot *GetParentBot();

	CScriptObject::eScriptType GetType();

	CPool<CIrcUser *> m_plIrcUsers;

private:
	CBot *m_pParentBot;
	char *m_szName;
};

#endif
