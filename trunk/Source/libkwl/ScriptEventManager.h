/*
kwlbot IRC bot


File:		ScriptEventManager.h
Purpose:	Handles calling events in scripts

*/

class CScriptEventManager;

#ifndef _SCRIPTEVENTMANAGER_H
#define _SCRIPTEVENTMANAGER_H

#include "Core.h"
#include "EventManager.h"

class CScriptEventManager : public CEventManager
{
public:
	CScriptEventManager(CCore *pParentCore);
	~CScriptEventManager();

	void OnBotCreated(CBot *pBot);
	void OnBotDestroyed(CBot *pBot);
	void OnBotConnected(CBot *pBot);
	void OnBotJoinedChannel(CBot *pBot, CIrcChannel *pChannel);
	void OnUserJoinedChannel(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel);
	void OnUserLeftChannel(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel, const char *szReason);
	void OnUserKickedUser(CBot *pBot, CIrcUser *pUser, CIrcUser *pVictim, CIrcChannel *pChannel, const char *szReason);
	void OnUserQuit(CBot *pBot, CIrcUser *pUser, const char *szReason);
	void OnUserChangedNickname(CBot *pBot, CIrcUser *pUser, const char *szOldNickname);
	void OnUserPrivateMessage(CBot *pBot, CIrcUser *pUser, const char *szMessage);
	void OnUserChannelMessage(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel, const char *szMessage);
	void OnUserSetChannelModes(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel, const char *szModes, const char *szParams);
	void OnBotReceivedRaw(CBot *pBot, const char *szRaw);

private:
	CCore *m_pParentCore;
};

#endif
