/*
kwlbot IRC bot


File:		EventManager.cpp
Purpose:	Handles calling events in scripts

*/

class CEventManager;

#ifndef _CEVENTMANAGER_H
#define _CEVENTMANAGER_H

#include "Core.h"
#include "Bot.h"
#include "IrcUser.h"
#include "IrcChannel.h"
#include "Script.h"

class CEventManager
{
public:
	CEventManager(CCore *pParentCore);
	~CEventManager();

	void OnBotConnected(CBot *pBot);
	void OnBotJoinedChannel(CBot *pBot, CIrcChannel *pChannel);
	void OnUserJoinedChannel(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel);
	void OnUserLeftChannel(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel);
	void OnUserKickedUser(CBot *pBot, CIrcUser *pUser, CIrcUser *pVictim, CIrcChannel *pChannel);
	void OnUserQuit(CBot *pBot, CIrcUser *pUser);
	void OnUserChangedNickname(CBot *pBot, CIrcUser *pUser, const char *szOldNickname);
	void OnUserChannelMessage(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel, const char *szMessage);
	void OnUserSetChannelModes(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel, const char *szModes, const char *szParams);

private:
	CCore *m_pParentCore;
};

#endif
