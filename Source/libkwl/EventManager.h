/*
kwlbot IRC bot


File:		EventManager.h
Purpose:	Abstract class which defines an event manager

*/

class CEventManager;

#ifndef _CEVENTMANAGER_H
#define _CEVENTMANAGER_H

#include "Core.h"
#include "Bot.h"
#include "IrcUser.h"
#include "IrcChannel.h"
#include "Script.h"

/**
 * Abstract class which defines an event manager.
 */
class CEventManager
{
public:
	virtual void OnBotCreated(CBot *pBot) = 0;
	virtual void OnBotDestroyed(CBot *pBot) = 0;
	virtual void OnBotConnected(CBot *pBot) = 0;
	virtual void OnBotJoinedChannel(CBot *pBot, CIrcChannel *pChannel) = 0;
	virtual void OnUserJoinedChannel(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel) = 0;
	virtual void OnUserLeftChannel(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel, const char *szReason) = 0;
	virtual void OnUserKickedUser(CBot *pBot, CIrcUser *pUser, CIrcUser *pVictim, CIrcChannel *pChannel, const char *szReason) = 0;
	virtual void OnUserQuit(CBot *pBot, CIrcUser *pUser, const char *szReason) = 0;
	virtual void OnUserChangedNickname(CBot *pBot, CIrcUser *pUser, const char *szOldNickname) = 0;
	virtual void OnUserPrivateMessage(CBot *pBot, CIrcUser *pUser, const char *szMessage) = 0;
	virtual void OnUserChannelMessage(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel, const char *szMessage) = 0;
	virtual void OnUserSetChannelModes(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel, const char *szModes, const char *szParams) = 0;
	virtual void OnBotGotChannelUserList(CBot *pBot, CIrcChannel *pChannel) = 0;
	virtual void OnBotReceivedRaw(CBot *pBot, const char *szRaw) = 0;
};

#endif
